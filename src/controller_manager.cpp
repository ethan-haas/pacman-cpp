#include "pacman/controller_manager.hpp"

#include <algorithm>
#include <cstdio>

namespace pac {

void ControllerManager::init() {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    int n = SDL_NumJoysticks();
    std::fprintf(stderr, "[pacmates] init: %d joystick(s) detected\n", n);
    // Don't scan-bind here — SDL fires JOYDEVICEADDED / CONTROLLERDEVICEADDED
    // events for every pre-existing device on first event poll. The hot-plug
    // handler binds them then, with a robust GC→JS fallback. This avoids the
    // intermittent "open failed: Unable to open /dev/input/event4" race we
    // see when scanning too early in startup.
    int slot = 0;
    for (int i = 0; i < 0 && i < n; ++i) {  // disabled init scan
        SDL_JoystickID jid = -1;
        const char* name = "controller";
        SDL_GameController* gc = nullptr;
        if (SDL_IsGameController(i)) {
            gc = SDL_GameControllerOpen(i);
            if (gc) {
                jid = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
                controllers[jid] = gc;
                name = SDL_GameControllerName(gc);
            }
        }
        if (!gc) {
            // Either no GC mapping, or GC open failed — fall back to raw joystick.
            SDL_Joystick* js = SDL_JoystickOpen(i);
            if (!js) {
                std::fprintf(stderr, "[pacmates] joystick %d: open failed: %s\n",
                             i, SDL_GetError());
                continue;
            }
            jid = SDL_JoystickInstanceID(js);
            joysticks[jid] = js;
            name = SDL_JoystickName(js);
        }
        if (slot == 0) {
            p1.joystick_id = jid;
            p1.name = name ? name : "";
            p1.connected = true;
        } else if (slot == 1) {
            p2.joystick_id = jid;
            p2.name = name ? name : "";
            p2.connected = true;
        }
        std::fprintf(stderr, "[pacmates] controller slot %d -> %s (jid=%d, %s)\n",
                     slot, name ? name : "?", jid,
                     controllers.count(jid) ? "gamecontroller" : "joystick");
        ++slot;
        if (slot >= 2) break;
    }
    std::fprintf(stderr, "[pacmates] %d joystick(s) seen, %d slot(s) bound\n", n, slot);
}

void ControllerManager::shutdown() {
    for (auto& kv : controllers) {
        if (kv.second) SDL_GameControllerClose(kv.second);
    }
    controllers.clear();
    for (auto& kv : joysticks) {
        if (kv.second) SDL_JoystickClose(kv.second);
    }
    joysticks.clear();
}

ControllerSlot* ControllerManager::slot_for(SDL_JoystickID jid) {
    if (p1.joystick_id == jid) return &p1;
    if (p2.joystick_id == jid) return &p2;
    return nullptr;
}

std::optional<int> ControllerManager::needs_reconnect_pause() const {
    for (const auto* s : {&p1, &p2}) {
        if (!s->connected && s->joystick_id != -1 && s->grace_left <= 0.f) {
            return s->player_id;
        }
    }
    return std::nullopt;
}

void ControllerManager::handle_event(const SDL_Event& ev) {
    if (ev.type == SDL_CONTROLLERDEVICEADDED) {
        SDL_GameController* gc = SDL_GameControllerOpen(ev.cdevice.which);
        if (!gc) {
            // GC open failed (busy/perm/race). Fall through to joystick.
            std::fprintf(stderr, "[pacmates] CONTROLLERADDED idx=%d GC open failed: %s — trying joystick\n",
                         ev.cdevice.which, SDL_GetError());
            // Wait one tick and retry as raw joystick
            SDL_Joystick* js = SDL_JoystickOpen(ev.cdevice.which);
            if (!js) {
                std::fprintf(stderr, "[pacmates]   joystick fallback also failed: %s\n",
                             SDL_GetError());
                return;
            }
            SDL_JoystickID jid = SDL_JoystickInstanceID(js);
            if (joysticks.count(jid)) { SDL_JoystickClose(js); return; }
            joysticks[jid] = js;
            for (auto* s : {&p1, &p2}) {
                if (s->joystick_id == -1 || !s->connected) {
                    s->joystick_id = jid;
                    s->name = SDL_JoystickName(js);
                    s->connected = true;
                    s->grace_left = 0;
                    std::fprintf(stderr, "[pacmates] hotplug: P%d bound to %s (jid=%d, JS-fallback)\n",
                                 s->player_id, s->name.c_str(), jid);
                    return;
                }
            }
            return;
        }
        SDL_JoystickID jid = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
        if (controllers.count(jid)) {
            SDL_GameControllerClose(gc);
            return;
        }
        controllers[jid] = gc;
        for (auto* s : {&p1, &p2}) {
            if (s->joystick_id == -1 || !s->connected) {
                s->joystick_id = jid;
                s->name = SDL_GameControllerName(gc);
                s->connected = true;
                s->grace_left = 0;
                std::fprintf(stderr, "[pacmates] hotplug: P%d bound to %s (jid=%d, GC)\n",
                             s->player_id, s->name.c_str(), jid);
                return;
            }
        }
        std::fprintf(stderr, "[pacmates] hotplug: extra controller, no free slot\n");
    } else if (ev.type == SDL_JOYDEVICEADDED) {
        // If it's already a recognised gamecontroller, the CONTROLLERADDED
        // handler will bind it. Otherwise open as raw joystick.
        if (SDL_IsGameController(ev.jdevice.which)) return;
        SDL_Joystick* js = SDL_JoystickOpen(ev.jdevice.which);
        if (!js) {
            std::fprintf(stderr, "[pacmates] JOYADDED idx=%d open failed: %s\n",
                         ev.jdevice.which, SDL_GetError());
            return;
        }
        SDL_JoystickID jid = SDL_JoystickInstanceID(js);
        if (joysticks.count(jid)) {
            SDL_JoystickClose(js);
            return;
        }
        joysticks[jid] = js;
        for (auto* s : {&p1, &p2}) {
            if (s->joystick_id == -1 || !s->connected) {
                s->joystick_id = jid;
                s->name = SDL_JoystickName(js);
                s->connected = true;
                s->grace_left = 0;
                std::fprintf(stderr, "[pacmates] hotplug: P%d bound to %s (jid=%d, JS)\n",
                             s->player_id, s->name.c_str(), jid);
                return;
            }
        }
    } else if (ev.type == SDL_CONTROLLERDEVICEREMOVED || ev.type == SDL_JOYDEVICEREMOVED) {
        SDL_JoystickID jid = (ev.type == SDL_CONTROLLERDEVICEREMOVED)
                             ? ev.cdevice.which : ev.jdevice.which;
        auto cit = controllers.find(jid);
        if (cit != controllers.end()) {
            SDL_GameControllerClose(cit->second);
            controllers.erase(cit);
        }
        auto jit = joysticks.find(jid);
        if (jit != joysticks.end()) {
            SDL_JoystickClose(jit->second);
            joysticks.erase(jit);
        }
        if (auto* s = slot_for(jid)) {
            s->connected = false;
            s->grace_left = grace_seconds;
            std::fprintf(stderr, "[pacmates] removed: P%d (jid=%d) — grace %.1fs\n",
                         s->player_id, jid, grace_seconds);
        }
    }
}

void ControllerManager::tick(float dt) {
    for (auto* s : {&p1, &p2}) {
        if (!s->connected && s->joystick_id != -1 && s->grace_left > 0.f) {
            s->grace_left = std::max(0.f, s->grace_left - dt);
        }
    }
    // Periodic rescan — EmuELEC's SDL2 v2.24 sometimes misses
    // udev hot-plug events for already-bound-then-replugged devices.
    static float rescan_t = 0.f;
    rescan_t += dt;
    if (rescan_t < 3.0f) return;
    rescan_t = 0.f;
    int n = SDL_NumJoysticks();
    for (int i = 0; i < n; ++i) {
        // Cheap query — does NOT open.
        SDL_JoystickID jid = SDL_JoystickGetDeviceInstanceID(i);
        if (jid < 0) continue;
        if (controllers.count(jid) || joysticks.count(jid)) continue;  // already known
        // Not yet bound — try gamecontroller first, then raw joystick.
        SDL_GameController* gc = SDL_IsGameController(i) ? SDL_GameControllerOpen(i) : nullptr;
        const char* name = nullptr;
        if (gc) {
            controllers[jid] = gc;
            name = SDL_GameControllerName(gc);
        } else {
            SDL_Joystick* js = SDL_JoystickOpen(i);
            if (!js) continue;
            joysticks[jid] = js;
            name = SDL_JoystickName(js);
        }
        for (auto* s : {&p1, &p2}) {
            if (s->joystick_id == -1 || !s->connected) {
                s->joystick_id = jid;
                s->name = name ? name : "controller";
                s->connected = true;
                s->grace_left = 0;
                std::fprintf(stderr, "[pacmates] rescan: P%d bound to %s (jid=%d, %s)\n",
                             s->player_id, s->name.c_str(), jid,
                             gc ? "GC" : "JS");
                break;
            }
        }
    }
}

} // namespace pac
