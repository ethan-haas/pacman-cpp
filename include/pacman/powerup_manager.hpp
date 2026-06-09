#pragma once
#include "constants.hpp"
#include "maze.hpp"

#include <unordered_map>
#include <optional>

namespace pac {

struct Pickup {
    PowerUp kind = PowerUp::None;
    float x = 0.f, y = 0.f;
    float lifetime = 9.0f;
    float age = 0.f;
};

class PowerUpManager {
public:
    std::unordered_map<PowerUp, float> active;
    std::optional<Pickup> on_maze;
    float spawn_cooldown = 12.0f;
    float level_elapsed = 0.0f;
    bool heart_spawned = false;

    void reset_level();
    void maybe_spawn(const Maze& m, int pellets_eaten, int total_pellets,
                     float dt, float pac_x, float pac_y);
    void update(float dt);
    std::optional<Pickup> collect() {
        auto p = on_maze;
        on_maze.reset();
        return p;
    }
    void apply(PowerUp p) {
        float d = powerup_duration(p);
        if (d > 0) active[p] = d;
    }
    bool is_active(PowerUp p) const { return active.find(p) != active.end(); }
    float time_left(PowerUp p) const {
        auto it = active.find(p);
        return it == active.end() ? 0.f : it->second;
    }
};

} // namespace pac
