#include "pacman/powerup_manager.hpp"

#include <array>
#include <random>
#include <utility>
#include <vector>

namespace pac {

static thread_local std::mt19937 rng_{0xFEEDFACEu};

void PowerUpManager::reset_level() {
    active.clear();
    on_maze.reset();
    std::uniform_real_distribution<float> d(POWERUP_INTERVAL_LO, POWERUP_INTERVAL_HI);
    spawn_cooldown = d(rng_);
    level_elapsed = 0.0f;
    heart_spawned = false;
}

static std::pair<float, float> pick_tile_(const Maze& m, float pac_x, float pac_y) {
    std::vector<std::pair<int, int>> cands;
    for (int y = 1; y < ROWS - 1; ++y) {
        for (int x = 1; x < COLS - 1; ++x) {
            if (m.is_wall(x, y, false)) continue;
            if (y >= 13 && y <= 16 && x >= 10 && x <= 17) continue;  // ghost house
            if (m.tunnel_row(y) && (x < 6 || x > 21)) continue;
            if (m.pellets().count({x, y}) > 0) continue;
            if (m.energizers().count({x, y}) > 0) continue;
            float dx = x - pac_x, dy = y - pac_y;
            if (dx * dx + dy * dy < 36.f) continue;
            cands.push_back({x, y});
        }
    }
    if (cands.empty()) {
        for (int y = 0; y < ROWS; ++y)
            for (int x = 0; x < COLS; ++x)
                if (!m.is_wall(x, y, false) && !(y >= 13 && y <= 16 && x >= 10 && x <= 17))
                    cands.push_back({x, y});
    }
    if (cands.empty()) return {13.5f, 17.0f};
    std::uniform_int_distribution<size_t> dist(0, cands.size() - 1);
    auto p = cands[dist(rng_)];
    return {static_cast<float>(p.first), static_cast<float>(p.second)};
}

static PowerUp weighted_pick_() {
    // weights match Python POWERUP_WEIGHTS
    static const std::array<std::pair<PowerUp, int>, 6> W = {{
        {PowerUp::Cherry, 6}, {PowerUp::Speed, 4}, {PowerUp::Magnet, 4},
        {PowerUp::Freeze, 2}, {PowerUp::Phase, 2}, {PowerUp::ScoreX2, 2},
    }};
    int total = 0;
    for (const auto& kv : W) total += kv.second;
    std::uniform_int_distribution<int> dist(0, total - 1);
    int r = dist(rng_), acc = 0;
    for (const auto& kv : W) {
        acc += kv.second;
        if (r < acc) return kv.first;
    }
    return PowerUp::Cherry;
}

void PowerUpManager::maybe_spawn(const Maze& m, int pellets_eaten, int total_pellets,
                                 float dt, float pac_x, float pac_y) {
    if (on_maze.has_value()) return;
    if (level_elapsed < POWERUP_NO_SPAWN_EARLY) return;
    spawn_cooldown -= dt;
    if (!heart_spawned && total_pellets > 0
        && static_cast<float>(pellets_eaten) / total_pellets >= 0.60f) {
        auto [px, py] = pick_tile_(m, pac_x, pac_y);
        on_maze = Pickup{PowerUp::Heart, px, py, 12.0f, 0.f};
        heart_spawned = true;
        return;
    }
    if (spawn_cooldown <= 0) {
        auto [px, py] = pick_tile_(m, pac_x, pac_y);
        on_maze = Pickup{weighted_pick_(), px, py, 9.0f, 0.f};
        std::uniform_real_distribution<float> d(POWERUP_INTERVAL_LO, POWERUP_INTERVAL_HI);
        spawn_cooldown = d(rng_);
    }
}

void PowerUpManager::update(float dt) {
    level_elapsed += dt;
    std::vector<PowerUp> expired;
    for (auto& [k, v] : active) {
        v -= dt;
        if (v <= 0) expired.push_back(k);
    }
    for (auto k : expired) active.erase(k);
    if (on_maze.has_value()) {
        on_maze->age += dt;
        if (on_maze->age >= on_maze->lifetime) on_maze.reset();
    }
}

} // namespace pac
