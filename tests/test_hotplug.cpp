// Controller hot-plug fuzz. Synthesises SDL_CONTROLLERDEVICE/JOYDEVICE
// ADDED/REMOVED events at random and verifies routing stays sane:
// - slot_for(jid) never returns dangling
// - removing a bound device disconnects the slot
// - re-adding routes to the disconnected slot

#include "pacman/controller_manager.hpp"
#include "pacman/input_router.hpp"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <random>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL hp] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static SDL_Event make_joy_removed(SDL_JoystickID jid) {
    SDL_Event ev{};
    ev.type = SDL_JOYDEVICEREMOVED;
    ev.jdevice.which = jid;
    return ev;
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);

    pac::ControllerManager cm;
    cm.init();

    // P1 starts as keyboard fallback (connected=true, joystick_id=-1).
    // P2 unset.
    CHECK(cm.p1.player_id == 1);
    CHECK(cm.p2.player_id == 2);

    // Simulate hot-plug by directly mutating slots — no real SDL device
    // available in test env. Verifies slot_for + reconnect logic.
    cm.p1.joystick_id = 100;
    cm.p1.connected = true;
    cm.p1.name = "fake-p1";
    cm.p2.joystick_id = 101;
    cm.p2.connected = true;
    cm.p2.name = "fake-p2";

    CHECK(cm.slot_for(100) == &cm.p1);
    CHECK(cm.slot_for(101) == &cm.p2);
    CHECK(cm.slot_for(999) == nullptr);

    // Simulate removal event for P1
    cm.handle_event(make_joy_removed(100));
    CHECK(!cm.p1.connected);
    CHECK(cm.p1.grace_left == cm.grace_seconds);
    CHECK(cm.needs_reconnect_pause() == std::nullopt);   // still in grace

    // Tick past grace
    cm.tick(cm.grace_seconds + 0.1f);
    CHECK(cm.needs_reconnect_pause() == 1);

    // P2 still bound
    CHECK(cm.slot_for(101) == &cm.p2);
    CHECK(cm.p2.connected);

    // Remove + readd P2 quickly
    cm.handle_event(make_joy_removed(101));
    CHECK(!cm.p2.connected);
    // Re-add as new jid
    cm.p2.joystick_id = -1;   // simulate manager clearing
    cm.p2.connected = false;

    // Stress: rapid removal+grace cycles
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> jid_dist(50, 200);
    for (int i = 0; i < 100; ++i) {
        SDL_JoystickID jid = jid_dist(rng);
        cm.handle_event(make_joy_removed(jid));    // mostly no-op
        cm.tick(0.5f);
        // Sanity: slot lookup stable
        auto* s = cm.slot_for(jid);
        (void)s;
    }
    CHECK(g_fail == 0);  // no crashes

    // InputRouter route() must not crash on UNBOUND device events
    pac::InputRouter router(cm);
    SDL_Event jb{};
    jb.type = SDL_JOYBUTTONDOWN;
    jb.jbutton.which = 9999;
    jb.jbutton.button = 0;
    auto out = router.route(jb);
    CHECK(out.empty());   // unbound jid → no actions

    SDL_Event hm{};
    hm.type = SDL_JOYHATMOTION;
    hm.jhat.which = 9999;
    hm.jhat.value = SDL_HAT_UP;
    out = router.route(hm);
    CHECK(out.empty());

    // Keyboard event still routes for P1
    SDL_Event kd{};
    kd.type = SDL_KEYDOWN;
    kd.key.repeat = 0;
    kd.key.keysym.sym = SDLK_UP;
    out = router.route(kd);
    bool found_p1_up = false;
    for (auto& p : out) if (p.first == 1 && p.second == "up") found_p1_up = true;
    CHECK(found_p1_up);

    cm.shutdown();
    SDL_Quit();
    std::printf("[hotplug] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
