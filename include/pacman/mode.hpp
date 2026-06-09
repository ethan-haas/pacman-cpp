// Mode strategy interface. Solo / Alternating / Coop.
#pragma once

#include "pacman_entity.hpp"
#include "powerup_manager.hpp"
#include "score.hpp"

#include <memory>
#include <string>
#include <vector>

namespace pac {

struct Ghost;
class MultiGameplay;

struct PlayerState {
    int player_id = 0;
    Pacman pac;
    Score score;                                // alternating: per-player; coop: shared ptr
    PowerUpManager powerups;
    int lives = 3;
    int level = 1;
    int pellets_eaten = 0;
    int ghosts_eaten = 0;
    int longest_combo = 0;
    float time_alive = 0.f;
    int deaths = 0;
    int fruits_eaten = 0;
    int revives = 0;
    int contribution = 0;
    float drop_x = -1.f, drop_y = -1.f;
    float drop_timer = 0.f;
};

class Mode {
public:
    virtual ~Mode() = default;
    virtual const char* name() const = 0;
    virtual bool is_multi() const = 0;

    virtual void setup(MultiGameplay& gp) = 0;
    virtual void update_world(MultiGameplay& gp, float dt) = 0;
    virtual void handle_input_action(MultiGameplay& gp, int player_id, const std::string& action) = 0;

    // Returns 0 (P1's pac) for solo. For coop returns ghost-specific target.
    virtual Pacman* ghost_target_pac(MultiGameplay& gp, const Ghost& g) = 0;
    virtual bool is_game_over(const MultiGameplay& gp) const = 0;

    // Solo + Alternating use _tick_one_player; Coop uses _tick_two_players.
    virtual void on_life_lost(MultiGameplay& gp, PlayerState& ps) {}
    virtual void on_player_death(MultiGameplay& gp, PlayerState& ps) {}
    virtual void tick_rescue(MultiGameplay& gp, float dt) {}
};

std::unique_ptr<Mode> make_mode(const std::string& name);

} // namespace pac
