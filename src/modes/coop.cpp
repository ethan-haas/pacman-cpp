#include "pacman/audio.hpp"
#include "pacman/mode.hpp"
#include "pacman/multi_gameplay.hpp"

#include <algorithm>
#include <random>
#include <unordered_set>

namespace pac {

static thread_local std::mt19937 coop_rng_{0xC00C00};

// Power-ups whose effect applies globally.
static const std::unordered_set<PowerUp> GLOBAL_POWERUPS = {
    PowerUp::Freeze, PowerUp::ScoreX2, PowerUp::Cherry,
};

class CoopMode : public Mode {
public:
    const char* name() const override { return "coop"; }
    bool is_multi() const override { return true; }

    void setup(MultiGameplay& gp) override {
        // PRD §10.5: each player has independent lives (3 each), shared maze.
        gp.shared_lives = 0;   // unused under PRD indep-lives rule
        gp.shared_score = Score{};
        gp.team_level = 1;
        gp.clyde_target_id = 1;
        gp.clyde_timer = 0.f;
        gp.rescue_revive_on = false;
        for (int i = 0; i < 2; ++i) {
            PlayerState ps;
            ps.player_id = i + 1;
            ps.pac = Pacman::spawn(gp.maze, i + 1);
            if (i == 1) ps.pac.x = std::max(0.5f, ps.pac.x - 1.0f);
            ps.lives = 3;       // PRD §10.5: 3 per player
            ps.powerups.reset_level();
            gp.players.push_back(std::move(ps));
        }
        gp.active_idx = 0;
    }

    void update_world(MultiGameplay& gp, float dt) override {
        for (auto& ps : gp.players) {
            if (ps.drop_timer > 0) {
                ps.drop_timer = std::max(0.f, ps.drop_timer - dt);
                if (ps.drop_timer == 0) { ps.drop_x = -1; ps.drop_y = -1; }
            }
        }
        gp.clyde_timer -= dt;
        if (gp.clyde_timer <= 0) {
            std::uniform_int_distribution<int> d(1, 2);
            gp.clyde_target_id = d(coop_rng_);
            gp.clyde_timer = COOP_SCATTER_CYCLE_T;
        }
        gp.tick_two_players(dt);
    }

    void handle_input_action(MultiGameplay& gp, int pid, const std::string& a) override {
        for (auto& ps : gp.players) {
            if (ps.player_id != pid) continue;
            if (a == "up")    ps.pac.request(Dir::Up);
            else if (a == "down")  ps.pac.request(Dir::Down);
            else if (a == "left")  ps.pac.request(Dir::Left);
            else if (a == "right") ps.pac.request(Dir::Right);
            else if (a == "ping") {
                ps.drop_x = ps.pac.x; ps.drop_y = ps.pac.y;
                ps.drop_timer = 2.0f;
            }
            return;
        }
    }

    Pacman* ghost_target_pac(MultiGameplay& gp, const Ghost& g) override {
        auto& p1 = gp.players[0];
        auto& p2 = gp.players[1];
        if (g.name == "blinky") return (p1.contribution <= p2.contribution) ? &p1.pac : &p2.pac;
        if (g.name == "pinky")  return &p1.pac;
        if (g.name == "inky")   return &p2.pac;
        if (g.name == "clyde")  return (gp.clyde_target_id == 1) ? &p1.pac : &p2.pac;
        return &p1.pac;
    }

    void on_player_death(MultiGameplay& gp, PlayerState& ps) override {
        if (g_audio) g_audio->play_death();
        // PRD §10.5: independent lives — decrement THIS player only.
        if (gp.rescue_revive_on) {
            ps.drop_x = ps.pac.x;
            ps.drop_y = ps.pac.y;
            ps.drop_timer = COOP_RESCUE_WINDOW;
            ps.pac.alive = false;
        } else {
            --ps.lives;
            ++ps.deaths;
            ps.pac.alive = false;
        }
    }

    void tick_rescue(MultiGameplay& gp, float dt) override {
        if (!gp.rescue_revive_on) return;
        for (size_t i = 0; i < gp.players.size(); ++i) {
            auto& ps = gp.players[i];
            if (ps.pac.alive || ps.drop_timer <= 0) continue;
            auto& partner = gp.players[1 - i];
            if (!partner.pac.alive) continue;
            float dx = partner.pac.x - ps.drop_x;
            float dy = partner.pac.y - ps.drop_y;
            if (dx * dx + dy * dy < 1.0f) {
                ++ps.revives;
                ps.drop_timer = 0.f;
                ps.pac.respawn(gp.maze);
                return;
            }
        }
        for (auto& ps : gp.players) {
            if (!ps.pac.alive && ps.drop_timer == 0 && ps.drop_x >= 0) {
                --ps.lives;
                ++ps.deaths;
                ps.drop_x = -1;
            }
        }
    }

    bool is_game_over(const MultiGameplay& gp) const override {
        // PRD: game ends only when BOTH players exhausted lives.
        return gp.players[0].lives <= 0 && !gp.players[0].pac.alive
            && gp.players[1].lives <= 0 && !gp.players[1].pac.alive;
    }
};

std::unique_ptr<Mode> make_coop_mode_() { return std::make_unique<CoopMode>(); }

bool is_global_powerup(PowerUp p) { return GLOBAL_POWERUPS.count(p) > 0; }

// Factory
std::unique_ptr<Mode> make_solo_mode_();
std::unique_ptr<Mode> make_alternating_mode_();

std::unique_ptr<Mode> make_mode(const std::string& name) {
    if (name == "alternating") return make_alternating_mode_();
    if (name == "coop")        return make_coop_mode_();
    return make_solo_mode_();
}

} // namespace pac
