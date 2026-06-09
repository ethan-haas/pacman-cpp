#include "pacman/app.hpp"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>

namespace pac {

std::unique_ptr<Scene> make_main_menu(App&);
std::unique_ptr<Scene> make_mode_select(App&);
std::unique_ptr<Scene> make_multi_gameplay_scene(App&, const std::string&);

App::App(bool hl) : headless(hl) {
    Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK;
    if (!headless) flags |= SDL_INIT_AUDIO;
    if (headless) {
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    }
    if (SDL_Init(flags) < 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        running = false;
        return;
    }
    // Fullscreen on real device (env-flag or auto-detect kmsdrm). On x2 Pro
    // EmuELEC runs kmsdrm at 1080p; we render at native 672×864 and let
    // SDL scale to the display via SDL_RenderSetLogicalSize.
    Uint32 win_flags = SDL_WINDOW_SHOWN;
    const char* fs = SDL_getenv("PACMAN_FULLSCREEN");
    const char* drv = SDL_GetCurrentVideoDriver();
    bool want_fs = (fs && fs[0] == '1') || (drv && (std::strstr(drv, "kmsdrm") || std::strstr(drv, "rpi")));
    if (want_fs && !headless) win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    window = SDL_CreateWindow("PAC-MAN Reimagined v2.0 (C++)",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WIN_W, WIN_H, win_flags);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        running = false;
        return;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderSetLogicalSize(renderer, WIN_W, WIN_H);
        // Non-integer scale → fits the smaller axis fully (vertical on
        // landscape TVs). Portrait game on 1080p TV: 1.25× vertical,
        // pillarboxed sides. Better than 1× integer (1/3 of screen).
        SDL_RenderSetIntegerScale(renderer, SDL_FALSE);
    }
    // Init joystick/gamecontroller subsystem and bind any plugged
    // devices NOW, so menus respond to controllers too.
    controllers.init();
    scene = make_main_menu(*this);
}

App::~App() {
    scene.reset();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}

void App::go_main_menu()        { scene = make_main_menu(*this); }
void App::go_mode_select()      { scene = make_mode_select(*this); }
void App::start_solo_game()     { scene = make_multi_gameplay_scene(*this, "single"); }
void App::start_multi_game(const std::string& m) { scene = make_multi_gameplay_scene(*this, m); }

void App::run(int max_frames) {
    Uint64 last = SDL_GetPerformanceCounter();
    double freq = static_cast<double>(SDL_GetPerformanceFrequency());
    int frame = 0;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = false; break; }
            // App-level: let controller manager observe hot-plug events
            // before scenes handle the rest.
            controllers.handle_event(ev);
            if (scene) scene->handle_event(ev);
        }
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (now - last) / freq;
        last = now;
        if (dt > 1.0 / 30.0) dt = 1.0 / 30.0;
        // App-level controller tick — runs in menus too, where scene
        // doesn't tick the manager itself. Drives the periodic rescan
        // and disconnect-grace timer.
        controllers.tick(static_cast<float>(dt));
        if (scene) {
            scene->update(static_cast<float>(dt));
            scene->draw(renderer);
        }
        if (renderer) SDL_RenderPresent(renderer);
        ++frame;
        if (max_frames > 0 && frame >= max_frames) break;
        if (!renderer) SDL_Delay(16);
    }
}

} // namespace pac
