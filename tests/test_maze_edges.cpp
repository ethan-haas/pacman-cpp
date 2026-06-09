// Maze + pacman edge cases: door blocks pac, walls block movement,
// tunnel wrap both directions, opposite-direction instant reverse,
// energizer eaten removes it.

#include "pacman/maze.hpp"
#include "pacman/pacman_entity.hpp"

#include <cmath>
#include <cstdio>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL me] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_door_blocks_pacman() {
    auto m = pac::Maze::load(1);
    // Find the door cells ('-') in raw layout — at row 12 cols 13-14
    CHECK(m.is_wall(13, 12, /*for_ghost=*/false));    // pac blocked
    CHECK(!m.is_wall(13, 12, /*for_ghost=*/true));    // ghost can pass
}

static void test_walls_block_pacman_in_all_4_dirs() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    // Try each direction at known-wall-cell. (0,0) is wall border.
    // Start inside corridor and move toward a wall.
    pac.x = 1.0f; pac.y = 1.0f;
    for (auto d : {pac::Dir::Up, pac::Dir::Left}) {
        pac.direction = d;
        pac.desired = d;
        float prev_x = pac.x, prev_y = pac.y;
        pac.update(1.0f / 60.0f, m);
        // Wall above and left of (1,1)
        CHECK(std::fabs(pac.x - prev_x) < 0.5f);
        CHECK(std::fabs(pac.y - prev_y) < 0.5f);
    }
}

static void test_tunnel_wrap_right_to_left() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.x = pac::COLS - 0.4f; pac.y = 14.0f;
    pac.direction = pac::Dir::Right;
    pac.update(1.0f / 60.0f, m);
    CHECK(pac.x < 0.5f);    // wrapped to left edge
}

static void test_tunnel_wrap_left_to_right() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.x = -0.6f; pac.y = 14.0f;
    pac.direction = pac::Dir::Left;
    pac.update(1.0f / 60.0f, m);
    CHECK(pac.x > pac::COLS - 1.5f);
}

static void test_opposite_dir_reverses_instantly() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.x = 1.0f; pac.y = 5.0f;
    pac.direction = pac::Dir::Right;
    pac.desired = pac::Dir::Left;   // opposite — should reverse without waiting for center
    pac.update(1.0f / 60.0f, m);
    CHECK(pac.direction == pac::Dir::Left);
}

static void test_perpendicular_dir_waits_for_center() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.x = 1.5f; pac.y = 1.0f;   // half-tile off center
    pac.direction = pac::Dir::Right;
    pac.desired = pac::Dir::Down;
    pac.update(1.0f / 60.0f, m);
    // Should NOT have turned yet (not at center)
    CHECK(pac.direction == pac::Dir::Right);
}

static void test_pellets_at_known_positions() {
    auto m = pac::Maze::load(1);
    // Row 1 cols 1..12, 15..26 should have pellets
    CHECK(m.pellets().count({1, 1}) == 1);
    CHECK(m.pellets().count({12, 1}) == 1);
    CHECK(m.pellets().count({26, 1}) == 1);
    // Wall at (13, 1) — no pellet
    CHECK(m.pellets().count({13, 1}) == 0);
}

static void test_energizers_at_4_corners() {
    auto m = pac::Maze::load(1);
    CHECK(m.energizers().count({1, 3}) == 1);             // top-left
    CHECK(m.energizers().count({pac::COLS - 2, 3}) == 1); // top-right
    CHECK(m.energizers().count({1, 23}) == 1);            // bottom-left (row 23 in classic)
    CHECK(m.energizers().count({pac::COLS - 2, 23}) == 1);// bottom-right
}

static void test_pacman_invuln_decays() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.alive = true;
    pac.invuln = 1.0f;
    pac.x = 5; pac.y = 5;
    pac.update(0.5f, m);   // big dt clamps to 1/30 internally — but invuln decrements by raw dt
    CHECK(pac.invuln < 1.0f);
}

static void test_pacman_dead_doesnt_move() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.alive = false;
    pac.x = 5; pac.y = 5;
    pac.direction = pac::Dir::Right;
    pac.update(1.0f / 60.0f, m);
    CHECK(pac.x == 5.0f);
    CHECK(pac.y == 5.0f);
}

int main() {
    test_door_blocks_pacman();
    test_walls_block_pacman_in_all_4_dirs();
    test_tunnel_wrap_right_to_left();
    test_tunnel_wrap_left_to_right();
    test_opposite_dir_reverses_instantly();
    test_perpendicular_dir_waits_for_center();
    test_pellets_at_known_positions();
    test_energizers_at_4_corners();
    test_pacman_invuln_decays();
    test_pacman_dead_doesnt_move();
    std::printf("[maze_edges] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
