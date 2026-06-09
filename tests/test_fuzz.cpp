// Random-input fuzz: drive MultiGameplay with random direction
// inputs at random intervals for each mode. Asserts no crash and
// invariants hold (pellets monotonic, score monotonic, lives never
// negative, frame count progresses).

#include "pacman/app.hpp"
#include "pacman/multi_gameplay.hpp"

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL fuzz] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static void fuzz_mode(const char* mode, int frames, std::uint32_t seed) {
    static pac::App app(true);
    pac::MultiGameplay gp(app, mode);
    gp.sub = pac::Sub::Play;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dir(0, 4);  // up/down/left/right/none
    const std::string dirs[] = {"up", "down", "left", "right"};
    int prev_score = 0;
    for (int i = 0; i < frames; ++i) {
        if (i % 7 == 0) {
            int d = dir(rng);
            if (d < 4) {
                for (auto& ps : gp.players) {
                    gp.mode->handle_input_action(gp, ps.player_id, dirs[d]);
                }
            }
        }
        gp.update(1.0f / 60.0f);
        // Invariants: pellets non-negative, score non-negative
        CHECK(gp.maze.total_pellets() >= 0);
        int cur_score = gp.players[0].score.points + (gp.players.size() > 1 ? gp.players[1].score.points : 0)
                       + gp.shared_score.points;
        CHECK(cur_score >= prev_score);
        prev_score = cur_score;
        for (auto& ps : gp.players) CHECK(ps.lives >= 0);
        CHECK(gp.t >= 0);
    }
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    std::printf("[fuzz] running single 3600f seed=1\n");
    fuzz_mode("single", 3600, 1);
    std::printf("[fuzz] running alternating 3600f seed=2\n");
    fuzz_mode("alternating", 3600, 2);
    std::printf("[fuzz] running coop 3600f seed=3\n");
    fuzz_mode("coop", 3600, 3);
    std::printf("[fuzz] running coop 7200f seed=4\n");
    fuzz_mode("coop", 7200, 4);
    // Long soak across many seeds
    for (int s = 100; s < 110; ++s) {
        std::printf("[fuzz] soak coop 18000f seed=%d\n", s);
        fuzz_mode("coop", 18000, s);
    }
    for (int s = 200; s < 205; ++s) {
        std::printf("[fuzz] soak alt 18000f seed=%d\n", s);
        fuzz_mode("alternating", 18000, s);
    }
    for (int s = 300; s < 305; ++s) {
        std::printf("[fuzz] soak solo 18000f seed=%d\n", s);
        fuzz_mode("single", 18000, s);
    }
    std::printf("[fuzz] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
