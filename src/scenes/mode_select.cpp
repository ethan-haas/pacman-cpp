#include "pacman/app.hpp"
#include "pacman/render.hpp"

#include <array>
#include <string>

namespace pac {

struct Tile3 { const char* key; const char* label; const char* blurb; };
static const std::array<Tile3, 3> TILES = {{
    {"single",      "1 PLAYER",      "SOLO CLASSIC"},
    {"alternating", "2P ALTERNATE",  "TAKE TURNS HIGH SCORE WINS"},
    {"coop",        "2P COOP",       "SHARE MAZE SAVE PARTNER"},
}};

class ModeSelectScene : public Scene {
public:
    App& app;
    int sel = 0;
    float t = 0.f;

    explicit ModeSelectScene(App& a) : app(a) {}
    void update(float dt) override { t += dt; }

    void draw(SDL_Renderer* r) override {
        fill_rect(r, 0, 0, WIN_W, WIN_H, BG);
        draw_text(r, "MODE SELECT", WIN_W / 2, 140, 84, {255, 220, 60}, true);
        int tile_w = 480, tile_h = 540, gap = 40;
        int total_w = tile_w * 3 + gap * 2;
        int x0 = (WIN_W - total_w) / 2;
        int y0 = 280;
        for (size_t i = 0; i < TILES.size(); ++i) {
            int x = x0 + static_cast<int>(i) * (tile_w + gap);
            Color base = {40, 60, 120};
            Color border = (static_cast<int>(i) == sel) ? Color{255, 220, 60} : Color{90, 130, 200};
            fill_rect(r, x, y0, tile_w, tile_h, base);
            draw_rect(r, x, y0, tile_w, tile_h, border, 6);
            draw_text(r, TILES[i].label, x + tile_w / 2, y0 + 36, 42, {255, 255, 255}, true);
            draw_text(r, TILES[i].blurb, x + tile_w / 2, y0 + 160, 21, {220, 220, 240}, true);
            int cx = x + tile_w / 2, cy = y0 + 380;
            fill_circle(r, cx - 32, cy, 28, P1_COLOR);
            if (std::string(TILES[i].key) != "single") {
                fill_circle(r, cx + 32, cy, 28, P2_COLOR);
            }
        }
        draw_text(r, "DPAD SELECT   A START   B BACK", WIN_W / 2,
                  WIN_H - 64, 28, {160, 160, 200}, true);
    }

    void do_action(const std::string& a) {
        if (a == "left" || a == "up")
            sel = (sel - 1 + static_cast<int>(TILES.size())) % static_cast<int>(TILES.size());
        else if (a == "right" || a == "down")
            sel = (sel + 1) % static_cast<int>(TILES.size());
        else if (a == "confirm")
            app.start_multi_game(TILES[sel].key);
        else if (a == "back")
            app.go_main_menu();
    }

    void handle_event(const SDL_Event& ev) override {
        if (ev.type == SDL_KEYDOWN && ev.key.repeat == 0) {
            switch (ev.key.keysym.sym) {
                case SDLK_LEFT: case SDLK_a: do_action("left"); break;
                case SDLK_RIGHT: case SDLK_d: do_action("right"); break;
                case SDLK_UP: case SDLK_w: do_action("up"); break;
                case SDLK_DOWN: case SDLK_s: do_action("down"); break;
                case SDLK_RETURN: case SDLK_SPACE: do_action("confirm"); break;
                case SDLK_ESCAPE: do_action("back"); break;
                default: break;
            }
            return;
        }
        for (auto& [_pid, action] : app.router.route(ev)) {
            do_action(action);
        }
    }
};

std::unique_ptr<Scene> make_mode_select(App& app) {
    return std::make_unique<ModeSelectScene>(app);
}

} // namespace pac
