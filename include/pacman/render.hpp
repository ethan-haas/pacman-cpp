#pragma once
#include "constants.hpp"
#include "ghost.hpp"
#include "maze.hpp"
#include "pacman_entity.hpp"
#include "powerup_manager.hpp"

#include <SDL2/SDL.h>
#include <string>

namespace pac {

void set_color(SDL_Renderer* r, Color c);
void fill_rect(SDL_Renderer* r, int x, int y, int w, int h, Color c);
void draw_rect(SDL_Renderer* r, int x, int y, int w, int h, Color c, int thickness = 1);
void fill_circle(SDL_Renderer* r, int cx, int cy, int radius, Color c);
void draw_circle(SDL_Renderer* r, int cx, int cy, int radius, Color c);
void draw_text(SDL_Renderer* r, const std::string& s, int x, int y, int size, Color c, bool center = false);

void draw_maze(SDL_Renderer* r, const Maze& m);
void draw_pellets(SDL_Renderer* r, const Maze& m, float t);
void draw_pacman(SDL_Renderer* r, const Pacman& p, Color color = PAC_YELLOW);
void draw_ghost(SDL_Renderer* r, const Ghost& g, float t, float fright_left);
void draw_pickup(SDL_Renderer* r, const Pickup& p, float t);

inline int to_screen_x(float x) { return static_cast<int>(x * TILE + TILE / 2 + MAZE_X_OFFSET); }
inline int to_screen_y(float y) { return static_cast<int>(y * TILE + TILE / 2 + HUD_TOP + MAZE_Y_OFFSET); }

} // namespace pac
