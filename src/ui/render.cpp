#include "pacman/render.hpp"

#include <SDL2/SDL.h>
#include <cmath>
#include <vector>

namespace pac {

void set_color(SDL_Renderer* r, Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

void fill_rect(SDL_Renderer* r, int x, int y, int w, int h, Color c) {
    set_color(r, c);
    SDL_Rect rc{x, y, w, h};
    SDL_RenderFillRect(r, &rc);
}

void draw_rect(SDL_Renderer* r, int x, int y, int w, int h, Color c, int thickness) {
    set_color(r, c);
    for (int t = 0; t < thickness; ++t) {
        SDL_Rect rc{x + t, y + t, w - 2 * t, h - 2 * t};
        SDL_RenderDrawRect(r, &rc);
    }
}

void fill_circle(SDL_Renderer* r, int cx, int cy, int radius, Color c) {
    set_color(r, c);
    for (int dy = -radius; dy <= radius; ++dy) {
        int dx = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - dy * dy)));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void draw_circle(SDL_Renderer* r, int cx, int cy, int radius, Color c) {
    set_color(r, c);
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);
        ++y;
        if (err <= 0) err += 2 * y + 1;
        if (err > 0) { --x; err -= 2 * x + 1; }
    }
}

// --- text rendering: tiny 5×7 bitmap font (uppercase + digits + space).
// Keeps the binary self-contained (no SDL_ttf font file deps required for
// gameplay HUD). SDL2_ttf is still linked for higher-quality menu text in
// a future pass.
namespace {
struct Glyph { char ch; std::uint8_t rows[7]; };
// 1-bit per pixel, 5 cols × 7 rows. MSB = leftmost pixel.
constexpr Glyph FONT[] = {
    {' ', {0,0,0,0,0,0,0}},
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3', {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},
    {'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5', {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
    {'6', {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
    {'7', {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9', {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D', {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}},
    {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'J', {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M', {0x11,0x1B,0x15,0x11,0x11,0x11,0x11}},
    {'N', {0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W', {0x11,0x11,0x11,0x11,0x15,0x15,0x0A}},
    {'X', {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y', {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
    {':', {0x00,0x04,0x04,0x00,0x04,0x04,0x00}},
    {'.', {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}},
    {'!', {0x04,0x04,0x04,0x04,0x04,0x00,0x04}},
    {'+', {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
    {'>', {0x10,0x08,0x04,0x02,0x04,0x08,0x10}},
    {'<', {0x01,0x02,0x04,0x08,0x04,0x02,0x01}},
    {'/', {0x01,0x02,0x04,0x08,0x10,0x00,0x00}},
    {',', {0x00,0x00,0x00,0x00,0x0C,0x04,0x08}},
    {'(', {0x02,0x04,0x08,0x08,0x08,0x04,0x02}},
    {')', {0x08,0x04,0x02,0x02,0x02,0x04,0x08}},
};

const Glyph* find_glyph_(char c) {
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    for (auto& g : FONT) if (g.ch == c) return &g;
    return nullptr;
}
} // anon

void draw_text(SDL_Renderer* r, const std::string& s, int x, int y, int size, Color c, bool center) {
    int scale = std::max(1, size / 7);
    int glyph_w = 5 * scale;
    int spacing = scale;
    int total_w = static_cast<int>(s.size()) * (glyph_w + spacing) - spacing;
    int cx = center ? (x - total_w / 2) : x;
    set_color(r, c);
    for (char ch : s) {
        const Glyph* g = find_glyph_(ch);
        if (g) {
            for (int row = 0; row < 7; ++row) {
                std::uint8_t bits = g->rows[row];
                for (int col = 0; col < 5; ++col) {
                    if (bits & (0x10 >> col)) {
                        SDL_Rect rc{cx + col * scale, y + row * scale, scale, scale};
                        SDL_RenderFillRect(r, &rc);
                    }
                }
            }
        }
        cx += glyph_w + spacing;
    }
}

// Helper: render a single wall cell at arbitrary pixel offset (used for
// the playable maze AND the decorative side wings).
static void draw_wall_cell_(SDL_Renderer* r, int sx, int sy,
                            bool open_up, bool open_down,
                            bool open_left, bool open_right) {
    fill_rect(r, sx, sy, TILE, TILE, WALL_FILL);
    set_color(r, WALL_RIM);
    const int inset = 3;
    if (open_up)    SDL_RenderDrawLine(r, sx + inset, sy + inset,
                                       sx + TILE - inset, sy + inset);
    if (open_down)  SDL_RenderDrawLine(r, sx + inset, sy + TILE - inset,
                                       sx + TILE - inset, sy + TILE - inset);
    if (open_left)  SDL_RenderDrawLine(r, sx + inset, sy + inset,
                                       sx + inset, sy + TILE - inset);
    if (open_right) SDL_RenderDrawLine(r, sx + TILE - inset, sy + inset,
                                       sx + TILE - inset, sy + TILE - inset);
}

void draw_maze(SDL_Renderer* r, const Maze& m) {
    // --- PLAYABLE maze (60 cols × 31 rows fills the screen) ---
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            if (m.at(x, y) != '#') continue;
            int sx = x * TILE + MAZE_X_OFFSET;
            int sy = y * TILE + HUD_TOP + MAZE_Y_OFFSET;
            draw_wall_cell_(r, sx, sy,
                            !m.is_wall(x, y - 1),
                            !m.is_wall(x, y + 1),
                            !m.is_wall(x - 1, y),
                            !m.is_wall(x + 1, y));
        }
    }
    // door
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            if (m.at(x, y) == '-') {
                fill_rect(r, x * TILE + MAZE_X_OFFSET,
                          y * TILE + HUD_TOP + MAZE_Y_OFFSET + TILE / 2 - 2,
                          TILE, 4, DOOR);
            }
        }
    }
}

void draw_pellets(SDL_Renderer* r, const Maze& m, float t) {
    // Pellets pulse subtly with a low-frequency sine
    int pellet_r = std::max(2, TILE / 8);
    int pellet_pulse = (int)((std::sin(t * 2.0f) + 1.0f) * 0.5f);   // 0 or 1
    for (auto& tile : m.pellets()) {
        fill_circle(r, to_screen_x(tile.x), to_screen_y(tile.y),
                    pellet_r + pellet_pulse, PELLET);
    }
    // Energizers throb visibly
    int base = TILE / 4;
    int flash = static_cast<int>((std::sin(t * 6.0f) + 1.0f) * 1.5f);
    int er = base + flash;
    for (auto& tile : m.energizers()) {
        // Glow halo
        Color halo = ENERGIZER; halo.a = 110;
        fill_circle(r, to_screen_x(tile.x), to_screen_y(tile.y), er + 6, halo);
        fill_circle(r, to_screen_x(tile.x), to_screen_y(tile.y), er + 2, ENERGIZER);
        fill_circle(r, to_screen_x(tile.x), to_screen_y(tile.y), er - 2, {255, 240, 150});
    }
}

void draw_pacman(SDL_Renderer* r, const Pacman& p, Color color) {
    if (!p.alive) return;
    if (p.invuln > 0 && (int)(p.invuln * 8) % 2 == 0) return;
    int cx = to_screen_x(p.x), cy = to_screen_y(p.y);
    int radius = TILE / 2 - 1;

    // Solid body
    fill_circle(r, cx, cy, radius, color);

    // Mouth pie cutout — proper wedge filled in BG colour using triangle
    // fan via SDL_RenderGeometry. Mouth opens/closes based on p.mouth.
    float open = std::fabs(std::sin(p.mouth)) * 0.55f + 0.05f;   // 0.05..0.60 rad
    float base = 0.f;
    switch (p.direction) {
        case Dir::Right: base = 0.f;           break;
        case Dir::Left:  base = 3.14159265f;   break;
        case Dir::Up:    base = -1.5707963f;   break;
        case Dir::Down:  base = 1.5707963f;    break;
        default:         base = 0.f;
    }
    float a1 = base - open;
    float a2 = base + open;

    const int FAN = 10;
    SDL_Vertex verts[FAN + 2];
    SDL_Color bg_sdl = {BG.r, BG.g, BG.b, 255};
    verts[0].position = {static_cast<float>(cx), static_cast<float>(cy)};
    verts[0].color = bg_sdl;
    verts[0].tex_coord = {0, 0};
    float reach = static_cast<float>(radius) + 2.0f;  // slight overshoot to clip the edge
    for (int i = 0; i <= FAN; ++i) {
        float a = a1 + (a2 - a1) * (static_cast<float>(i) / FAN);
        verts[i + 1].position = {cx + std::cos(a) * reach, cy + std::sin(a) * reach};
        verts[i + 1].color = bg_sdl;
        verts[i + 1].tex_coord = {0, 0};
    }
    int indices[FAN * 3];
    for (int i = 0; i < FAN; ++i) {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }
    SDL_RenderGeometry(r, nullptr, verts, FAN + 2, indices, FAN * 3);

    // Tiny eye dot above the body (classic pacman has a small eye)
    int ey_dx = dx_of(p.direction) * 2;
    int ey_dy = dy_of(p.direction) * 2;
    fill_circle(r, cx + ey_dx, cy - radius / 2 + ey_dy, std::max(2, TILE / 14), {20, 20, 30});
}

void draw_ghost(SDL_Renderer* r, const Ghost& g, float t, float fright_left) {
    int cx = to_screen_x(g.x), cy = to_screen_y(g.y);
    int radius = TILE / 2 - 1;
    Color col = g.color;
    if (g.mode == GhostMode::Eaten) col = EYES;
    else if (g.mode == GhostMode::Frightened) {
        // Flash white at 6 Hz during last 2 seconds of fright (warns end).
        col = (fright_left < 2.0f && (int)(t * 6) % 2 == 0) ? FRIGHT_FLASH : FRIGHT;
    }
    if (g.mode != GhostMode::Eaten) {
        // Wobble vertically a touch — adds life
        int wob = (int)(std::sin(t * 6.0f + (long long)&g) * 1.0f);
        fill_circle(r, cx, cy - 1 + wob, radius, col);
        fill_rect(r, cx - radius, cy - 1 + wob, radius * 2, radius, col);
        // Skirt (alternating teeth animate)
        int phase = (int)(t * 6.0f) & 1;
        int n = 6;
        for (int i = 0; i < n; ++i) {
            int x0 = cx - radius + i * (radius * 2 / n);
            int x1 = x0 + (radius * 2 / n);
            int yoff = (((i + phase) & 1) ? 3 : 0);
            fill_rect(r, x0, cy + radius - 1 + wob, x1 - x0, 3 + yoff, col);
        }
    }
    // Eyes: track Pacman direction (frightened ghosts look "down" / blank)
    int eye_r = std::max(3, TILE / 7);
    int spread = TILE / 6;
    int eye_y = cy - 2;
    bool frightened = (g.mode == GhostMode::Frightened);
    if (frightened) {
        // Tiny single line "scared" eyes
        Color line = (col.r > 180) ? Color{255, 80, 80} : Color{255, 200, 60};
        for (int dx : {-spread, spread}) {
            fill_rect(r, cx + dx - 3, eye_y - 1, 6, 2, line);
        }
        return;
    }
    fill_circle(r, cx - spread, eye_y, eye_r, EYES);
    fill_circle(r, cx + spread, eye_y, eye_r, EYES);
    int ex = (g.direction == Dir::Left) ? -2 : (g.direction == Dir::Right ? 2 : 0);
    int ey = (g.direction == Dir::Up) ? -2 : (g.direction == Dir::Down ? 2 : 0);
    int pupil_r = std::max(1, TILE / 14) + 1;
    fill_circle(r, cx - spread + ex, eye_y + ey, pupil_r, PUPIL);
    fill_circle(r, cx + spread + ex, eye_y + ey, pupil_r, PUPIL);
}

void draw_pickup(SDL_Renderer* r, const Pickup& p, float t) {
    int cx = to_screen_x(p.x), cy = to_screen_y(p.y);
    if (p.kind == PowerUp::Heart) {
        fill_circle(r, cx - 4, cy - 2, 5, {255, 90, 130});
        fill_circle(r, cx + 4, cy - 2, 5, {255, 90, 130});
        std::vector<SDL_Point> pts = {{cx - 8, cy}, {cx + 8, cy}, {cx, cy + 9}};
        set_color(r, {255, 90, 130});
        SDL_RenderDrawLines(r, pts.data(), 3);
        return;
    }
    Color c = powerup_color(p.kind);
    int pulse = static_cast<int>((std::sin(t * 6) + 1.0f));
    int rr = TILE / 2 - 2 + pulse;
    fill_circle(r, cx, cy, rr, c);
    draw_circle(r, cx, cy, rr, {255, 255, 255});
    draw_text(r, std::string(powerup_glyph(p.kind)), cx, cy - 4, 10, {20, 20, 30}, true);
}

} // namespace pac
