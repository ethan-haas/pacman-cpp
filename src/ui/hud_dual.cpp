#include "pacman/multi_gameplay.hpp"
#include "pacman/render.hpp"

#include <string>

namespace pac {

// HUD bars are dedicated strips inside TV-safe area, ABOVE + BELOW the
// maze. HUD_BAR_H = 66 (from constants.hpp) — text fits cleanly without
// crossing the divider lines.
//
// Per-bar vertical layout (66 px tall):
//   y+4   : label (14 px font)
//   y+24  : value (24 px font)  → ends y+48, well inside 66 px bar
//   y+50  : second row of details (lives pips, power-ups)
//   y+66  : divider line
static constexpr Color STRIP_BG  = {16, 20, 50, 230};
static constexpr Color STRIP_HI  = {60, 120, 200};
static constexpr Color STRIP_PAD = {8, 10, 30, 230};   // outer padding

static constexpr int F_LBL = 14;
static constexpr int F_VAL = 24;
static constexpr int F_MID = 18;

static int text_w_(const std::string& s, int size) {
    int scale = std::max(1, size / 7);
    int gw = 5 * scale, sp = scale;
    int n = static_cast<int>(s.size());
    return n * (gw + sp) - sp;
}
static int right_x_(const std::string& s, int size, int right_edge) {
    return right_edge - text_w_(s, size);
}

static void strip_top_(SDL_Renderer* r) {
    fill_rect(r, SAFE_X0, HUD_BAR_TOP_Y, SAFE_X1 - SAFE_X0, HUD_BAR_H, STRIP_BG);
    // divider on bottom edge — separate from maze
    fill_rect(r, SAFE_X0, HUD_BAR_TOP_Y + HUD_BAR_H, SAFE_X1 - SAFE_X0, 2, STRIP_HI);
}
static void strip_bot_(SDL_Renderer* r) {
    fill_rect(r, SAFE_X0, HUD_BAR_BOT_Y, SAFE_X1 - SAFE_X0, HUD_BAR_H, STRIP_BG);
    // divider on top edge of bottom bar
    fill_rect(r, SAFE_X0, HUD_BAR_BOT_Y - 2, SAFE_X1 - SAFE_X0, 2, STRIP_HI);
}

static void draw_lives_pips_(SDL_Renderer* r, int x, int y, int lives, Color c, int max_show = 5) {
    for (int i = 0; i < std::min(lives, max_show); ++i) {
        int cx = x + i * 22;
        fill_circle(r, cx, y, 9, c);
    }
}

static void draw_powerup_row_(SDL_Renderer* r, int x, int y,
                              const std::unordered_map<PowerUp, float>& active) {
    int cur = x;
    for (auto& [k, left] : active) {
        Color c = powerup_color(k);
        fill_circle(r, cur, y, 11, c);
        fill_circle(r, cur, y, 6, {16, 20, 50});
        draw_text(r, std::string(powerup_glyph(k)), cur, y - 5, 12, {255, 255, 255}, true);
        cur += 28;
    }
}

void draw_solo_hud(SDL_Renderer* r, const PlayerState& ps, float /*t*/) {
    strip_top_(r);
    int top_lbl = HUD_BAR_TOP_Y + 6;
    int top_val = HUD_BAR_TOP_Y + 24;
    // LEFT: SCORE
    draw_text(r, "SCORE", SAFE_X0 + 12, top_lbl, F_LBL, TEXT_DIM);
    auto sscore = std::to_string(ps.score.points);
    draw_text(r, sscore, SAFE_X0 + 12, top_val, F_VAL, TEXT);
    // CENTER: HIGH
    draw_text(r, "HIGH", WIN_W / 2, top_lbl, F_LBL, TEXT_DIM, true);
    auto shigh = std::to_string(ps.score.high);
    draw_text(r, shigh, WIN_W / 2, top_val, F_VAL, P1_COLOR, true);
    // RIGHT: LEVEL
    auto slev = std::to_string(ps.level);
    int lbl_w = text_w_("LEVEL", F_LBL);
    draw_text(r, "LEVEL", SAFE_X1 - 12 - lbl_w, top_lbl, F_LBL, TEXT_DIM);
    draw_text(r, slev, right_x_(slev, F_VAL, SAFE_X1 - 12), top_val, F_VAL, TEXT);

    strip_bot_(r);
    int bot_lbl = HUD_BAR_BOT_Y + 8;
    int bot_pips = HUD_BAR_BOT_Y + 38;
    draw_text(r, "LIVES", SAFE_X0 + 12, bot_lbl, F_LBL, P1_COLOR);
    draw_lives_pips_(r, SAFE_X0 + 12, bot_pips, ps.lives, P1_COLOR);
    draw_text(r, "POWERUPS", SAFE_X1 - 12 - text_w_("POWERUPS", F_LBL), bot_lbl, F_LBL, TEXT_DIM);
    draw_powerup_row_(r, SAFE_X1 - 270, bot_pips, ps.powerups.active);
}

void draw_alternating_hud(SDL_Renderer* r, const PlayerState& p1, const PlayerState& p2,
                          int active_idx, float /*t*/) {
    strip_top_(r);
    int top_lbl = HUD_BAR_TOP_Y + 6;
    int top_val = HUD_BAR_TOP_Y + 24;
    auto half = [](Color c) {
        return Color{static_cast<std::uint8_t>(c.r / 2),
                     static_cast<std::uint8_t>(c.g / 2),
                     static_cast<std::uint8_t>(c.b / 2)};
    };
    {
        Color tc = (active_idx == 0) ? P1_COLOR : half(P1_COLOR);
        draw_text(r, "P1", SAFE_X0 + 12, top_lbl, F_LBL, tc);
        draw_text(r, std::to_string(p1.score.points), SAFE_X0 + 12, top_val, F_MID, tc);
        draw_text(r, "LV " + std::to_string(p1.level), SAFE_X0 + 180, top_val, F_MID, tc);
    }
    int act_id = active_idx == 0 ? p1.player_id : p2.player_id;
    Color act_col = active_idx == 0 ? P1_COLOR : P2_COLOR;
    draw_text(r, "ACTIVE P" + std::to_string(act_id), WIN_W / 2, top_val, F_MID, act_col, true);
    {
        Color tc = (active_idx == 1) ? P2_COLOR : half(P2_COLOR);
        auto sscore = std::to_string(p2.score.points);
        auto slv = "LV " + std::to_string(p2.level);
        draw_text(r, "P2", right_x_("P2", F_LBL, SAFE_X1 - 12), top_lbl, F_LBL, tc);
        draw_text(r, slv, right_x_(slv, F_MID, SAFE_X1 - 12), top_val, F_MID, tc);
        draw_text(r, sscore, right_x_(sscore, F_MID, SAFE_X1 - 160), top_val, F_MID, tc);
    }

    strip_bot_(r);
    int bot_lbl = HUD_BAR_BOT_Y + 8;
    int bot_pips = HUD_BAR_BOT_Y + 38;
    draw_text(r, "P1 LIVES", SAFE_X0 + 12, bot_lbl, F_LBL, P1_COLOR);
    draw_lives_pips_(r, SAFE_X0 + 12, bot_pips, p1.lives, P1_COLOR);
    draw_text(r, "P2 LIVES", right_x_("P2 LIVES", F_LBL, SAFE_X1 - 12), bot_lbl, F_LBL, P2_COLOR);
    draw_lives_pips_(r, SAFE_X1 - 130, bot_pips, p2.lives, P2_COLOR);
    const auto& ap = (active_idx == 0) ? p1 : p2;
    draw_powerup_row_(r, WIN_W / 2 - 100, bot_pips, ap.powerups.active);
}

void draw_coop_hud(SDL_Renderer* r, const PlayerState& p1, const PlayerState& p2,
                   int team_score, int shared_lives, int team_level, float /*t*/) {
    (void)shared_lives;
    strip_top_(r);
    int top_lbl = HUD_BAR_TOP_Y + 6;
    int top_val = HUD_BAR_TOP_Y + 24;
    // LEFT: TEAM SCORE
    draw_text(r, "TEAM", SAFE_X0 + 12, top_lbl, F_LBL, TEXT_DIM);
    auto sscore = std::to_string(team_score);
    draw_text(r, sscore, SAFE_X0 + 12, top_val, F_VAL, {255, 220, 60});
    // CENTER: P1 LIVES + P2 LIVES (the user-requested fix)
    int center_x = WIN_W / 2;
    int p1_lbl_x = center_x - 180;
    int p2_lbl_x = center_x + 40;
    draw_text(r, "P1 LIVES", p1_lbl_x, top_lbl, F_LBL, P1_COLOR);
    draw_lives_pips_(r, p1_lbl_x, top_val + 10, p1.lives, P1_COLOR);
    draw_text(r, "P2 LIVES", p2_lbl_x, top_lbl, F_LBL, P2_COLOR);
    draw_lives_pips_(r, p2_lbl_x, top_val + 10, p2.lives, P2_COLOR);
    // RIGHT: LEVEL
    auto slev = std::to_string(team_level);
    int lbl_w = text_w_("LEVEL", F_LBL);
    draw_text(r, "LEVEL", SAFE_X1 - 12 - lbl_w, top_lbl, F_LBL, TEXT_DIM);
    draw_text(r, slev, right_x_(slev, F_VAL, SAFE_X1 - 12), top_val, F_VAL, TEXT);

    strip_bot_(r);
    int bot_lbl = HUD_BAR_BOT_Y + 8;
    int bot_val = HUD_BAR_BOT_Y + 28;
    int bot_pips = HUD_BAR_BOT_Y + 50;
    // LEFT: P1 contribution + power-ups
    draw_text(r, "P1 CONTRIB", SAFE_X0 + 12, bot_lbl, F_LBL, P1_COLOR);
    draw_text(r, std::to_string(p1.contribution), SAFE_X0 + 12, bot_val, F_MID, P1_COLOR);
    draw_powerup_row_(r, SAFE_X0 + 220, bot_pips, p1.powerups.active);
    // RIGHT: P2 contribution + power-ups
    auto p2s = std::to_string(p2.contribution);
    int p2_lbl_w = text_w_("P2 CONTRIB", F_LBL);
    draw_text(r, "P2 CONTRIB", SAFE_X1 - 12 - p2_lbl_w, bot_lbl, F_LBL, P2_COLOR);
    draw_text(r, p2s, right_x_(p2s, F_MID, SAFE_X1 - 12), bot_val, F_MID, P2_COLOR);
    draw_powerup_row_(r, SAFE_X1 - 168, bot_pips, p2.powerups.active);
}

} // namespace pac
