// PAC-MAN entity. Tile-snapped 4-dir movement with input buffer.
#pragma once

#include "constants.hpp"
#include "maze.hpp"

#include <cmath>

namespace pac {

struct Pacman {
    float x = 0.f, y = 0.f;
    Dir direction = Dir::None;
    Dir desired = Dir::None;
    float speed_mult = 1.0f;
    float mouth = 0.0f;
    bool alive = true;
    float invuln = 0.0f;
    int player_id = 0;   // 0 = solo, 1 = P1, 2 = P2

    static Pacman spawn(const Maze& m, int player_id = 0);

    Tile tile() const {
        return {static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y))};
    }
    bool centered(float eps = 0.05f) const {
        return std::fabs(x - std::round(x)) < eps && std::fabs(y - std::round(y)) < eps;
    }

    void update(float dt, const Maze& m);
    void request(Dir d) { desired = d; }
    void respawn(const Maze& m);
};

} // namespace pac
