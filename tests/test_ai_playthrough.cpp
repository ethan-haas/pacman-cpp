// AI playthrough: greedy bot drives Pacman toward nearest pellet via
// 4-direction lookahead. Asserts game makes progress without crashing.
//
// This catches systemic bugs (pacman gets stuck, ghosts trap immediately,
// score never advances, infinite-loop in update) that fuzz might miss.

#include "pacman/app.hpp"
#include "pacman/multi_gameplay.hpp"

#include <SDL2/SDL.h>
#include <cmath>
#include <cstdio>
#include <queue>
#include <string>
#include <unordered_set>

static int g_pass = 0;
static int g_fail = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::fprintf(stderr, \
    "[FAIL ai] %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

// BFS find nearest pellet from pacman tile. Returns first direction to step.
static std::string nearest_pellet_dir(const pac::MultiGameplay& gp, const pac::Pacman& p) {
    int sx = static_cast<int>(std::round(p.x)), sy = static_cast<int>(std::round(p.y));
    if (gp.maze.is_wall(sx, sy, false)) return "left";   // shouldn't happen
    struct Node { int x, y; std::string first; };
    std::queue<Node> q;
    std::unordered_set<long long> seen;
    auto key = [](int x, int y) { return (long long)x * 1000 + y; };
    q.push({sx, sy, ""});
    seen.insert(key(sx, sy));
    const std::pair<int, int> ds[4] = {{0,-1},{0,1},{-1,0},{1,0}};
    const char* names[4] = {"up","down","left","right"};
    int budget = 600;
    while (!q.empty() && budget-- > 0) {
        auto [x, y, first] = q.front(); q.pop();
        // pellet here?
        if (gp.maze.pellets().count({x, y}) || gp.maze.energizers().count({x, y})) {
            if (!first.empty()) return first;
        }
        for (int i = 0; i < 4; ++i) {
            int nx = x + ds[i].first, ny = y + ds[i].second;
            // tunnel wrap
            if (gp.maze.tunnel_row(ny)) {
                if (nx < 0) nx = pac::COLS - 1;
                else if (nx >= pac::COLS) nx = 0;
            }
            if (nx < 0 || nx >= pac::COLS || ny < 0 || ny >= pac::ROWS) continue;
            if (gp.maze.is_wall(nx, ny, false)) continue;
            if (seen.count(key(nx, ny))) continue;
            seen.insert(key(nx, ny));
            q.push({nx, ny, first.empty() ? names[i] : first});
        }
    }
    return "left";
}

static void test_solo_bot_makes_progress() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "single");
    gp.sub = pac::Sub::Play;
    int initial = gp.maze.total_pellets();
    int score_milestones = 0;
    for (int frame = 0; frame < 60 * 60; ++frame) {   // 60 seconds
        // Re-evaluate every 6 frames (matches input poll cadence)
        if (frame % 6 == 0 && gp.players[0].pac.alive) {
            auto d = nearest_pellet_dir(gp, gp.players[0].pac);
            gp.mode->handle_input_action(gp, 1, d);
        }
        gp.update(1.0f / 60.0f);
        if (gp.players[0].score.points >= (score_milestones + 1) * 100) {
            ++score_milestones;
        }
        if (gp.sub == pac::Sub::GameOver) break;
    }
    int final_pellets = gp.maze.total_pellets();
    std::printf("[ai] solo: ate %d pellets, score=%d, milestones=%d\n",
                initial - final_pellets, gp.players[0].score.points, score_milestones);
    CHECK(initial - final_pellets > 20);     // made meaningful progress
    CHECK(gp.players[0].score.points > 200); // score advances
    CHECK(score_milestones > 2);
}

static void test_coop_two_bots_make_progress() {
    static pac::App app(true);
    pac::MultiGameplay gp(app, "coop");
    gp.sub = pac::Sub::Play;
    int initial = gp.maze.total_pellets();
    for (int frame = 0; frame < 60 * 60; ++frame) {
        if (frame % 6 == 0) {
            for (auto& ps : gp.players) {
                if (ps.pac.alive) {
                    auto d = nearest_pellet_dir(gp, ps.pac);
                    gp.mode->handle_input_action(gp, ps.player_id, d);
                }
            }
        }
        gp.update(1.0f / 60.0f);
        if (gp.sub == pac::Sub::GameOver) break;
    }
    int eaten = initial - gp.maze.total_pellets();
    std::printf("[ai] coop: ate %d pellets, team_score=%d\n",
                eaten, gp.shared_score.points);
    CHECK(eaten > 20);
    CHECK(gp.shared_score.points > 200);
}

int main() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    test_solo_bot_makes_progress();
    test_coop_two_bots_make_progress();
    std::printf("[ai] %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
