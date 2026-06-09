#include "pacman/mode.hpp"
#include "pacman/multi_gameplay.hpp"

#include <algorithm>

namespace pac {

class AlternatingMode : public Mode {
public:
    const char* name() const override { return "alternating"; }
    bool is_multi() const override { return true; }

    void setup(MultiGameplay& gp) override {
        gp.mazes.push_back(Maze::load(1));
        gp.mazes.push_back(Maze::load(1));
        for (int i = 0; i < 2; ++i) {
            PlayerState ps;
            ps.player_id = i + 1;
            ps.pac = Pacman::spawn(gp.mazes[i], i + 1);
            ps.lives = DEFAULT_LIVES;
            ps.powerups.reset_level();
            gp.players.push_back(std::move(ps));
        }
        gp.active_idx = 0;
        gp.maze = gp.mazes[0];   // copy bind
        gp.ghosts = make_ghosts(gp.maze);
    }

    void update_world(MultiGameplay& gp, float dt) override {
        if (gp.swap_banner_t > 0.f) {
            gp.swap_banner_t = std::max(0.f, gp.swap_banner_t - dt);
            if (gp.swap_banner_t == 0.f) {
                gp.active_idx = gp.next_alive_idx();
                gp.maze = gp.mazes[gp.active_idx];
                gp.ghosts = make_ghosts(gp.maze);
                gp.reset_mode_timing();
                gp.sub = Sub::Intro;
                gp.sub_timer = 1.6f;
                gp.players[gp.active_idx].pac.respawn(gp.maze);
            }
            return;
        }
        gp.tick_one_player(dt, gp.players[gp.active_idx]);
    }

    void handle_input_action(MultiGameplay& gp, int pid, const std::string& a) override {
        if (gp.swap_banner_t > 0.f) return;
        auto& ps = gp.players[gp.active_idx];
        if (pid != ps.player_id) return;
        if (a == "up")    ps.pac.request(Dir::Up);
        else if (a == "down")  ps.pac.request(Dir::Down);
        else if (a == "left")  ps.pac.request(Dir::Left);
        else if (a == "right") ps.pac.request(Dir::Right);
    }

    Pacman* ghost_target_pac(MultiGameplay& gp, const Ghost&) override {
        return &gp.players[gp.active_idx].pac;
    }

    void on_life_lost(MultiGameplay& gp, PlayerState& ps) override {
        auto& other = gp.players[1 - gp.active_idx];
        if (other.lives > 0) {
            gp.swap_banner_from = ps.player_id;
            gp.swap_banner_to   = other.player_id;
            gp.swap_banner_t    = SWAP_INTERSTITIAL_T;
        }
    }

    bool is_game_over(const MultiGameplay& gp) const override {
        return gp.players[0].lives <= 0 && gp.players[1].lives <= 0;
    }
};

std::unique_ptr<Mode> make_alternating_mode_() { return std::make_unique<AlternatingMode>(); }

} // namespace pac
