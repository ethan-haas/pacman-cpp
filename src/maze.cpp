#include "pacman/maze.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <random>
#include <string_view>
#include <vector>

namespace pac {

// Classic 28×31 Pac-Man maze.
static constexpr std::array<std::string_view, ROWS> CLASSIC_MAZE = {
    "############################",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#o####.#####.##.#####.####o#",
    "#.####.#####.##.#####.####.#",
    "#..........................#",
    "#.####.##.########.##.####.#",
    "#.####.##.########.##.####.#",
    "#......##....##....##......#",
    "######.##### ## #####.######",
    "######.##### ## #####.######",
    "######.##          ##.######",
    "######.## ###--### ##.######",
    "######.## #1 23 4# ##.######",
    "T     .   #      #   .     T",
    "######.## #      # ##.######",
    "######.## ######## ##.######",
    "######.##          ##.######",
    "######.## ######## ##.######",
    "######.## ######## ##.######",
    "#............##............#",
    "#.####.#####.##.#####.####.#",
    "#.####.#####.##.#####.####.#",
    "#o..##.......PP.......##..o#",
    "###.##.##.########.##.##.###",
    "###.##.##.########.##.##.###",
    "#......##....##....##......#",
    "#.##########.##.##########.#",
    "#.##########.##.##########.#",
    "#..........................#",
    "############################",
};

static char gen_cell_(int x, int y) {
    return CLASSIC_MAZE[y][x];
}


static const char* ghost_name_for_(char c) {
    switch (c) {
        case '1': return "blinky";
        case '2': return "pinky";
        case '3': return "inky";
        case '4': return "clyde";
        default:  return nullptr;
    }
}

Maze Maze::load(int level) {
    Maze m;
    int spawn_slot = 0;
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            char ch = gen_cell_(x, y);
            char cell = ' ';
            if (ch == '.') {
                m.pellets_.insert({x, y});
            } else if (ch == 'o') {
                m.energizers_.insert({x, y});
            } else if (ch == 'P') {
                // pac spawn marker (no pellet)
            } else if (const char* gn = ghost_name_for_(ch)) {
                if (spawn_slot < 4) {
                    m.ghost_spawns_[spawn_slot++] =
                        {gn, {static_cast<float>(x), static_cast<float>(y)}};
                }
            } else if (ch == 'T') {
                m.tunnel_rows_.insert(y);
            } else {
                cell = ch;
            }
            m.grid_[y][x] = cell;
        }
    }
    m.pac_spawn_ = {13.5f, 23.0f};
    for (int dx : {-1, 0, 1, 2})
        m.pellets_.erase({13 + dx, 23});
    if (level >= 2) m.apply_variation_(level);
    return m;
}

char Maze::at(int x, int y) const {
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return '#';
    return grid_[y][x];
}

bool Maze::is_wall(int x, int y, bool for_ghost) const {
    char c = at(x, y);
    if (c == '#') return true;
    if (c == '-') return !for_ghost;
    return false;
}

bool Maze::walkable(int x, int y, bool for_ghost) const {
    return !is_wall(x, y, for_ghost);
}

std::pair<float, float> Maze::ghost_spawn(const std::string& name) const {
    for (auto& [n, pos] : ghost_spawns_) {
        if (n == name) return pos;
    }
    return {13.5f, 14.0f};
}

// Deterministic mirror-symmetric maze variation per level.
// Port of _vary() — connectivity-preserving wall toggles.
void Maze::apply_variation_(int level) {
    std::mt19937 rng(static_cast<std::uint32_t>(0xBEE5u ^ (level * 31337u)));
    auto protected_tile = [](int x, int y) -> bool {
        if (y >= 12 && y <= 16) return true;
        if (x <= 0 || x >= COLS - 1 || y <= 0 || y >= ROWS - 1) return true;
        return false;
    };

    std::vector<Tile> candidates;
    for (int y = 1; y < ROWS - 1; ++y) {
        for (int x = 2; x < COLS / 2; ++x) {
            if (protected_tile(x, y)) continue;
            candidates.push_back({x, y});
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), rng);

    auto connectivity_ok = [&]() -> bool {
        // BFS from (1,1)
        std::array<std::array<bool, COLS>, ROWS> seen{};
        std::queue<Tile> q;
        q.push({1, 1});
        seen[1][1] = true;
        int count = 0;
        while (!q.empty()) {
            Tile t = q.front(); q.pop();
            ++count;
            const std::array<std::pair<int, int>, 4> ds = {{{1,0},{-1,0},{0,1},{0,-1}}};
            for (auto [dx, dy] : ds) {
                int nx = t.x + dx, ny = t.y + dy;
                if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) continue;
                if (seen[ny][nx]) continue;
                if (grid_[ny][nx] == '#') continue;
                seen[ny][nx] = true;
                q.push({nx, ny});
            }
        }
        // verify every non-wall tile got visited
        for (int y = 0; y < ROWS; ++y)
            for (int x = 0; x < COLS; ++x)
                if (grid_[y][x] != '#' && !seen[y][x])
                    return false;
        (void)count;
        return true;
    };

    int flips_target = 8 + (level % 5) * 2;
    int flips_done = 0;
    for (auto t : candidates) {
        if (flips_done >= flips_target) break;
        int x = t.x, y = t.y;
        int mx = COLS - 1 - x;
        char old_l = grid_[y][x];
        char old_r = grid_[y][mx];
        bool had_e_l = energizers_.count({x, y}) > 0;
        bool had_e_r = energizers_.count({mx, y}) > 0;
        if (old_l == '#' && old_r == '#') {
            grid_[y][x] = ' ';
            grid_[y][mx] = ' ';
            if (connectivity_ok()) {
                pellets_.insert({x, y});
                pellets_.insert({mx, y});
                ++flips_done;
            } else {
                grid_[y][x] = old_l;
                grid_[y][mx] = old_r;
            }
        } else if (old_l != '#' && old_r != '#' && !had_e_l && !had_e_r) {
            bool had_p_l = pellets_.count({x, y}) > 0;
            bool had_p_r = pellets_.count({mx, y}) > 0;
            grid_[y][x] = '#';
            grid_[y][mx] = '#';
            if (connectivity_ok()) {
                pellets_.erase({x, y});
                pellets_.erase({mx, y});
                ++flips_done;
            } else {
                grid_[y][x] = old_l;
                grid_[y][mx] = old_r;
                if (had_p_l) pellets_.insert({x, y});
                if (had_p_r) pellets_.insert({mx, y});
            }
        }
    }
    // clear pellets at pac spawn
    pellets_.erase({static_cast<int>(pac_spawn_.first), static_cast<int>(pac_spawn_.second)});
    pellets_.erase({static_cast<int>(pac_spawn_.first) + 1, static_cast<int>(pac_spawn_.second)});
}

} // namespace pac
