#include "pacman/ghost.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>

namespace pac {

static thread_local std::mt19937 rng_{0xC0FFEE};

float Ghost::speed() const {
    switch (mode) {
        case GhostMode::Frightened: return GHOST_FRIGHT;
        case GhostMode::Eaten:      return GHOST_EATEN;
        case GhostMode::House:      return GHOST_FRIGHT;
        default:                    return GHOST_SPEED;
    }
}

Tile Ghost::scatter_target() const {
    if (name == "blinky") return {SCATTER_BLINKY.x, SCATTER_BLINKY.y};
    if (name == "pinky")  return {SCATTER_PINKY.x,  SCATTER_PINKY.y};
    if (name == "inky")   return {SCATTER_INKY.x,   SCATTER_INKY.y};
    if (name == "clyde")  return {SCATTER_CLYDE.x,  SCATTER_CLYDE.y};
    return {0, 0};
}

Tile Ghost::choose_target(const Pacman& pac, std::pair<float, float> blinky_pos) const {
    if (mode == GhostMode::Scatter) return scatter_target();
    if (mode == GhostMode::Eaten)   return {COLS / 2, 14};
    if (mode == GhostMode::House || mode == GhostMode::Leaving) return {COLS / 2, 11};
    // chase
    Tile pt = pac.tile();
    int pdx = dx_of(pac.direction), pdy = dy_of(pac.direction);
    if (name == "blinky") return {pt.x + pdx, pt.y + pdy};
    if (name == "pinky")  return {pt.x + 4 * pdx, pt.y + 4 * pdy};
    if (name == "inky") {
        Tile ahead{pt.x + 2 * pdx, pt.y + 2 * pdy};
        int bx = static_cast<int>(blinky_pos.first);
        int by = static_cast<int>(blinky_pos.second);
        return {ahead.x * 2 - bx, ahead.y * 2 - by};
    }
    if (name == "clyde") {
        float dxp = x - pt.x, dyp = y - pt.y;
        float d2 = dxp * dxp + dyp * dyp;
        if (d2 > 36.f) return pt;
        return scatter_target();
    }
    return pt;
}

static std::vector<Dir> candidates_(const Ghost& g, const Maze& m) {
    Tile t = g.tile();
    std::vector<Dir> out;
    const std::array<Dir, 4> all = {Dir::Up, Dir::Down, Dir::Left, Dir::Right};
    auto valid = [&](Dir d) {
        int nx = t.x + dx_of(d), ny = t.y + dy_of(d);
        if (!m.walkable(nx, ny, /*for_ghost=*/true)) return false;
        if (m.at(nx, ny) == '-' && g.mode != GhostMode::Eaten && g.mode != GhostMode::Leaving) return false;
        return true;
    };
    for (Dir d : all) {
        if (!valid(d)) continue;
        if (g.direction != Dir::None && d == opposite(g.direction)) continue;
        out.push_back(d);
    }
    if (out.empty()) {
        for (Dir d : all) if (valid(d)) out.push_back(d);
    }
    return out;
}

static Dir pick_direction_(Ghost& g, const Maze& m, const Pacman& pac, std::pair<float, float> blinky_pos) {
    auto cands = candidates_(g, m);
    if (cands.empty()) return g.direction;
    if (g.mode == GhostMode::Frightened) {
        std::uniform_int_distribution<size_t> dist(0, cands.size() - 1);
        return cands[dist(rng_)];
    }
    Tile tgt = g.choose_target(pac, blinky_pos);
    g.target = tgt;
    // tie-break: up, left, down, right
    auto pri = [](Dir d) {
        switch (d) {
            case Dir::Up: return 0;
            case Dir::Left: return 1;
            case Dir::Down: return 2;
            case Dir::Right: return 3;
            default: return 99;
        }
    };
    Dir best = cands[0];
    long long best_d2 = (1LL << 60);
    int best_pri = 99;
    Tile t = g.tile();
    for (Dir d : cands) {
        int nx = t.x + dx_of(d), ny = t.y + dy_of(d);
        long long d2 = (long long)(nx - tgt.x) * (nx - tgt.x) + (long long)(ny - tgt.y) * (ny - tgt.y);
        int pp = pri(d);
        if (d2 < best_d2 || (d2 == best_d2 && pp < best_pri)) {
            best_d2 = d2;
            best_pri = pp;
            best = d;
        }
    }
    return best;
}

void Ghost::reverse() {
    if (direction != Dir::None) direction = opposite(direction);
    last_tile = {-1, -1};
}

void Ghost::update(float dt, const Maze& m, const Pacman& pac,
                   std::pair<float, float> blinky_pos, const char* global_mode, bool frozen) {
    if (frozen && mode != GhostMode::Eaten && mode != GhostMode::House && mode != GhostMode::Leaving)
        return;
    blink_t += dt;
    if (mode == GhostMode::House) {
        house_timer -= dt;
        float cy = std::round(y);
        if (y >= cy + 0.4f) direction = Dir::Up;
        else if (y <= cy - 0.4f) direction = Dir::Down;
        y += dy_of(direction) * 1.5f * dt;
        if (house_timer <= 0) {
            mode = GhostMode::Leaving;
            direction = Dir::Up;
        }
        return;
    }
    if (mode == GhostMode::Leaving) {
        float tx = (COLS / 2.0f - 0.5f);
        if (std::fabs(x - tx) > 0.05f) {
            x += 3.0f * dt * (tx > x ? 1.f : -1.f);
            if (y < 13.0f) y += 2.0f * dt;
            else if (y > 13.0f) y -= 2.0f * dt;
        } else {
            x = tx;
            y -= speed() * dt;
            if (y <= 11.0f) {
                y = 11.0f;
                std::string gm = global_mode ? global_mode : "scatter";
                if (gm == "chase") mode = GhostMode::Chase;
                else mode = GhostMode::Scatter;
                direction = Dir::Left;
            }
        }
        return;
    }
    if (mode == GhostMode::Eaten) {
        float cx = (COLS / 2.0f - 0.5f), cy = 14.0f;
        if (std::fabs(x - cx) < 0.2f && std::fabs(y - cy) < 0.2f) {
            // Classic Pacman behaviour: ghost regenerates in the house
            // for a beat before exiting. Sit in House mode with timer.
            mode = GhostMode::House;
            house_timer = 2.5f;
            direction = Dir::Up;
            return;
        }
    }
    // Wider centring window so a ghost moving at ~5.7 tiles/sec doesn't
    // skip the tile-center check between frames and miss its turn.
    Tile cur = tile();
    if (centered(0.22f) && cur != last_tile) {
        x = std::round(x);
        y = std::round(y);
        Dir chosen = pick_direction_(*this, m, pac, blinky_pos);
        if (chosen != Dir::None) direction = chosen;
        last_tile = cur;
    }
    int dxv = dx_of(direction), dyv = dy_of(direction);
    float spd = speed();
    Tile t = tile();
    bool on_tunnel = m.tunnel_row(t.y);
    bool perp_aligned = (direction == Dir::Up || direction == Dir::Down)
        ? std::fabs(x - std::round(x)) < 0.12f
        : std::fabs(y - std::round(y)) < 0.12f;
    bool ahead_open = perp_aligned && m.walkable(t.x + dxv, t.y + dyv, /*for_ghost=*/true);
    if (ahead_open && m.at(t.x + dxv, t.y + dyv) == '-'
        && mode != GhostMode::Eaten && mode != GhostMode::Leaving) {
        ahead_open = false;
    }
    if (on_tunnel && (direction == Dir::Left || direction == Dir::Right)
        && (t.x + dxv < 0 || t.x + dxv >= COLS)) {
        ahead_open = true;
    }
    if (!ahead_open) {
        if (direction == Dir::Left || direction == Dir::Right) x = std::round(x);
        else y = std::round(y);
    } else {
        x += dxv * spd * dt;
        y += dyv * spd * dt;
    }
    if (on_tunnel) {
        if (x < -0.5f) x = COLS - 0.5f;
        else if (x > COLS - 0.5f) x = -0.5f;
    }
}

std::vector<Ghost> make_ghosts(const Maze& m) {
    struct Spec { const char* name; Color color; float delay; };
    const std::array<Spec, 4> specs = {{
        {"blinky", RED,    0.0f},
        {"pinky",  PINK,   2.0f},
        {"inky",   CYAN,   5.0f},
        {"clyde",  ORANGE, 9.0f},
    }};
    std::vector<Ghost> out;
    out.reserve(4);
    for (auto& s : specs) {
        auto [sx, sy] = m.ghost_spawn(s.name);
        Ghost g;
        g.name = s.name;
        g.color = s.color;
        g.x = sx;
        g.y = sy;
        g.house_timer = s.delay;
        g.mode = (std::string(s.name) == "blinky") ? GhostMode::Scatter : GhostMode::House;
        if (g.mode == GhostMode::Scatter) {
            g.x = (COLS / 2.0f - 0.5f); g.y = 11.0f; g.direction = Dir::Left;
        }
        out.push_back(std::move(g));
    }
    return out;
}

} // namespace pac
