#include "pacman/mode.hpp"
#include "pacman/multi_gameplay.hpp"

namespace pac {

class SoloMode : public Mode {
public:
    const char* name() const override { return "single"; }
    bool is_multi() const override { return false; }

    void setup(MultiGameplay& gp) override {
        PlayerState ps;
        ps.player_id = 1;
        ps.pac = Pacman::spawn(gp.maze, 1);
        ps.lives = DEFAULT_LIVES;
        ps.powerups.reset_level();
        gp.players.push_back(std::move(ps));
        gp.active_idx = 0;
    }

    void update_world(MultiGameplay& gp, float dt) override {
        gp.tick_one_player(dt, gp.players[0]);
    }

    void handle_input_action(MultiGameplay& gp, int /*pid*/, const std::string& a) override {
        if (a == "up")    gp.players[0].pac.request(Dir::Up);
        else if (a == "down")  gp.players[0].pac.request(Dir::Down);
        else if (a == "left")  gp.players[0].pac.request(Dir::Left);
        else if (a == "right") gp.players[0].pac.request(Dir::Right);
    }

    Pacman* ghost_target_pac(MultiGameplay& gp, const Ghost& /*g*/) override {
        return &gp.players[0].pac;
    }

    bool is_game_over(const MultiGameplay& gp) const override {
        return gp.players[0].lives <= 0;
    }
};

std::unique_ptr<Mode> make_solo_mode_() { return std::make_unique<SoloMode>(); }

} // namespace pac
