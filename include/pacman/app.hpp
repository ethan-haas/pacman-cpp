#pragma once
#include "audio.hpp"
#include "constants.hpp"
#include "controller_manager.hpp"
#include "input_router.hpp"
#include <SDL2/SDL.h>
#include <memory>
#include <string>

namespace pac {

class Scene {
public:
    virtual ~Scene() = default;
    virtual void update(float dt) = 0;
    virtual void draw(SDL_Renderer* r) = 0;
    virtual void handle_event(const SDL_Event& ev) = 0;
};

class App {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;
    bool headless = false;
    std::unique_ptr<Scene> scene;
    ControllerManager controllers;
    InputRouter router{controllers};
    Audio audio;

    explicit App(bool headless = false);
    ~App();

    void run(int max_frames = -1);
    void go_main_menu();
    void go_mode_select();
    void start_solo_game();
    void start_multi_game(const std::string& mode_name);
    void quit() { running = false; }
};

} // namespace pac
