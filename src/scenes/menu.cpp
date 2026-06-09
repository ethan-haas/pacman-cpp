#include "pacman/app.hpp"
#include "pacman/render.hpp"

#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace pac {

class MainMenuScene : public Scene {
public:
    App& app;
    int sel = 0;
    float t = 0.f;
    std::vector<std::string> options = {"PLAY", "MULTIPLAYER", "QUIT"};

    explicit MainMenuScene(App& a) : app(a) {}

    void update(float dt) override { t += dt; }

    void draw(SDL_Renderer* r) override {
        fill_rect(r, 0, 0, WIN_W, WIN_H, BG);
        draw_text(r, "PAC MATES", WIN_W / 2, 180, 112, {255, 220, 60}, true);
        draw_text(r, "MULTIPLAYER EDITION", WIN_W / 2, 300, 42, {200, 220, 255}, true);
        for (size_t i = 0; i < options.size(); ++i) {
            Color c = (static_cast<int>(i) == sel) ? Color{255, 220, 60} : Color{220, 220, 240};
            std::string label = options[i];
            if (static_cast<int>(i) == sel) label = "> " + label + " <";
            draw_text(r, label, WIN_W / 2, 500 + static_cast<int>(i) * 80, 56, c, true);
        }
        draw_text(r, "A CONFIRM   DPAD NAVIGATE   B BACK", WIN_W / 2,
                  WIN_H - 60, 28, {160, 160, 200}, true);
    }

    void do_action(const std::string& action) {
        if (action == "up")
            sel = (sel - 1 + static_cast<int>(options.size())) % static_cast<int>(options.size());
        else if (action == "down")
            sel = (sel + 1) % static_cast<int>(options.size());
        else if (action == "confirm") {
            if (options[sel] == "PLAY") app.start_solo_game();
            else if (options[sel] == "MULTIPLAYER") app.go_mode_select();
            else app.quit();
        } else if (action == "back" || action == "pause") {
            // no-op at top menu
        }
    }

    void handle_event(const SDL_Event& ev) override {
        // Keyboard
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
            switch (ev.key.keysym.sym) {
                case SDLK_UP: case SDLK_w:        do_action("up"); break;
                case SDLK_DOWN: case SDLK_s:      do_action("down"); break;
                case SDLK_RETURN: case SDLK_SPACE:do_action("confirm"); break;
                case SDLK_ESCAPE: app.quit(); break;
                default: break;
            }
            return;
        }
        // Controller events (game controller OR raw joystick)
        for (auto& [_pid, action] : app.router.route(ev)) {
            do_action(action);
        }
    }
};

std::unique_ptr<Scene> make_main_menu(App& app) {
    return std::make_unique<MainMenuScene>(app);
}

} // namespace pac
