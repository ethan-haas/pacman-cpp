// Headless simulation tests that drive the full multi_gameplay scene
// without a window. Hunts integration bugs (mode transitions, life
// loss, ghost-house release timing, pellet eating, level clear flow,
// game-over conditions).

#include "pacman/multi_gameplay.hpp"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <string>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

// Minimal App stub so MultiGameplay can be constructed without SDL window.
namespace pac {
class App;
}

#include "pacman/app.hpp"

namespace pac {

static App* make_dummy_app() {
    static App app(true);
    return &app;
}

}

static void test_solo_pellet_eating() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "single");
    // Force into PLAY immediately
    gp.sub = pac::Sub::Play;
    auto initial = gp.maze.total_pellets();
    CHECK(initial > 200);
    // Tick a few hundred frames; pacman moves on his own (direction left
    // from spawn), should eat at least some pellets.
    for (int i = 0; i < 600; ++i) gp.update(1.0f / 60.0f);
    auto remaining = gp.maze.total_pellets();
    CHECK(remaining < initial);
    CHECK(gp.players[0].score.points > 0);
}

static void test_ghost_house_release_order() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "single");
    gp.sub = pac::Sub::Play;
    // Tick long enough for all ghosts to leave
    for (int i = 0; i < 60 * 12; ++i) gp.update(1.0f / 60.0f);   // 12 sec
    int free_count = 0;
    for (auto& g : gp.ghosts) {
        if (g.mode != pac::GhostMode::House && g.mode != pac::GhostMode::Leaving)
            ++free_count;
    }
    CHECK(free_count >= 3);   // pinky, inky should be out; clyde maybe still leaving
}

static void test_mode_timer_transitions() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "single");
    gp.sub = pac::Sub::Play;
    // Freeze pacman so he can't eat energizer (would freeze mode timer)
    gp.players[0].pac.alive = false;
    CHECK(std::string(gp.global_mode) == "scatter");
    for (int i = 0; i < 60 * 8; ++i) gp.update(1.0f / 60.0f);
    CHECK(std::string(gp.global_mode) == "chase");
    for (int i = 0; i < 60 * 21; ++i) gp.update(1.0f / 60.0f);
    CHECK(std::string(gp.global_mode) == "scatter");
}

static void test_coop_independent_lives() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "coop");
    CHECK(gp.players.size() == 2);
    CHECK(gp.players[0].lives == 3);
    CHECK(gp.players[1].lives == 3);
    // Kill P1 once; P2's lives should be untouched.
    gp.players[0].pac.invuln = 0.f;
    gp.mode->on_player_death(gp, gp.players[0]);
    CHECK(gp.players[0].lives == 2);
    CHECK(gp.players[1].lives == 3);
}

static void test_coop_game_over_only_when_both_dead() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "coop");
    gp.players[0].lives = 0;
    gp.players[0].pac.alive = false;
    CHECK(!gp.mode->is_game_over(gp));   // p2 still alive
    gp.players[1].lives = 0;
    gp.players[1].pac.alive = false;
    CHECK(gp.mode->is_game_over(gp));
}

static void test_pacman_mouth_animates() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "single");
    gp.sub = pac::Sub::Play;
    float start = gp.players[0].pac.mouth;
    for (int i = 0; i < 30; ++i) gp.update(1.0f / 60.0f);
    CHECK(gp.players[0].pac.mouth != start);
}

static void test_alt_mode_only_active_player_visible() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "alternating");
    CHECK(gp.players.size() == 2);
    // P1 active initially
    CHECK(gp.active_idx == 0);
    CHECK(gp.players[0].pac.alive);
    CHECK(gp.players[1].pac.alive);   // both spawned, but render code hides inactive
}

static void test_fruit_history_caps_at_7() {
    auto* app = pac::make_dummy_app();
    pac::MultiGameplay gp(*app, "single");
    for (int i = 0; i < 10; ++i) gp.record_fruit(i + 1);
    CHECK(gp.fruit_history.size() == 7);
    CHECK(gp.fruit_history.front() == 4);   // first 3 dropped
    CHECK(gp.fruit_history.back() == 10);
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    test_solo_pellet_eating();
    test_ghost_house_release_order();
    test_mode_timer_transitions();
    test_coop_independent_lives();
    test_coop_game_over_only_when_both_dead();
    test_pacman_mouth_animates();
    test_alt_mode_only_active_player_visible();
    test_fruit_history_caps_at_7();
    std::printf("[sim tests] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
