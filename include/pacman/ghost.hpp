// Ghost: 4 personalities sharing one update routine.
#pragma once

#include "constants.hpp"
#include "maze.hpp"
#include "pacman_entity.hpp"

#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace pac {

enum class GhostMode : std::uint8_t {
    House, Leaving, Scatter, Chase, Frightened, Eaten
};

struct Ghost {
    std::string name;
    Color color{255, 0, 0};
    float x = 0.f, y = 0.f;
    Dir direction = Dir::Up;
    GhostMode mode = GhostMode::House;
    float house_timer = 0.f;
    float blink_t = 0.f;
    Tile target{0, 0};
    Tile last_tile{-1, -1};

    Tile tile() const {
        return {static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y))};
    }
    bool centered(float eps = 0.08f) const {
        return std::fabs(x - std::round(x)) < eps && std::fabs(y - std::round(y)) < eps;
    }
    float speed() const;
    Tile scatter_target() const;
    Tile choose_target(const Pacman& pac, std::pair<float, float> blinky_pos) const;
    void reverse();
    void update(float dt, const Maze& m, const Pacman& pac,
                std::pair<float, float> blinky_pos, const char* global_mode,
                bool frozen = false);
};

std::vector<Ghost> make_ghosts(const Maze& m);

} // namespace pac
