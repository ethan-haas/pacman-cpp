#pragma once

#include "controller_manager.hpp"

#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace pac {

class InputRouter {
public:
    explicit InputRouter(ControllerManager& cm) : controllers(cm) {}
    // SDL event → list of (player_id, action) pairs
    std::vector<std::pair<int, std::string>> route(const SDL_Event& ev);
    // Poll the last-known dpad direction for a player (for analog-stick hold).
    std::string poll_dpad(int player_id) const {
        auto it = dpad_.find(player_id);
        return (it == dpad_.end()) ? "" : it->second;
    }

private:
    ControllerManager& controllers;
    std::unordered_map<int, std::string> dpad_;
};

} // namespace pac
