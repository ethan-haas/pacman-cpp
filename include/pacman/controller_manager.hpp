#pragma once

#include <SDL2/SDL.h>
#include <optional>
#include <string>
#include <unordered_map>

namespace pac {

struct ControllerSlot {
    int player_id = 0;
    SDL_JoystickID joystick_id = -1;   // -1 = keyboard fallback / unassigned
    std::string name;
    bool connected = true;
    float grace_left = 0.f;
};

class ControllerManager {
public:
    ControllerSlot p1{1, -1, "keyboard", true, 0.f};
    ControllerSlot p2{2, -1, "", false, 0.f};
    std::unordered_map<SDL_JoystickID, SDL_GameController*> controllers;
    // Devices opened as raw SDL_Joystick (no gamecontroller mapping
    // available — common for EmuELEC's SDL2 2.24 lacking newer DB).
    std::unordered_map<SDL_JoystickID, SDL_Joystick*> joysticks;
    float grace_seconds = 5.0f;

    void init();
    void shutdown();
    bool both_present() const { return p1.connected && p2.connected; }
    ControllerSlot* slot_for(SDL_JoystickID jid);
    std::optional<int> needs_reconnect_pause() const;

    void handle_event(const SDL_Event& ev);
    void tick(float dt);
};

} // namespace pac
