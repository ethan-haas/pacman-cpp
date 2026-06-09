// Per-ghost AI targeting tests. Verifies classic Pac-Man Dossier rules:
// Blinky aims at pac's lookahead, Pinky 4 ahead, Inky vector reflection,
// Clyde scatters when close (< 8 tiles).

#include "pacman/ghost.hpp"
#include "pacman/maze.hpp"
#include "pacman/pacman_entity.hpp"

#include <cstdio>
#include <cstdlib>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL ai] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static pac::Ghost make_g(const char* name, pac::GhostMode m) {
    pac::Ghost g;
    g.name = name;
    g.mode = m;
    g.x = 10; g.y = 10;
    return g;
}

static void test_blinky_chase_lookahead() {
    auto m = pac::Maze::load(1);
    (void)m;
    pac::Pacman pac{};
    pac.x = 10; pac.y = 10; pac.direction = pac::Dir::Right;
    auto blinky = make_g("blinky", pac::GhostMode::Chase);
    auto tgt = blinky.choose_target(pac, {0, 0});
    // Blinky one-step lookahead in pac's direction
    CHECK(tgt.x == 11);
    CHECK(tgt.y == 10);
}

static void test_pinky_4_ahead() {
    pac::Pacman pac{};
    pac.x = 10; pac.y = 10; pac.direction = pac::Dir::Up;
    auto pinky = make_g("pinky", pac::GhostMode::Chase);
    auto tgt = pinky.choose_target(pac, {0, 0});
    CHECK(tgt.x == 10);
    CHECK(tgt.y == 10 - 4);
}

static void test_inky_vector_reflection() {
    pac::Pacman pac{};
    pac.x = 10; pac.y = 10; pac.direction = pac::Dir::Right;
    auto inky = make_g("inky", pac::GhostMode::Chase);
    // Inky: ahead = pac+2dir = (12,10). Then mirror through blinky.
    // tgt = ahead*2 - blinky_pos
    auto tgt = inky.choose_target(pac, {5.0f, 5.0f});
    // ahead = (12, 10), 2*ahead = (24, 20), minus blinky (5,5) = (19, 15)
    CHECK(tgt.x == 19);
    CHECK(tgt.y == 15);
}

static void test_clyde_flees_when_close() {
    pac::Pacman pac{};
    pac.x = 10; pac.y = 10;
    auto clyde = make_g("clyde", pac::GhostMode::Chase);
    clyde.x = 11; clyde.y = 11;  // within 6 tiles (dist^2 = 2)
    auto tgt = clyde.choose_target(pac, {0, 0});
    // Should be scatter target (bottom-left): {0, ROWS-1}
    CHECK(tgt.x == 0);
    CHECK(tgt.y == pac::ROWS - 1);
}

static void test_clyde_chases_when_far() {
    pac::Pacman pac{};
    pac.x = 27; pac.y = 30;
    auto clyde = make_g("clyde", pac::GhostMode::Chase);
    clyde.x = 1; clyde.y = 1;   // far away (dist^2 > 36)
    auto tgt = clyde.choose_target(pac, {0, 0});
    CHECK(tgt.x == 27);
    CHECK(tgt.y == 30);
}

static void test_scatter_targets_per_ghost() {
    auto blinky = make_g("blinky", pac::GhostMode::Scatter);
    auto pinky = make_g("pinky", pac::GhostMode::Scatter);
    auto inky = make_g("inky", pac::GhostMode::Scatter);
    auto clyde = make_g("clyde", pac::GhostMode::Scatter);
    pac::Pacman pac{};
    CHECK(blinky.choose_target(pac, {0, 0}).x == pac::COLS - 3);
    CHECK(blinky.choose_target(pac, {0, 0}).y == 0);
    CHECK(pinky.choose_target(pac, {0, 0}).x == 2);
    CHECK(pinky.choose_target(pac, {0, 0}).y == 0);
    CHECK(inky.choose_target(pac, {0, 0}).x == pac::COLS - 1);
    CHECK(inky.choose_target(pac, {0, 0}).y == pac::ROWS - 1);
    CHECK(clyde.choose_target(pac, {0, 0}).x == 0);
    CHECK(clyde.choose_target(pac, {0, 0}).y == pac::ROWS - 1);
}

static void test_eaten_ghost_targets_house() {
    auto blinky = make_g("blinky", pac::GhostMode::Eaten);
    pac::Pacman pac{};
    auto tgt = blinky.choose_target(pac, {0, 0});
    CHECK(tgt.x == 14);
    CHECK(tgt.y == 14);
}

int main() {
    test_blinky_chase_lookahead();
    test_pinky_4_ahead();
    test_inky_vector_reflection();
    test_clyde_flees_when_close();
    test_clyde_chases_when_far();
    test_scatter_targets_per_ghost();
    test_eaten_ghost_targets_house();
    std::printf("[ghost_ai] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
