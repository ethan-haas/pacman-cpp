// Coop-specific logic: ghost retargeting by name, power-up scope rules,
// rescue revive window, simultaneous death handling.

#include "pacman/app.hpp"
#include "pacman/multi_gameplay.hpp"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdio>

namespace pac { bool is_global_powerup(PowerUp p); }

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL co] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_coop_ghost_targeting_by_name() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    auto& p1 = gp.players[0];
    auto& p2 = gp.players[1];
    // Blinky targets the lower-contribution player
    p1.contribution = 100; p2.contribution = 50;
    auto blinky_iter = std::find_if(gp.ghosts.begin(), gp.ghosts.end(),
        [](const pac::Ghost& g) { return g.name == "blinky"; });
    CHECK(blinky_iter != gp.ghosts.end());
    auto* target = gp.mode->ghost_target_pac(gp, *blinky_iter);
    CHECK(target == &p2.pac);  // p2 lower contrib → blinky targets p2
    // Pinky always P1
    auto pinky_iter = std::find_if(gp.ghosts.begin(), gp.ghosts.end(),
        [](const pac::Ghost& g) { return g.name == "pinky"; });
    CHECK(gp.mode->ghost_target_pac(gp, *pinky_iter) == &p1.pac);
    // Inky always P2
    auto inky_iter = std::find_if(gp.ghosts.begin(), gp.ghosts.end(),
        [](const pac::Ghost& g) { return g.name == "inky"; });
    CHECK(gp.mode->ghost_target_pac(gp, *inky_iter) == &p2.pac);
}

static void test_coop_powerup_scope_global() {
    CHECK(pac::is_global_powerup(pac::PowerUp::Freeze));
    CHECK(pac::is_global_powerup(pac::PowerUp::ScoreX2));
    CHECK(pac::is_global_powerup(pac::PowerUp::Cherry));
}

static void test_coop_powerup_scope_solo() {
    CHECK(!pac::is_global_powerup(pac::PowerUp::Speed));
    CHECK(!pac::is_global_powerup(pac::PowerUp::Magnet));
    CHECK(!pac::is_global_powerup(pac::PowerUp::Phase));
}

static void test_coop_apply_freeze_affects_both() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    auto& p1 = gp.players[0];
    auto& p2 = gp.players[1];
    gp.apply_pickup_coop(p1, pac::PowerUp::Freeze, 0, 0);
    CHECK(p1.powerups.is_active(pac::PowerUp::Freeze));
    CHECK(p2.powerups.is_active(pac::PowerUp::Freeze));
}

static void test_coop_apply_speed_picker_only() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    gp.apply_pickup_coop(gp.players[0], pac::PowerUp::Speed, 0, 0);
    CHECK(gp.players[0].powerups.is_active(pac::PowerUp::Speed));
    CHECK(!gp.players[1].powerups.is_active(pac::PowerUp::Speed));
}

static void test_coop_heart_goes_to_picker() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    int p1_start = gp.players[0].lives;
    int p2_start = gp.players[1].lives;
    gp.apply_pickup_coop(gp.players[0], pac::PowerUp::Heart, 0, 0);
    CHECK(gp.players[0].lives == p1_start + 1);
    CHECK(gp.players[1].lives == p2_start);   // unchanged
}

static void test_coop_indep_lives_dont_share() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    CHECK(gp.players[0].lives == 3);
    CHECK(gp.players[1].lives == 3);
    gp.mode->on_player_death(gp, gp.players[0]);
    CHECK(gp.players[0].lives == 2);
    CHECK(gp.players[1].lives == 3);
    gp.mode->on_player_death(gp, gp.players[0]);
    CHECK(gp.players[0].lives == 1);
    gp.mode->on_player_death(gp, gp.players[0]);
    CHECK(gp.players[0].lives == 0);
    CHECK(gp.players[1].lives == 3);
    CHECK(!gp.mode->is_game_over(gp));   // P2 still has lives
}

static void test_coop_game_over_both_exhausted() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    for (int i = 0; i < 3; ++i) gp.mode->on_player_death(gp, gp.players[0]);
    for (int i = 0; i < 3; ++i) gp.mode->on_player_death(gp, gp.players[1]);
    CHECK(gp.mode->is_game_over(gp));
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    test_coop_ghost_targeting_by_name();
    test_coop_powerup_scope_global();
    test_coop_powerup_scope_solo();
    test_coop_apply_freeze_affects_both();
    test_coop_apply_speed_picker_only();
    test_coop_heart_goes_to_picker();
    test_coop_indep_lives_dont_share();
    test_coop_game_over_both_exhausted();
    std::printf("[coop_logic] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
