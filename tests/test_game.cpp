// Headless game-logic tests. No SDL window, no graphics. Validates
// pure game state: maze load, ghost AI targeting, score thresholds,
// power-up timers, mode setup.
//
// Build: included as a separate CMake target (pacman-tests). Run via
// `./build-arm64/pacman-tests` (returns exit 0 on pass, !=0 on fail).

#include "pacman/constants.hpp"
#include "pacman/ghost.hpp"
#include "pacman/maze.hpp"
#include "pacman/pacman_entity.hpp"
#include "pacman/powerup_manager.hpp"
#include "pacman/score.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond) do {                                                   \
    if (cond) { ++g_pass; }                                                \
    else { ++g_fail; std::fprintf(stderr,                                  \
        "[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #cond); }                \
} while (0)

#define CHECK_EQ(a, b) do {                                                \
    auto _a = (a); auto _b = (b);                                          \
    if (_a == _b) { ++g_pass; }                                            \
    else { ++g_fail; std::fprintf(stderr,                                  \
        "[FAIL] %s:%d  %s == %s (got vs expected)\n",                      \
        __FILE__, __LINE__, #a, #b); }                                     \
} while (0)

// ----------------------------------------------------------------------

static void test_maze_load() {
    auto m = pac::Maze::load(1);
    // 40×31 extended — bigger pellet count
    CHECK(m.pellets().size() > 200);
    CHECK(m.pellets().size() < 350);
    CHECK_EQ(m.energizers().size(), 4u);
    auto [sx, sy] = m.pac_spawn();
    CHECK(sx > 13.0f && sx < 14.5f);
    CHECK_EQ(sy, 23.0f);
    CHECK(m.tunnel_row(14));
    // Outer border is all wall
    for (int x = 0; x < pac::COLS; ++x) {
        CHECK(m.is_wall(x, 0));
        CHECK(m.is_wall(x, pac::ROWS - 1));
    }
}

static void test_ghost_house_release() {
    auto m = pac::Maze::load(1);
    auto ghosts = pac::make_ghosts(m);
    CHECK_EQ(ghosts.size(), 4u);
    // Blinky starts outside house in Scatter
    CHECK_EQ(ghosts[0].name, std::string("blinky"));
    CHECK(ghosts[0].mode == pac::GhostMode::Scatter);
    // Pinky / Inky / Clyde start in House
    CHECK(ghosts[1].mode == pac::GhostMode::House);
    CHECK(ghosts[2].mode == pac::GhostMode::House);
    CHECK(ghosts[3].mode == pac::GhostMode::House);
}

static void test_ghost_blinky_chases() {
    auto m = pac::Maze::load(1);
    auto ghosts = pac::make_ghosts(m);
    pac::Pacman pac{};
    pac.x = 5.0f; pac.y = 5.0f;
    pac.direction = pac::Dir::Right;
    // Force blinky into chase mode
    auto& blinky = ghosts[0];
    blinky.mode = pac::GhostMode::Chase;
    auto tgt = blinky.choose_target(pac, {blinky.x, blinky.y});
    // Blinky targets pac's next-tile lookahead (one tile in pac's direction)
    CHECK(tgt.x >= 5 && tgt.x <= 7);
    CHECK(tgt.y == 5);
}

static void test_score_thresholds() {
    pac::Score s;
    int lives = 3;
    // No extra life until 10k
    s.points = 9999;
    CHECK(!s.check_extra_life(lives));
    CHECK_EQ(lives, 3);
    // Cross 10k → +1 life
    s.points = 10000;
    CHECK(s.check_extra_life(lives));
    CHECK_EQ(lives, 4);
    // Next at 60k (10k + 50k)
    s.points = 59999;
    CHECK(!s.check_extra_life(lives));
    s.points = 60000;
    CHECK(s.check_extra_life(lives));
    CHECK_EQ(lives, 5);
}

static void test_score_ghost_streak() {
    pac::Score s;
    int g1 = s.eat_ghost();
    int g2 = s.eat_ghost();
    int g3 = s.eat_ghost();
    int g4 = s.eat_ghost();
    CHECK_EQ(g1, 200);
    CHECK_EQ(g2, 400);
    CHECK_EQ(g3, 800);
    CHECK_EQ(g4, 1600);
    CHECK_EQ(s.points, 200 + 400 + 800 + 1600);
}

static void test_powerup_apply_and_decay() {
    pac::PowerUpManager pum;
    pum.reset_level();
    pum.apply(pac::PowerUp::Speed);
    CHECK(pum.is_active(pac::PowerUp::Speed));
    // Tick beyond duration
    for (int i = 0; i < 200; ++i) pum.update(0.1f);
    CHECK(!pum.is_active(pac::PowerUp::Speed));
}

static void test_pacman_spawn() {
    auto m = pac::Maze::load(1);
    auto pac = pac::Pacman::spawn(m, 1);
    CHECK_EQ(pac.player_id, 1);
    CHECK(pac.alive);
    auto [sx, sy] = m.pac_spawn();
    CHECK_EQ(pac.x, sx);
    CHECK_EQ(pac.y, sy);
    CHECK(pac.direction == pac::Dir::Left);
}

static void test_pacman_blocked_by_wall() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    // Spawn in corridor, try to move into a wall
    pac.x = 1.0f; pac.y = 1.0f;
    pac.direction = pac::Dir::Up;       // row 0 is all wall — can't go up
    pac.desired = pac::Dir::Up;
    pac.update(1.0f / 60.0f, m);
    CHECK_EQ(static_cast<int>(std::round(pac.y)), 1);   // didn't move into wall
}

static void test_tunnel_wrap() {
    auto m = pac::Maze::load(1);
    pac::Pacman pac{};
    pac.x = -0.6f; pac.y = 14.0f;
    pac.direction = pac::Dir::Left;
    pac.update(1.0f / 60.0f, m);
    CHECK(pac.x > pac::COLS - 2);     // wrapped to right edge
}

// ----------------------------------------------------------------------

int main() {
    test_maze_load();
    test_ghost_house_release();
    test_ghost_blinky_chases();
    test_score_thresholds();
    test_score_ghost_streak();
    test_powerup_apply_and_decay();
    test_pacman_spawn();
    test_pacman_blocked_by_wall();
    test_tunnel_wrap();
    std::printf("[tests] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
