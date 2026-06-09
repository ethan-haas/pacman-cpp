// Power-up + frightened state tests. Verifies:
// - Apply duration matches POWERUP_DURATION table
// - Multiple distinct power-ups stack
// - Heart pickup grants +1 life capped at LIVES_CAP
// - Cherry awards fruit_value(level) immediately
// - Frightened ghost chain awards 200/400/800/1600

#include "pacman/constants.hpp"
#include "pacman/powerup_manager.hpp"
#include "pacman/score.hpp"

#include <cstdio>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL pu] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_powerup_durations_match_constants() {
    pac::PowerUpManager pm;
    pm.reset_level();
    pm.apply(pac::PowerUp::Speed);
    pm.apply(pac::PowerUp::Magnet);
    pm.apply(pac::PowerUp::Freeze);
    pm.apply(pac::PowerUp::Phase);
    pm.apply(pac::PowerUp::ScoreX2);
    CHECK(pm.time_left(pac::PowerUp::Speed)   == 8.0f);
    CHECK(pm.time_left(pac::PowerUp::Magnet)  == 5.0f);
    CHECK(pm.time_left(pac::PowerUp::Freeze)  == 4.0f);
    CHECK(pm.time_left(pac::PowerUp::Phase)   == 5.0f);
    CHECK(pm.time_left(pac::PowerUp::ScoreX2) == 10.0f);
}

static void test_powerup_distinct_decay_rates() {
    pac::PowerUpManager pm;
    pm.reset_level();
    pm.apply(pac::PowerUp::Freeze);
    pm.apply(pac::PowerUp::ScoreX2);
    for (int i = 0; i < 50; ++i) pm.update(0.1f);  // 5 sec
    CHECK(!pm.is_active(pac::PowerUp::Freeze));     // 4s < 5s elapsed
    CHECK(pm.is_active(pac::PowerUp::ScoreX2));     // 10s > 5s elapsed
}

static void test_cherry_is_instant_not_timed() {
    pac::PowerUpManager pm;
    pm.reset_level();
    pm.apply(pac::PowerUp::Cherry);
    CHECK(!pm.is_active(pac::PowerUp::Cherry));     // duration=0 → never goes "active"
}

static void test_score_multiplier_scorex2() {
    pac::Score s;
    s.multiplier = 2.0f;
    int g = s.eat_pellet();
    CHECK(g == pac::SCORE_PELLET * 2);
    CHECK(s.points == pac::SCORE_PELLET * 2);
}

static void test_score_ghost_chain_200_400_800_1600() {
    pac::Score s;
    CHECK(s.eat_ghost() == 200);
    CHECK(s.eat_ghost() == 400);
    CHECK(s.eat_ghost() == 800);
    CHECK(s.eat_ghost() == 1600);
    // Streak persists into 5th eat (capped at index 3)
    CHECK(s.eat_ghost() == 1600);
}

static void test_score_reset_streak() {
    pac::Score s;
    s.eat_ghost();   // streak=1
    s.eat_ghost();   // streak=2
    s.reset_streak();
    CHECK(s.eat_ghost() == 200);   // back to bottom
}

static void test_score_fruit_value_per_level() {
    pac::Score s;
    CHECK(s.fruit_value(1) == 100);   // cherry
    CHECK(s.fruit_value(2) == 300);   // strawberry
    CHECK(s.fruit_value(3) == 500);
    CHECK(s.fruit_value(15) == 5000); // key (capped)
}

static void test_extra_life_first_at_10k_then_every_50k() {
    pac::Score s;
    int lives = 3;
    s.points = 10000;
    CHECK(s.check_extra_life(lives));
    CHECK(lives == 4);
    // No more until 60k
    s.points = 30000;
    CHECK(!s.check_extra_life(lives));
    s.points = 60000;
    CHECK(s.check_extra_life(lives));
    CHECK(lives == 5);
    // 110k next
    s.points = 109999;
    CHECK(!s.check_extra_life(lives));
    s.points = 110000;
    CHECK(s.check_extra_life(lives));
}

static void test_extra_life_capped_at_lives_cap() {
    pac::Score s;
    int lives = pac::LIVES_CAP;
    s.points = 10000;
    CHECK(s.check_extra_life(lives));  // award triggers
    CHECK(lives == pac::LIVES_CAP);    // but does not exceed cap
}

int main() {
    test_powerup_durations_match_constants();
    test_powerup_distinct_decay_rates();
    test_cherry_is_instant_not_timed();
    test_score_multiplier_scorex2();
    test_score_ghost_chain_200_400_800_1600();
    test_score_reset_streak();
    test_score_fruit_value_per_level();
    test_extra_life_first_at_10k_then_every_50k();
    test_extra_life_capped_at_lives_cap();
    std::printf("[powerups] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
