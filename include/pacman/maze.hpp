// 28×31 grid + pellet/energizer sets + per-level procedural variation.
// Port of pacman_reimagined/game/maze.py.
#pragma once

#include "constants.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>

namespace pac {

struct Tile { int x, y; };
inline bool operator==(Tile a, Tile b) { return a.x == b.x && a.y == b.y; }
inline bool operator!=(Tile a, Tile b) { return !(a == b); }

struct TileHash {
    std::size_t operator()(Tile t) const noexcept {
        return static_cast<std::size_t>(t.x) * 73856093u
             ^ static_cast<std::size_t>(t.y) * 19349663u;
    }
};

using TileSet = std::unordered_set<Tile, TileHash>;

class Maze {
public:
    static Maze load(int level = 1);

    char at(int x, int y) const;
    bool is_wall(int x, int y, bool for_ghost = false) const;
    bool walkable(int x, int y, bool for_ghost = false) const;
    int total_pellets() const { return static_cast<int>(pellets_.size() + energizers_.size()); }

    const TileSet& pellets() const { return pellets_; }
    const TileSet& energizers() const { return energizers_; }
    TileSet& pellets_mut() { return pellets_; }
    TileSet& energizers_mut() { return energizers_; }
    bool tunnel_row(int y) const { return tunnel_rows_.count(y) > 0; }

    std::pair<float, float> pac_spawn() const { return pac_spawn_; }
    std::pair<float, float> ghost_spawn(const std::string& name) const;

private:
    std::array<std::array<char, COLS>, ROWS> grid_{};
    TileSet pellets_;
    TileSet energizers_;
    std::unordered_set<int> tunnel_rows_;
    std::pair<float, float> pac_spawn_ {13.5f, 23.0f};
    // ghost name → (x, y)
    std::array<std::pair<std::string, std::pair<float, float>>, 4> ghost_spawns_{};
    void apply_variation_(int level);
};

} // namespace pac
