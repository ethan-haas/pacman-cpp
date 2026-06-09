#include "pacman/input_router.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace pac {

static std::string p1_key_action_(SDL_Keycode k) {
    switch (k) {
        case SDLK_UP: case SDLK_w:        return "up";
        case SDLK_DOWN: case SDLK_s:      return "down";
        case SDLK_LEFT: case SDLK_a:      return "left";
        case SDLK_RIGHT: case SDLK_d:     return "right";
        case SDLK_RETURN: case SDLK_SPACE:return "confirm";
        case SDLK_ESCAPE: case SDLK_p:    return "pause";
        case SDLK_BACKSPACE:              return "back";
        case SDLK_q:                      return "ping";
        default:                          return "";
    }
}

static std::string p2_key_action_(SDL_Keycode k) {
    switch (k) {
        case SDLK_i: case SDLK_KP_8: return "up";
        case SDLK_k: case SDLK_KP_5: return "down";
        case SDLK_j: case SDLK_KP_4: return "left";
        case SDLK_l: case SDLK_KP_6: return "right";
        case SDLK_RSHIFT: case SDLK_KP_ENTER: return "confirm";
        case SDLK_o: return "pause";
        case SDLK_u: return "back";
        case SDLK_y: return "ping";
        default:    return "";
    }
}

static std::string button_action_(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:     return "confirm";
        case SDL_CONTROLLER_BUTTON_B:     return "back";
        case SDL_CONTROLLER_BUTTON_Y:     return "ping";
        case SDL_CONTROLLER_BUTTON_BACK:  return "back";
        case SDL_CONTROLLER_BUTTON_START: return "pause";
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    return "up";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  return "down";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  return "left";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "right";
        default: return "";
    }
}

// Raw joystick button map (Xbox 360 / generic XInput layout).
// Used when SDL_GameController mapping isn't available (EmuELEC's older SDL2 DB).
static std::string js_button_action_(Uint8 button) {
    switch (button) {
        case 0: return "confirm";   // A
        case 1: return "back";      // B
        case 2: return "ping";      // X (some pads) / Y (others)
        case 3: return "ping";      // Y
        case 4: return "back";      // LB / Back-shoulder
        case 5: return "pause";     // RB / Start-ish
        case 6: return "back";      // Back / Select
        case 7: return "pause";     // Start
        default: return "";
    }
}

std::vector<std::pair<int, std::string>> InputRouter::route(const SDL_Event& ev) {
    std::vector<std::pair<int, std::string>> out;
    static const bool DBG = std::getenv("PACMATES_DEBUG_INPUT") != nullptr;
    if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
        auto a1 = p1_key_action_(ev.key.keysym.sym);
        if (!a1.empty()) out.emplace_back(1, a1);
        auto a2 = p2_key_action_(ev.key.keysym.sym);
        if (!a2.empty()) out.emplace_back(2, a2);
    } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
        auto* slot = controllers.slot_for(ev.cbutton.which);
        if (slot) {
            auto a = button_action_(ev.cbutton.button);
            if (!a.empty()) out.emplace_back(slot->player_id, a);
            if (DBG) std::fprintf(stderr, "[pacmates] CB jid=%d btn=%d -> P%d %s\n",
                                  ev.cbutton.which, ev.cbutton.button,
                                  slot->player_id, a.c_str());
        } else if (DBG) {
            std::fprintf(stderr, "[pacmates] CB jid=%d btn=%d (UNBOUND)\n",
                         ev.cbutton.which, ev.cbutton.button);
        }
    } else if (ev.type == SDL_CONTROLLERAXISMOTION) {
        auto* slot = controllers.slot_for(ev.caxis.which);
        if (slot) {
            auto it = controllers.controllers.find(ev.caxis.which);
            if (it == controllers.controllers.end()) return out;
            SDL_GameController* gc = it->second;
            if (!gc) return out;
            int ax_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
            int ay_raw = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
            float ax = ax_raw / 32768.f;
            float ay = ay_raw / 32768.f;
            std::string d;
            if (std::fabs(ax) < 0.4f && std::fabs(ay) < 0.4f) d = "";
            else if (std::fabs(ax) > std::fabs(ay)) d = (ax < 0) ? "left" : "right";
            else d = (ay < 0) ? "up" : "down";
            std::string& prev = dpad_[slot->player_id];
            if (!d.empty() && d != prev) out.emplace_back(slot->player_id, d);
            prev = d;
        }
    } else if (ev.type == SDL_JOYBUTTONDOWN) {
        // Gate: only honour for joystick-opened devices, else gamecontroller
        // gets double events (CONTROLLERBUTTONDOWN + JOYBUTTONDOWN).
        if (controllers.joysticks.find(ev.jbutton.which) == controllers.joysticks.end())
            return out;
        auto* slot = controllers.slot_for(ev.jbutton.which);
        if (slot) {
            auto a = js_button_action_(ev.jbutton.button);
            if (!a.empty()) out.emplace_back(slot->player_id, a);
        }
    } else if (ev.type == SDL_JOYHATMOTION) {
        // Same gate — dpad on gamecontroller already arrives via
        // CONTROLLERBUTTONDOWN; honouring the hat too caused +2 menu
        // jumps per press.
        if (controllers.joysticks.find(ev.jhat.which) == controllers.joysticks.end())
            return out;
        auto* slot = controllers.slot_for(ev.jhat.which);
        if (slot) {
            Uint8 v = ev.jhat.value;
            if (v & SDL_HAT_UP)    out.emplace_back(slot->player_id, "up");
            if (v & SDL_HAT_DOWN)  out.emplace_back(slot->player_id, "down");
            if (v & SDL_HAT_LEFT)  out.emplace_back(slot->player_id, "left");
            if (v & SDL_HAT_RIGHT) out.emplace_back(slot->player_id, "right");
        }
    } else if (ev.type == SDL_JOYAXISMOTION) {
        // Only honour raw joystick axis events for devices opened as
        // SDL_Joystick (no gamecontroller mapping). Otherwise we
        // double-handle the left stick AND, on some clone pads, mis-
        // route dpad press onto axes 0/1 producing inverted dpad nav.
        auto jit = controllers.joysticks.find(ev.jaxis.which);
        if (jit == controllers.joysticks.end()) return out;
        auto* slot = controllers.slot_for(ev.jaxis.which);
        if (!slot) return out;
        SDL_Joystick* js = jit->second;
        if (!js) return out;
        float ax = SDL_JoystickGetAxis(js, 0) / 32768.f;
        float ay = SDL_JoystickGetAxis(js, 1) / 32768.f;
        std::string d;
        if (std::fabs(ax) < 0.4f && std::fabs(ay) < 0.4f) d = "";
        else if (std::fabs(ax) > std::fabs(ay)) d = (ax < 0) ? "left" : "right";
        else d = (ay < 0) ? "up" : "down";
        std::string& prev = dpad_[slot->player_id];
        if (!d.empty() && d != prev) out.emplace_back(slot->player_id, d);
        prev = d;
    }
    return out;
}

} // namespace pac
