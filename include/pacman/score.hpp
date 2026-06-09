#pragma once
#include "constants.hpp"

namespace pac {

struct Score {
    int points = 0;
    int high = 0;
    int ghost_streak = 0;
    int last_extra = 0;
    float multiplier = 1.0f;

    void reset_streak() { ghost_streak = 0; }
    int fruit_value(int level) const;
    int award(int base);
    int eat_pellet()    { return award(SCORE_PELLET); }
    int eat_energizer() { return award(SCORE_ENERGIZER); }
    int eat_ghost();
    int eat_fruit(int level) { return award(fruit_value(level)); }
    int bonus(int p)         { return award(p); }
    bool check_extra_life(int& lives);  // returns true if life awarded
};

} // namespace pac
