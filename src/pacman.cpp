#include "pacman/pacman_entity.hpp"

#include <algorithm>
#include <cmath>

namespace pac {

Pacman Pacman::spawn(const Maze& m, int player_id) {
    Pacman p;
    auto [sx, sy] = m.pac_spawn();
    p.x = sx; p.y = sy;
    p.direction = Dir::Left;
    p.desired = Dir::Left;
    p.player_id = player_id;
    return p;
}

void Pacman::respawn(const Maze& m) {
    auto [sx, sy] = m.pac_spawn();
    x = sx; y = sy;
    direction = Dir::Left;
    desired = Dir::Left;
    alive = true;
    invuln = 1.2f;
    speed_mult = 1.0f;
}

static bool can_move_(const Pacman& p, const Maze& m, Dir d) {
    int tx = static_cast<int>(std::lround(p.x));
    int ty = static_cast<int>(std::lround(p.y));
    return m.walkable(tx + dx_of(d), ty + dy_of(d), /*for_ghost=*/false);
}

void Pacman::update(float dt, const Maze& m) {
    if (!alive) return;
    invuln = std::max(0.0f, invuln - dt);
    float speed = PAC_SPEED * speed_mult;

    // desired turn near center
    if (desired != Dir::None && desired != direction) {
        if (desired == opposite(direction)) {
            direction = desired;
        } else if (centered(0.18f) && can_move_(*this, m, desired)) {
            x = std::round(x);
            y = std::round(y);
            direction = desired;
        }
    }

    if (direction != Dir::None) {
        int dx = dx_of(direction), dy = dy_of(direction);
        int tx = static_cast<int>(std::lround(x));
        int ty = static_cast<int>(std::lround(y));
        bool on_tunnel = m.tunnel_row(ty);
        bool perp_aligned;
        if (direction == Dir::Up || direction == Dir::Down) {
            perp_aligned = std::fabs(x - std::round(x)) < 0.12f;
        } else {
            perp_aligned = std::fabs(y - std::round(y)) < 0.12f;
        }
        bool ahead_open = perp_aligned && m.walkable(tx + dx, ty + dy, false);
        if (on_tunnel && (direction == Dir::Left || direction == Dir::Right)
            && (tx + dx < 0 || tx + dx >= COLS)) {
            ahead_open = true;
        }
        if (!ahead_open) {
            if (direction == Dir::Left || direction == Dir::Right) x = std::round(x);
            else y = std::round(y);
        } else {
            x += dx * speed * dt;
            y += dy * speed * dt;
        }
        if (on_tunnel) {
            if (x < -0.5f)         x = COLS - 0.5f;
            else if (x > COLS - 0.5f) x = -0.5f;
        }
        // avoid fmodf@GLIBC_2.38: do the mod with double + floor
        double m_phase = static_cast<double>(mouth) + dt * 8.0;
        m_phase -= std::floor(m_phase / 6.2831853) * 6.2831853;
        mouth = static_cast<float>(m_phase);
    }
}

} // namespace pac
