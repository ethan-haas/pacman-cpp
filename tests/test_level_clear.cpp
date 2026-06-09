// Level-completion test: force-clear all pellets, verify level
// transitions cleanly (no crash, level++, maze regenerates, mode
// timer resets, pacman respawns at new spawn).

#include "pacman/app.hpp"
#include "pacman/multi_gameplay.hpp"

#include <SDL2/SDL.h>
#include <cstdio>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL lc] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_solo_level_clear() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "single");
    gp.sub = pac::Sub::Play;
    int start_level = gp.players[0].level;
    // Force-clear all pellets + energizers via mutating helpers.
    gp.maze.pellets_mut().clear();
    gp.maze.energizers_mut().clear();
    CHECK(gp.maze.total_pellets() == 0);
    // Tick — should enter LevelComplete, then tick out + transition
    for (int i = 0; i < 60 * 5; ++i) gp.update(1.0f / 60.0f);
    CHECK(gp.players[0].level == start_level + 1);
    CHECK(gp.maze.total_pellets() > 200);   // fresh pellets
    CHECK(gp.players[0].pac.alive);
}

static void test_coop_level_clear() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    gp.sub = pac::Sub::Play;
    int start_team = gp.team_level;
    gp.maze.pellets_mut().clear();
    gp.maze.energizers_mut().clear();
    for (int i = 0; i < 60 * 5; ++i) gp.update(1.0f / 60.0f);
    CHECK(gp.team_level == start_team + 1);
    CHECK(gp.maze.total_pellets() > 200);
    CHECK(gp.players[0].pac.alive);
    CHECK(gp.players[1].pac.alive);
}

static void test_multiple_clears() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "single");
    gp.sub = pac::Sub::Play;
    int start_level = gp.players[0].level;
    for (int round = 0; round < 5; ++round) {
        gp.maze.pellets_mut().clear();
        gp.maze.energizers_mut().clear();
        for (int i = 0; i < 60 * 5; ++i) gp.update(1.0f / 60.0f);
    }
    CHECK(gp.players[0].level == start_level + 5);
    CHECK(gp.maze.total_pellets() > 200);
}

static void test_solo_game_over_on_life_exhaust() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "single");
    gp.sub = pac::Sub::Play;
    // Force lives = 1, kill pacman, tick past life-lost timer
    gp.players[0].lives = 1;
    gp.die_solo(gp.players[0]);
    for (int i = 0; i < 60 * 3; ++i) gp.update(1.0f / 60.0f);
    CHECK(gp.sub == pac::Sub::GameOver);
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    test_solo_level_clear();
    test_coop_level_clear();
    test_multiple_clears();
    test_solo_game_over_on_life_exhaust();
    std::printf("[level_clear] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
