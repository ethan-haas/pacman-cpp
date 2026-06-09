#include "pacman/score.hpp"

#include <algorithm>

namespace pac {

int Score::fruit_value(int level) const {
    int idx = std::min(level - 1, static_cast<int>(FRUIT_TIERS.size()) - 1);
    return FRUIT_TIERS[std::max(0, idx)];
}

int Score::award(int base) {
    int gained = static_cast<int>(base * multiplier);
    points += gained;
    if (points > high) high = points;
    return gained;
}

int Score::eat_ghost() {
    int idx = std::min(ghost_streak, static_cast<int>(SCORE_GHOSTS.size()) - 1);
    int val = SCORE_GHOSTS[idx];
    ++ghost_streak;
    return award(val);
}

bool Score::check_extra_life(int& lives) {
    if (last_extra == 0 && points >= EXTRA_LIFE_FIRST) {
        last_extra = EXTRA_LIFE_FIRST;
        if (lives < LIVES_CAP) { ++lives; return true; }
        return true;
    }
    int next = (last_extra >= EXTRA_LIFE_FIRST)
        ? last_extra + EXTRA_LIFE_REPEAT
        : EXTRA_LIFE_FIRST + EXTRA_LIFE_REPEAT;
    if (last_extra >= EXTRA_LIFE_FIRST && points >= next) {
        last_extra = next;
        if (lives < LIVES_CAP) { ++lives; return true; }
        return true;
    }
    return false;
}

} // namespace pac
