#include "pacman/render.hpp"

namespace pac {

void draw_turn_swap(SDL_Renderer* r, int from_pid, int to_pid, float t_remaining) {
    Color col = (to_pid == 1) ? P1_COLOR : P2_COLOR;
    int bar_h = 110;
    Color bar = col; bar.a = 200;
    fill_rect(r, 0, WIN_H / 2 - bar_h / 2, WIN_W, bar_h, bar);
    draw_text(r, "PLAYER " + std::to_string(from_pid) + " DOWN",
              WIN_W / 2, WIN_H / 2 - bar_h / 2 + 12, 16, {20, 20, 30}, true);
    draw_text(r, "PLAYER " + std::to_string(to_pid) + " IS UP",
              WIN_W / 2, WIN_H / 2 - 8, 22, {20, 20, 30}, true);
    int dots = std::max(0, static_cast<int>(t_remaining) + 1);
    std::string countdown;
    for (int i = 0; i < dots; ++i) countdown += '.';
    draw_text(r, countdown, WIN_W / 2, WIN_H / 2 + bar_h / 2 - 22, 12, {20, 20, 30}, true);
}

} // namespace pac
