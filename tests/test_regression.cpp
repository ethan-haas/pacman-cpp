// Regression tests for bugs the user previously hit. Each test
// codifies a specific past complaint so it can't silently come back.

#include "pacman/app.hpp"
#include "pacman/controller_manager.hpp"
#include "pacman/input_router.hpp"
#include "pacman/multi_gameplay.hpp"
#include "pacman/render.hpp"

#include <SDL2/SDL.h>
#include <cstdio>
#include <string>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL reg] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

// Regression: "DPad up/down inverted on menu" — root cause was
// JOYAXISMOTION re-handling a gamecontroller-opened device. Verify
// JOYAXIS events are skipped when the device has a gamecontroller
// mapping (not in joysticks map).
static void regr_dpad_no_double_route() {
    pac::ControllerManager cm;
    cm.p1.joystick_id = 50;
    cm.p1.connected = true;
    // Slot bound but NOT registered as raw joystick → JOYAXIS should ignore.
    cm.joysticks.clear();

    pac::InputRouter router(cm);
    SDL_Event ev{};
    ev.type = SDL_JOYAXISMOTION;
    ev.jaxis.which = 50;
    ev.jaxis.axis = 1;
    ev.jaxis.value = -32000;
    auto out = router.route(ev);
    CHECK(out.empty());   // must not fire — gamecontroller axis already handled separately
}

// Regression: "Second controller does not control P2" — coop mode
// handle_input_action must route to the matching player_id.
static void regr_coop_p2_input_routes() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    auto p2_before = gp.players[1].pac.desired;
    gp.mode->handle_input_action(gp, 2, "right");
    CHECK(gp.players[1].pac.desired == pac::Dir::Right);
    CHECK(gp.players[1].pac.desired != p2_before || p2_before == pac::Dir::Right);
}

// Regression: "Alt mode shows both pacmen instead of one at a time" —
// only the active player's pac should be rendered. We can't inspect
// pixels but we can verify that swap-on-death works.
static void regr_alt_swap_on_death() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "alternating");
    gp.sub = pac::Sub::Play;
    CHECK(gp.active_idx == 0);
    gp.players[0].lives = 1;       // make sure other player has remaining lives
    gp.players[1].lives = 3;
    gp.die_solo(gp.players[0]);
    // swap banner should now run; tick through life-lost + banner
    for (int i = 0; i < 60 * 6; ++i) gp.update(1.0f / 60.0f);
    // Either active_idx swapped to 1 OR game ended (if both died)
    CHECK(gp.active_idx == 1 || gp.mode->is_game_over(gp));
}

// Regression: "Eaten ghost exits ghost house immediately" — should
// wait 2.5s in House mode before exiting.
static void regr_eaten_ghost_waits_in_house() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "single");
    auto& blinky = gp.ghosts[0];
    blinky.mode = pac::GhostMode::Eaten;
    blinky.x = (pac::COLS / 2.0f - 0.5f);
    blinky.y = 14.0f;
    // tick once — should transition to House mode with timer
    pac::Pacman dummy{};
    dummy.x = 10; dummy.y = 10;
    blinky.update(1.0f / 60.0f, gp.maze, dummy, {blinky.x, blinky.y}, "scatter", false);
    CHECK(blinky.mode == pac::GhostMode::House);
    CHECK(blinky.house_timer > 1.0f);   // 2.5 s wait
}

// Regression: "Ghost stuck doing same loop" — direction-pick eps
// must be wide enough to catch tile crossings at GHOST_SPEED.
//
// NB: `gp.ghosts` is rebuilt on life-lost; do NOT hold references
// across update(). Index by [0] each iteration instead.
static void regr_ghost_direction_pick_eps() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "single");
    gp.sub = pac::Sub::Play;
    gp.players[0].pac.alive = false;   // prevent collision → rebuild
    gp.ghosts[0].mode = pac::GhostMode::Chase;
    float start_x = gp.ghosts[0].x, start_y = gp.ghosts[0].y;
    for (int i = 0; i < 60 * 5; ++i) gp.update(1.0f / 60.0f);
    float moved = std::sqrt((gp.ghosts[0].x - start_x) * (gp.ghosts[0].x - start_x)
                          + (gp.ghosts[0].y - start_y) * (gp.ghosts[0].y - start_y));
    CHECK(moved > 5.0f);
}

// Regression: "Pacman has no mouth / hard to see" — render uses
// SDL_RenderGeometry pie cutout. Just verify draw_pacman doesn't crash
// when called with the SDL dummy driver.
static void regr_pacman_mouth_renders() {
    SDL_Window* w = SDL_CreateWindow("test", 0, 0, 100, 100, SDL_WINDOW_HIDDEN);
    SDL_Renderer* r = SDL_CreateRenderer(w, -1, SDL_RENDERER_SOFTWARE);
    pac::Pacman p{};
    p.x = 5; p.y = 5; p.direction = pac::Dir::Right; p.alive = true;
    p.mouth = 1.0f;
    pac::draw_pacman(r, p, pac::PAC_YELLOW);   // must not crash
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(w);
    CHECK(true);
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    regr_dpad_no_double_route();
    regr_coop_p2_input_routes();
    regr_alt_swap_on_death();
    regr_eaten_ghost_waits_in_house();
    regr_ghost_direction_pick_eps();
    regr_pacman_mouth_renders();
    SDL_Quit();
    std::printf("[regression] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
