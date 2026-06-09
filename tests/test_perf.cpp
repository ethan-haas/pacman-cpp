// Perf budget: assert update() per-frame stays under target.
// x86_64 host should be MUCH faster than 1.8 GHz Cortex-A53;
// rough rule: if x86_64 hits >0.2 ms/frame, ARM may struggle.

#include "pacman/app.hpp"
#include "pacman/multi_gameplay.hpp"

#include <SDL2/SDL.h>
#include <chrono>
#include <cstdio>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL perf] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

static double bench_mode(const char* mode, int frames) {
    static pac::App app(true);
    pac::MultiGameplay gp(app, mode);
    gp.sub = pac::Sub::Play;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < frames; ++i) gp.update(1.0f / 60.0f);
    auto t1 = std::chrono::steady_clock::now();
    double ms_per = std::chrono::duration<double, std::milli>(t1 - t0).count() / frames;
    std::printf("[perf] mode=%s frames=%d  %.3f ms/frame\n", mode, frames, ms_per);
    return ms_per;
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    // Budget: 16.67 ms/frame target (60 fps). On x86_64 host stay
    // well under 2 ms to leave headroom for A53 (~10x slower).
    constexpr double BUDGET_MS = 2.0;
    CHECK(bench_mode("single", 3600) < BUDGET_MS);
    CHECK(bench_mode("alternating", 3600) < BUDGET_MS);
    CHECK(bench_mode("coop", 3600) < BUDGET_MS);
    std::printf("[perf] %d passed, %d failed (budget %.1f ms/frame)\n",
                g_pass, g_fail, BUDGET_MS);
    return g_fail == 0 ? 0 : 1;
}
