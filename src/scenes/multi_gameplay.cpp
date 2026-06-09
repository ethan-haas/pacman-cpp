#include "pacman/app.hpp"
#include "pacman/multi_gameplay.hpp"
#include "pacman/render.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace pac {

#include <cstdio>
#include <cstdlib>

// fwds for HUD/turn-swap drawers
void draw_solo_hud(SDL_Renderer*, const PlayerState&, float);
void draw_alternating_hud(SDL_Renderer*, const PlayerState&, const PlayerState&, int, float);
void draw_coop_hud(SDL_Renderer*, const PlayerState&, const PlayerState&, int, int, int, float);
void draw_turn_swap(SDL_Renderer*, int, int, float);
bool is_global_powerup(PowerUp);

MultiGameplay::MultiGameplay(App& a, const std::string& mode_name)
    : app(a), maze(Maze::load(1)) {
    // Controllers already initialised by App; nothing to do here.
    ghosts = make_ghosts(maze);
    total_pellets_start = maze.total_pellets();
    mode = make_mode(mode_name);
    mode->setup(*this);
}

int MultiGameplay::next_alive_idx() const {
    for (size_t off = 1; off <= players.size(); ++off) {
        size_t idx = (static_cast<size_t>(active_idx) + off) % players.size();
        if (players[idx].lives > 0) return static_cast<int>(idx);
    }
    return active_idx;
}

void MultiGameplay::reset_mode_timing() {
    mode_idx = 0;
    mode_timer = SCATTER_CHASE[0].duration;
    global_mode = SCATTER_CHASE[0].mode;
    fright_left = 0.f;
}

void MultiGameplay::tick_mode_timer(float dt) {
    if (fright_left > 0) {
        fright_left -= dt;
        if (fright_left <= 0) {
            fright_left = 0;
            for (auto& g : ghosts) {
                if (g.mode == GhostMode::Frightened) {
                    g.mode = (std::string(global_mode) == "chase") ? GhostMode::Chase : GhostMode::Scatter;
                }
            }
        }
    } else {
        bool terminal = mode_idx >= static_cast<int>(SCATTER_CHASE.size()) - 1;
        if (!terminal) {
            mode_timer -= dt;
            if (mode_timer <= 0) {
                mode_idx = std::min<int>(mode_idx + 1, SCATTER_CHASE.size() - 1);
                mode_timer = SCATTER_CHASE[mode_idx].duration;
                const char* new_mode = SCATTER_CHASE[mode_idx].mode;
                if (std::string(new_mode) != global_mode) {
                    for (auto& g : ghosts) {
                        if (g.mode == GhostMode::Scatter || g.mode == GhostMode::Chase) {
                            g.mode = (std::string(new_mode) == "chase") ? GhostMode::Chase : GhostMode::Scatter;
                            g.reverse();
                        }
                    }
                }
                global_mode = new_mode;
            }
        }
    }
}

void MultiGameplay::magnet_pull(PlayerState& ps, float /*dt*/) {
    auto& pellets = maze.pellets_mut();
    std::vector<Tile> to_remove;
    for (const auto& tile : pellets) {
        float dx = tile.x - ps.pac.x, dy = tile.y - ps.pac.y;
        if (dx * dx + dy * dy <= 9.f) to_remove.push_back(tile);
    }
    bool is_coop = std::string(mode->name()) == "coop";
    for (const auto& tl : to_remove) {       // 'tl' not 't' — t shadows member
        pellets.erase(tl);
        ++ps.pellets_eaten;
        ++pellets_eaten_total;
        if (is_coop) {
            int g = shared_score.eat_pellet();
            ps.contribution += g;
        } else {
            ps.score.eat_pellet();
        }
    }
}

void MultiGameplay::tick_one_player(float dt, PlayerState& ps) {
    tick_mode_timer(dt);
    ps.pac.speed_mult = ps.powerups.is_active(PowerUp::Speed) ? 1.4f : 1.0f;
    ps.pac.update(dt, maze);
    ps.time_alive += dt;
    std::pair<float, float> blinky_pos = ghosts.empty() ? std::make_pair(0.f, 0.f)
                                                        : std::make_pair(ghosts[0].x, ghosts[0].y);
    bool frozen = ps.powerups.is_active(PowerUp::Freeze);
    for (auto& g : ghosts) {
        Pacman* target = mode->ghost_target_pac(*this, g);
        g.update(dt, maze, *target, blinky_pos, global_mode, frozen);
    }
    ps.powerups.update(dt);
    auto d = app.router.poll_dpad(ps.player_id);
    if (!d.empty()) {
        if (d == "up") ps.pac.request(Dir::Up);
        else if (d == "down") ps.pac.request(Dir::Down);
        else if (d == "left") ps.pac.request(Dir::Left);
        else if (d == "right") ps.pac.request(Dir::Right);
    }
    collisions_solo(ps);
    ps.powerups.maybe_spawn(maze, ps.pellets_eaten, total_pellets_start, dt, ps.pac.x, ps.pac.y);
    if (ps.powerups.is_active(PowerUp::Magnet)) magnet_pull(ps, dt);
    if (maze.total_pellets() == 0) enter_level_complete();
}

void MultiGameplay::tick_two_players(float dt) {
    tick_mode_timer(dt);
    for (auto& ps : players) {
        ps.pac.speed_mult = ps.powerups.is_active(PowerUp::Speed) ? 1.4f : 1.0f;
        if (ps.pac.alive) {
            ps.pac.update(dt, maze);
            ps.time_alive += dt;
        }
        auto d = app.router.poll_dpad(ps.player_id);
        if (!d.empty() && ps.pac.alive) {
            if (d == "up") ps.pac.request(Dir::Up);
            else if (d == "down") ps.pac.request(Dir::Down);
            else if (d == "left") ps.pac.request(Dir::Left);
            else if (d == "right") ps.pac.request(Dir::Right);
        }
        ps.powerups.update(dt);
    }
    std::pair<float, float> blinky_pos = ghosts.empty() ? std::make_pair(0.f, 0.f)
                                                        : std::make_pair(ghosts[0].x, ghosts[0].y);
    bool any_freeze = std::any_of(players.begin(), players.end(),
        [](const PlayerState& p) { return p.powerups.is_active(PowerUp::Freeze); });
    for (auto& g : ghosts) {
        Pacman* target = mode->ghost_target_pac(*this, g);
        g.update(dt, maze, *target, blinky_pos, global_mode, any_freeze);
    }
    collisions_coop();
    mode->tick_rescue(*this, dt);
    for (auto& ps : players) {
        // PRD §10.5 independent lives — respawn if THIS player has lives left
        if (!ps.pac.alive && ps.drop_timer <= 0 && ps.lives > 0) {
            ps.pac.respawn(maze);
        }
    }
    auto& bag = players[0].powerups;
    bag.maybe_spawn(maze, pellets_eaten_total, total_pellets_start, dt,
                    players[0].pac.x, players[0].pac.y);
    for (auto& ps : players) {
        if (ps.powerups.is_active(PowerUp::Magnet)) magnet_pull(ps, dt);
    }
    if (maze.total_pellets() == 0) enter_level_complete();
}

void MultiGameplay::collisions_solo(PlayerState& ps) {
    ps.score.multiplier = ps.powerups.is_active(PowerUp::ScoreX2) ? 2.0f : 1.0f;
    if (!ps.pac.alive) return;
    Tile pt = ps.pac.tile();
    auto& pellets = maze.pellets_mut();
    auto& energizers = maze.energizers_mut();
    if (pellets.erase(pt) > 0) {
        ++ps.pellets_eaten;
        ++pellets_eaten_total;
        ps.score.eat_pellet();
        if (g_audio) g_audio->play_pellet();
    }
    if (energizers.erase(pt) > 0) {
        ++ps.pellets_eaten;
        ++pellets_eaten_total;
        ps.score.eat_energizer();
        if (g_audio) g_audio->play_energizer();
        fright_left = std::max(2.0f, FRIGHT_BASE - (ps.level - 1) * 0.5f);
        for (auto& g : ghosts) {
            if (g.mode == GhostMode::Scatter || g.mode == GhostMode::Chase) {
                g.mode = GhostMode::Frightened;
                g.reverse();
            }
        }
        ps.score.reset_streak();
    }
    if (ps.powerups.on_maze) {
        auto pk = *ps.powerups.on_maze;
        if (std::fabs(pk.x - ps.pac.x) < 0.6f && std::fabs(pk.y - ps.pac.y) < 0.6f) {
            ps.powerups.collect();
            apply_pickup_solo(ps, pk.kind, pk.x, pk.y);
        }
    }
    int ate_this_tick = 0;
    for (auto& g : ghosts) {
        if (std::fabs(g.x - ps.pac.x) < 0.7f && std::fabs(g.y - ps.pac.y) < 0.7f) {
            if (g.mode == GhostMode::Frightened) {
                int gained = ps.score.eat_ghost();
                ++ps.ghosts_eaten;
                ps.longest_combo = std::max(ps.longest_combo, ps.score.ghost_streak);
                add_popup("+" + std::to_string(gained), g.x, g.y - 0.5f, {200, 220, 255});
                if (g_audio) g_audio->play_ghost_eat();
                g.mode = GhostMode::Eaten;
                ++ate_this_tick;
            } else if (g.mode == GhostMode::Eaten) {
                // pass-through
            } else if (ps.powerups.is_active(PowerUp::Phase)) {
                // intangible
            } else {
                if (ps.pac.invuln > 0) continue;
                die_solo(ps);
                return;
            }
        }
    }
    if (ate_this_tick > 0 && ps.score.ghost_streak == 4 && ps.lives < LIVES_CAP) ++ps.lives;
    ps.score.check_extra_life(ps.lives);
}

void MultiGameplay::collisions_coop() {
    bool any_x2 = std::any_of(players.begin(), players.end(),
        [](const PlayerState& p) { return p.powerups.is_active(PowerUp::ScoreX2); });
    shared_score.multiplier = any_x2 ? 2.0f : 1.0f;
    auto& pellets = maze.pellets_mut();
    auto& energizers = maze.energizers_mut();
    for (auto& ps : players) {
        if (!ps.pac.alive) continue;
        Tile pt = ps.pac.tile();
        if (pellets.erase(pt) > 0) {
            ++ps.pellets_eaten;
            ++pellets_eaten_total;
            int gained = shared_score.eat_pellet();
            ps.contribution += gained;
            if (g_audio) g_audio->play_pellet();
        }
        if (energizers.erase(pt) > 0) {
            ++ps.pellets_eaten;
            ++pellets_eaten_total;
            int gained = shared_score.eat_energizer();
            ps.contribution += gained;
            if (g_audio) g_audio->play_energizer();
            fright_left = std::max(2.0f, FRIGHT_BASE - (team_level - 1) * 0.5f);
            for (auto& g : ghosts) {
                if (g.mode == GhostMode::Scatter || g.mode == GhostMode::Chase) {
                    g.mode = GhostMode::Frightened;
                    g.reverse();
                }
            }
            shared_score.reset_streak();
        }
    }
    auto& bag = players[0].powerups;
    if (bag.on_maze) {
        auto pk = *bag.on_maze;
        for (auto& ps : players) {
            if (!ps.pac.alive) continue;
            if (std::fabs(pk.x - ps.pac.x) < 0.6f && std::fabs(pk.y - ps.pac.y) < 0.6f) {
                bag.collect();
                apply_pickup_coop(ps, pk.kind, pk.x, pk.y);
                break;
            }
        }
    }
    std::vector<PlayerState*> died;
    bool streak4 = false;
    for (auto& g : ghosts) {
        for (auto& ps : players) {
            if (!ps.pac.alive) continue;
            if (std::fabs(g.x - ps.pac.x) < 0.7f && std::fabs(g.y - ps.pac.y) < 0.7f) {
                if (g.mode == GhostMode::Frightened) {
                    int gained = shared_score.eat_ghost();
                    ps.contribution += gained;
                    ++ps.ghosts_eaten;
                    ps.longest_combo = std::max(ps.longest_combo, shared_score.ghost_streak);
                    if (shared_score.ghost_streak == 4) streak4 = true;
                    add_popup("+" + std::to_string(gained), g.x, g.y - 0.5f,
                              ps.player_id == 2 ? P2_COLOR : P1_COLOR);
                    if (g_audio) g_audio->play_ghost_eat();
                    g.mode = GhostMode::Eaten;
                    break;
                } else if (g.mode == GhostMode::Eaten) {
                    continue;
                } else if (ps.powerups.is_active(PowerUp::Phase)) {
                    continue;
                } else {
                    if (ps.pac.invuln > 0) continue;
                    died.push_back(&ps);
                }
            }
        }
    }
    if (!died.empty()) {
        for (auto* ps : died) mode->on_player_death(*this, *ps);
        // PRD §10.5: independent lives — each player loses ONE life regardless
        // of simultaneous deaths (no shared-pool refund).
        for (auto* ps : died) ps->drop_timer = std::max(ps->drop_timer, 0.0001f);
    }
    // 4-ghost combo bonus → +1 life to the player who triggered it
    // (simpler attribution: just bump P1).
    if (streak4 && players[0].lives < LIVES_CAP) ++players[0].lives;
}

void MultiGameplay::apply_pickup_solo(PlayerState& ps, PowerUp k, float, float) {
    if (k == PowerUp::Cherry) {
        ps.score.eat_fruit(ps.level);
        ++ps.fruits_eaten;
        record_fruit(ps.level);
        return;
    }
    if (k == PowerUp::Heart) {
        if (ps.lives < LIVES_CAP) ++ps.lives;
        return;
    }
    ps.powerups.apply(k);
}

void MultiGameplay::apply_pickup_coop(PlayerState& ps, PowerUp k, float, float) {
    if (k == PowerUp::Cherry) {
        int gained = shared_score.eat_fruit(team_level);
        ps.contribution += gained;
        ++ps.fruits_eaten;
        record_fruit(team_level);
        return;
    }
    if (k == PowerUp::Heart) {
        // PRD §10.5: indep lives — Heart goes to the picker
        if (ps.lives < LIVES_CAP) ++ps.lives;
        return;
    }
    if (is_global_powerup(k)) {
        for (auto& p : players) p.powerups.apply(k);
    } else {
        ps.powerups.apply(k);
    }
}

void MultiGameplay::die_solo(PlayerState& ps) {
    if (g_audio) g_audio->play_death();
    --ps.lives;
    ++ps.deaths;
    ps.pac.alive = false;
    sub = Sub::LifeLost;
    sub_timer = LIFE_LOST_T;
    mode->on_life_lost(*this, ps);
}

void MultiGameplay::after_life_lost() {
    if (swap_banner_t > 0.f) {
        sub = Sub::Play;
        return;
    }
    if (mode->is_game_over(*this)) {
        enter_game_over();
    } else {
        players[active_idx].pac.respawn(maze);
        ghosts = make_ghosts(maze);
        reset_mode_timing();
        sub = Sub::Intro;
        sub_timer = LEVEL_INTRO_T;
    }
}

void MultiGameplay::enter_level_complete() {
    if (g_audio) g_audio->play_level_clear();
    sub = Sub::LevelComplete;
    sub_timer = LEVEL_COMPLETE_T;
}

void MultiGameplay::enter_game_over() {
    sub = Sub::GameOver;
}

void MultiGameplay::next_level() {
    std::string mn = mode->name();
    if (mn == "alternating") {
        auto& ps = players[active_idx];
        ++ps.level;
        mazes[active_idx] = Maze::load(ps.level);
        maze = mazes[active_idx];
        ps.pac = Pacman::spawn(maze, ps.player_id);
        ps.powerups.reset_level();
    } else if (mn == "coop") {
        ++team_level;
        for (auto& ps : players) ps.level = team_level;
        maze = Maze::load(team_level);
        mazes = {maze};
        for (size_t i = 0; i < players.size(); ++i) {
            players[i].pac = Pacman::spawn(maze, players[i].player_id);
            if (i == 1) players[i].pac.x = std::max(0.5f, players[i].pac.x - 1.0f);
            players[i].powerups.reset_level();
        }
    } else {
        auto& ps = players[0];
        ++ps.level;
        maze = Maze::load(ps.level);
        ps.pac = Pacman::spawn(maze, ps.player_id);
        ps.powerups.reset_level();
    }
    ghosts = make_ghosts(maze);
    reset_mode_timing();
    sub = Sub::Intro;
    sub_timer = LEVEL_INTRO_T;
    pellets_eaten_total = 0;
    total_pellets_start = maze.total_pellets();
}

void MultiGameplay::update(float dt) {
    t += dt;
    app.controllers.tick(dt);
    // Tick floating popups
    for (auto& pu : popups) { pu.life -= dt; pu.y -= dt * 1.0f; }
    popups.erase(std::remove_if(popups.begin(), popups.end(),
        [](const Popup& p) { return p.life <= 0; }), popups.end());
    if (sub == Sub::Intro) {
        sub_timer -= dt;
        if (sub_timer <= 0) sub = Sub::Play;
        return;
    }
    if (sub == Sub::Paused || sub == Sub::GameOver) return;
    if (sub == Sub::LifeLost) {
        sub_timer -= dt;
        if (sub_timer <= 0) after_life_lost();
        return;
    }
    if (sub == Sub::LevelComplete) {
        sub_timer -= dt;
        if (sub_timer <= 0) next_level();
        return;
    }
    mode->update_world(*this, dt);
    if (mode->is_game_over(*this)) enter_game_over();
}

void MultiGameplay::handle_sdl_event(const SDL_Event& ev) {
    // app.controllers.handle_event already called by App::run before scene dispatch
    if (sub == Sub::GameOver) {
        if (ev.type == SDL_KEYDOWN) {
            if (ev.key.keysym.sym == SDLK_RETURN)      app.start_multi_game(mode->name());
            else if (ev.key.keysym.sym == SDLK_ESCAPE) app.go_main_menu();
        }
        // controller
        for (auto& [_pid, action] : app.router.route(ev)) {
            if (action == "confirm" || action == "pause") app.start_multi_game(mode->name());
            else if (action == "back") app.go_main_menu();
        }
        return;
    }
    if (sub == Sub::Paused) {
        if (ev.type == SDL_KEYDOWN) {
            switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: case SDLK_p:
                case SDLK_RETURN: case SDLK_SPACE:
                    sub = Sub::Play; return;
                case SDLK_BACKSPACE: case SDLK_q:
                    app.go_main_menu(); return;
                default: break;
            }
        }
        for (auto& [_pid, action] : app.router.route(ev)) {
            if (action == "pause" || action == "confirm") { sub = Sub::Play; return; }
            if (action == "back") { app.go_main_menu(); return; }
        }
        return;
    }
    auto actions = app.router.route(ev);
    for (auto& [pid, action] : actions) {
        if (action == "pause" && sub == Sub::Play) {
            sub = Sub::Paused;
            return;
        }
        if (action == "back" && (sub == Sub::Intro || sub == Sub::Play)) {
            app.go_main_menu();
            return;
        }
        mode->handle_input_action(*this, pid, action);
    }
}

// --- draw (declared in app.hpp as Scene::draw is virtual; we expose it via free function used by scene wrapper)
void MultiGameplay_draw_(SDL_Renderer* r, MultiGameplay& gp) {
    fill_rect(r, 0, 0, WIN_W, WIN_H, BG);
    draw_maze(r, gp.maze);
    draw_pellets(r, gp.maze, gp.t);
    auto& bag = gp.players[0].powerups;
    if (bag.on_maze) draw_pickup(r, *bag.on_maze, gp.t);
    {
        std::string mn_render = gp.mode->name();
        for (size_t i = 0; i < gp.players.size(); ++i) {
            auto& ps = gp.players[i];
            // Alternating: only render the currently active player. The
            // inactive one is "waiting their turn" — should not appear.
            if (mn_render == "alternating" && static_cast<int>(i) != gp.active_idx) continue;
            if (ps.pac.alive) {
                Color c = (ps.player_id == 2) ? P2_COLOR : P1_COLOR;
                draw_pacman(r, ps.pac, c);
            }
            if (ps.drop_timer > 0 && ps.drop_x >= 0) {
                Color c = (ps.player_id == 2) ? P2_COLOR : P1_COLOR;
                draw_circle(r, to_screen_x(ps.drop_x), to_screen_y(ps.drop_y), 6, c);
            }
        }
    }
    for (auto& g : gp.ghosts) draw_ghost(r, g, gp.t, gp.fright_left);
    // Floating score popups (fade with life)
    for (auto& pu : gp.popups) {
        std::uint8_t alpha = static_cast<std::uint8_t>(std::min(255.0f, pu.life * 320.0f));
        Color c = pu.color; c.a = alpha;
        draw_text(r, pu.text, to_screen_x(pu.x), to_screen_y(pu.y), 18, c, true);
    }
    std::string mn = gp.mode->name();
    if (mn == "single") draw_solo_hud(r, gp.players[0], gp.t);
    else if (mn == "alternating")
        draw_alternating_hud(r, gp.players[0], gp.players[1], gp.active_idx, gp.t);
    else
        draw_coop_hud(r, gp.players[0], gp.players[1],
                      gp.shared_score.points, gp.shared_lives, gp.team_level, gp.t);
    // PRD §8.2: fruit tray — last 7 fruits earned, right-aligned along
    // the bottom safe edge. Cherry icon for any fruit (simplified).
    {
        int icon_size = 22;
        int gap = 6;
        int total_w = static_cast<int>(gp.fruit_history.size()) * (icon_size + gap) - gap;
        int x = SAFE_X1 - 8 - total_w;
        int y = SAFE_Y1 - icon_size - 4;
        for (size_t i = 0; i < gp.fruit_history.size(); ++i) {
            int cx = x + static_cast<int>(i) * (icon_size + gap) + icon_size / 2;
            int cy = y + icon_size / 2;
            fill_circle(r, cx, cy, icon_size / 2 - 2, {240, 80, 80});      // cherry body
            fill_rect(r, cx - 1, cy - icon_size / 2, 2, 4, {120, 90, 30}); // stem
        }
    }
    if (gp.sub == Sub::Intro) {
        Color dim = BG; dim.a = 150;
        fill_rect(r, 0, 0, WIN_W, WIN_H, dim);
        draw_text(r, "LEVEL " + std::to_string(gp.players[0].level), WIN_W / 2, WIN_H / 2 - 70, 70, {255, 220, 60}, true);
        draw_text(r, "READY", WIN_W / 2, WIN_H / 2 + 30, 56, {255, 255, 255}, true);
    } else if (gp.sub == Sub::Paused) {
        Color dim = {0, 0, 0, 150};
        fill_rect(r, 0, 0, WIN_W, WIN_H, dim);
        draw_text(r, "PAUSED", WIN_W / 2, WIN_H / 2 - 80, 84, {255, 255, 255}, true);
        draw_text(r, "A RESUME    B MAIN MENU", WIN_W / 2, WIN_H / 2 + 30, 32, {200, 200, 220}, true);
    } else if (gp.sub == Sub::GameOver) {
        Color dim = {0, 0, 0, 170};
        fill_rect(r, 0, 0, WIN_W, WIN_H, dim);
        draw_text(r, "GAME OVER", WIN_W / 2, 220, 112, RED, true);
        if (mn == "coop") {
            draw_text(r, "TEAM " + std::to_string(gp.shared_score.points), WIN_W / 2, 380, 56, {255, 220, 60}, true);
            int mvp = (gp.players[0].contribution >= gp.players[1].contribution) ? 1 : 2;
            draw_text(r, "MVP P" + std::to_string(mvp), WIN_W / 2, 460, 42, (mvp == 1 ? P1_COLOR : P2_COLOR), true);
        } else if (mn == "alternating") {
            int winner = 0;
            if (gp.players[0].score.points > gp.players[1].score.points) winner = 1;
            else if (gp.players[1].score.points > gp.players[0].score.points) winner = 2;
            if (winner == 0) draw_text(r, "CO CHAMPIONS", WIN_W / 2, 380, 56, {255, 220, 60}, true);
            else draw_text(r, "WINNER P" + std::to_string(winner), WIN_W / 2, 380, 56, (winner == 1 ? P1_COLOR : P2_COLOR), true);
        } else {
            draw_text(r, "SCORE " + std::to_string(gp.players[0].score.points), WIN_W / 2, 380, 56, TEXT, true);
        }
        draw_text(r, "A REPLAY    B MAIN MENU", WIN_W / 2, 700, 32, {200, 200, 220}, true);
    } else if (gp.sub == Sub::LifeLost) {
        Color dim = {0, 0, 0, 80};
        fill_rect(r, 0, 0, WIN_W, WIN_H, dim);
        draw_text(r, "OUCH", WIN_W / 2, WIN_H / 2 - 30, 70, RED, true);
    } else if (gp.sub == Sub::LevelComplete) {
        Color dim = {0, 0, 0, 100};
        fill_rect(r, 0, 0, WIN_W, WIN_H, dim);
        draw_text(r, "LEVEL CLEAR", WIN_W / 2, WIN_H / 2 - 30, 70, {255, 220, 60}, true);
    }
    if (gp.swap_banner_t > 0.f) {
        draw_turn_swap(r, gp.swap_banner_from, gp.swap_banner_to, gp.swap_banner_t);
    }
}

// Scene wrapper so App can hold a unique_ptr<Scene>
class MultiGameplayScene : public Scene {
public:
    MultiGameplay gp;
    MultiGameplayScene(App& a, const std::string& mode_name) : gp(a, mode_name) {}
    void update(float dt) override { gp.update(dt); }
    void draw(SDL_Renderer* r) override { MultiGameplay_draw_(r, gp); }
    void handle_event(const SDL_Event& ev) override { gp.handle_sdl_event(ev); }
};

std::unique_ptr<Scene> make_multi_gameplay_scene(App& app, const std::string& mode_name) {
    return std::make_unique<MultiGameplayScene>(app, mode_name);
}

} // namespace pac
