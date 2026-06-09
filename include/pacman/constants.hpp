// Game-wide constants — single source of truth.
// Port of pacman_reimagined/game/constants.py.
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pac {

// --- grid / window ---
// 1080p TV target. Classic 28×31 Pac-Man layout.
//
// TV-safe overscan: 2.5 % on every edge (48 px horiz / 27 px vert).
//
// Vertical layout:
//   HUD top strip       y=27..71         (44 tall, inside safe area)
//   maze                y=75..1005       (930 tall, TILE=30)
//   HUD bottom strip    y=1009..1053     (44 tall, inside safe area)
//
// Horizontal layout:
//   playable maze       x=540..1380      (840 wide, centred)
//   decorative wall art x=48..540 and x=1380..1872
//   (the side art continues the maze pattern up to the safe inset)
// Per pacman-coop-PRD.md §4 + §9: 28×36 internal Pac-Man grid.
// TV is 16:9 (1920×1080); PRD targets 9:16 portrait. Adapt by
// placing the 28×36 playfield centred and using the side wings for
// extended HUD (marquee + scores + fruit tray as PRD §8 chrome).
//
// TILE 26 → playfield 728×936. HUD chrome rendered around it within
// 1920×1080. (TILE 22 would also work for tighter HUD margins.)
// Geometry budget (vertical, must sum to <= WIN_H = 1080):
//   SAFE_INSET_Y     30
//   HUD_BAR_H        66    ← bigger, fits 24 px values + 14 px labels inside
//   HUD_MAZE_GAP     20
//   PLAY_H           = ROWS * TILE
//   HUD_MAZE_GAP     20
//   HUD_BAR_H        66
//   SAFE_INSET_Y     30
//   Total fixed      232    → PLAY_H budget = 848 → TILE = 27 (PLAY_H = 837)
//   Remaining slack  11     → split into top + bottom of maze (MAZE_Y_OFFSET)
constexpr int TILE = 27;
constexpr int COLS = 28;
constexpr int ROWS = 31;
constexpr int PLAY_W = COLS * TILE;        // 756
constexpr int PLAY_H = ROWS * TILE;        // 837
constexpr int WIN_W = 1920;
constexpr int WIN_H = 1080;
constexpr int SAFE_INSET_X = 48;
constexpr int SAFE_INSET_Y = 30;
constexpr int HUD_MAZE_GAP = 20;
constexpr int SAFE_X0 = SAFE_INSET_X;
constexpr int SAFE_Y0 = SAFE_INSET_Y;
constexpr int SAFE_X1 = WIN_W - SAFE_INSET_X;
constexpr int SAFE_Y1 = WIN_H - SAFE_INSET_Y;
constexpr int HUD_BAR_H = 66;
constexpr int HUD_BAR_TOP_Y = SAFE_Y0;                            // 30
constexpr int HUD_BAR_BOT_Y = SAFE_Y1 - HUD_BAR_H;                // 984
// Maze vertically centred between HUD bars.
// render.hpp uses (HUD_TOP + MAZE_Y_OFFSET) — keep MAZE_Y_OFFSET=0 so
// centering math lives ONLY in HUD_TOP and we don't double-offset.
constexpr int _BAR_BOT_OF_TOP = SAFE_Y0 + HUD_BAR_H;              // 96
constexpr int _AVAIL_V = HUD_BAR_BOT_Y - _BAR_BOT_OF_TOP;         // 888
constexpr int MAZE_Y_OFFSET = 0;
constexpr int HUD_TOP    = _BAR_BOT_OF_TOP + (_AVAIL_V - PLAY_H) / 2;  // 121
constexpr int HUD_BOTTOM = HUD_BAR_H;
constexpr int MAZE_X_OFFSET = (WIN_W - PLAY_W) / 2;               // 582 — centred
constexpr int FPS = 60;

struct Color { std::uint8_t r, g, b, a = 255; };

// --- palette --- (reverted to neon)
constexpr Color BG          {11, 14, 44};
constexpr Color WALL_FILL   {24, 30, 70};
constexpr Color WALL_RIM    {90, 200, 255};
constexpr Color WALL_HC     {40, 40, 60};
constexpr Color WALL_RIM_HC {255, 255, 255};
constexpr Color DOOR        {255, 180, 230};
constexpr Color PELLET      {255, 220, 180};
constexpr Color ENERGIZER   {255, 230, 120};
constexpr Color PAC_YELLOW  {255, 220, 60};
constexpr Color PAC_CYAN    {34, 211, 238};
constexpr Color P1_COLOR    = PAC_YELLOW;
constexpr Color P2_COLOR    = PAC_CYAN;
constexpr Color PAC_GREEN   = P2_COLOR;             // legacy alias
constexpr Color TEXT        {240, 240, 255};
constexpr Color TEXT_DIM    {160, 160, 200};
constexpr Color RED         {255, 80, 80};
constexpr Color PINK        {255, 170, 200};
constexpr Color CYAN        {130, 230, 255};
constexpr Color ORANGE      {255, 170, 80};
constexpr Color FRIGHT      {50, 70, 200};
constexpr Color FRIGHT_FLASH{240, 240, 255};
constexpr Color EYES        {240, 240, 240};
constexpr Color PUPIL       {40, 40, 120};

// --- movement (tiles/sec) ---
constexpr float PAC_SPEED    = 6.0f;
constexpr float GHOST_SPEED  = 5.7f;
constexpr float GHOST_FRIGHT = 3.6f;
constexpr float GHOST_EATEN  = 9.5f;

// --- mode timing (scatter/chase cycle) ---
struct ModePhase { float duration; const char* mode; };  // duration < 0 = forever
constexpr std::array<ModePhase, 8> SCATTER_CHASE = {{
    {7.0f, "scatter"}, {20.0f, "chase"},
    {7.0f, "scatter"}, {20.0f, "chase"},
    {5.0f, "scatter"}, {20.0f, "chase"},
    {5.0f, "scatter"}, {-1.0f, "chase"},
}};
constexpr float FRIGHT_BASE         = 6.0f;
constexpr float LEVEL_INTRO_T       = 2.0f;
constexpr float LIFE_LOST_T         = 1.8f;
constexpr float LEVEL_COMPLETE_T    = 1.8f;
constexpr float POWERUP_NO_SPAWN_EARLY = 10.0f;
constexpr float POWERUP_INTERVAL_LO = 12.0f;
constexpr float POWERUP_INTERVAL_HI = 22.0f;

// --- scoring ---
constexpr int SCORE_PELLET = 10;
constexpr int SCORE_ENERGIZER = 50;
constexpr std::array<int, 4> SCORE_GHOSTS = {200, 400, 800, 1600};
constexpr std::array<int, 8> FRUIT_TIERS  = {100, 300, 500, 700, 1000, 2000, 3000, 5000};
constexpr int EXTRA_LIFE_FIRST  = 10000;
constexpr int EXTRA_LIFE_REPEAT = 50000;

// --- lives ---
constexpr int DEFAULT_LIVES = 3;
constexpr int LIVES_CAP     = 5;

// --- multi-mode tunables ---
constexpr float SWAP_INTERSTITIAL_T = 2.5f;
constexpr int   COOP_DEFAULT_LIVES  = 5;
constexpr float COOP_SCATTER_CYCLE_T = 7.0f;
constexpr float COOP_RESCUE_WINDOW  = 4.0f;
constexpr float COOP_SPECTRAL_T     = 1.5f;

// --- scatter corners ---
struct ScatterTarget { int x, y; };
constexpr ScatterTarget SCATTER_BLINKY = {COLS - 3, 0};
constexpr ScatterTarget SCATTER_PINKY  = {2, 0};
constexpr ScatterTarget SCATTER_INKY   = {COLS - 1, ROWS - 1};
constexpr ScatterTarget SCATTER_CLYDE  = {0, ROWS - 1};

// --- power-ups ---
enum class PowerUp : std::uint8_t {
    None = 0, Cherry, Speed, Magnet, Freeze, Phase, ScoreX2, Heart
};

constexpr float powerup_duration(PowerUp p) {
    switch (p) {
        case PowerUp::Cherry:  return 0.0f;
        case PowerUp::Speed:   return 8.0f;
        case PowerUp::Magnet:  return 5.0f;
        case PowerUp::Freeze:  return 4.0f;
        case PowerUp::Phase:   return 5.0f;
        case PowerUp::ScoreX2: return 10.0f;
        default: return 0.0f;
    }
}

constexpr Color powerup_color(PowerUp p) {
    switch (p) {
        case PowerUp::Cherry:  return {240, 80, 80};
        case PowerUp::Speed:   return {255, 240, 100};
        case PowerUp::Magnet:  return {200, 100, 240};
        case PowerUp::Freeze:  return {120, 220, 255};
        case PowerUp::Phase:   return {200, 200, 255};
        case PowerUp::ScoreX2: return {255, 200, 80};
        case PowerUp::Heart:   return {255, 90, 130};
        default: return {255, 255, 255};
    }
}

constexpr std::string_view powerup_glyph(PowerUp p) {
    switch (p) {
        case PowerUp::Cherry:  return "C";
        case PowerUp::Speed:   return ">";
        case PowerUp::Magnet:  return "M";
        case PowerUp::Freeze:  return "*";
        case PowerUp::Phase:   return "P";
        case PowerUp::ScoreX2: return "x2";
        case PowerUp::Heart:   return "+";
        default: return "?";
    }
}

// --- directions ---
enum class Dir : std::uint8_t { None = 0, Up, Down, Left, Right };

constexpr int dx_of(Dir d) {
    switch (d) {
        case Dir::Left:  return -1;
        case Dir::Right: return 1;
        default:         return 0;
    }
}
constexpr int dy_of(Dir d) {
    switch (d) {
        case Dir::Up:   return -1;
        case Dir::Down: return 1;
        default:        return 0;
    }
}
constexpr Dir opposite(Dir d) {
    switch (d) {
        case Dir::Up:    return Dir::Down;
        case Dir::Down:  return Dir::Up;
        case Dir::Left:  return Dir::Right;
        case Dir::Right: return Dir::Left;
        default:         return Dir::None;
    }
}

} // namespace pac
