// Mode-driven gameplay scene. Hosts solo / alternating / coop.
#pragma once

#include "ghost.hpp"
#include "input_router.hpp"
#include "maze.hpp"
#include "mode.hpp"

#include <memory>
#include <vector>

namespace pac {

class App;

enum class Sub : std::uint8_t {
    Intro, Play, Paused, LifeLost, LevelComplete, GameOver
};

class MultiGameplay {
public:
    App& app;
    Maze maze;
    std::vector<Maze> mazes;        // alternating uses per-player mazes
    std::vector<Ghost> ghosts;
    std::vector<PlayerState> players;
    std::unique_ptr<Mode> mode;
    int active_idx = 0;
    float t = 0.f;
    Sub sub = Sub::Intro;
    float sub_timer = LEVEL_INTRO_T;
    int mode_idx = 0;
    float mode_timer = SCATTER_CHASE[0].duration;
    const char* global_mode = "scatter";
    float fright_left = 0.f;
    int pellets_eaten_total = 0;
    int total_pellets_start = 0;
    // Score popups ("+200" floating up from a ghost eat etc)
    struct Popup { std::string text; float x, y; float life; Color color; };
    std::vector<Popup> popups;
    void add_popup(const std::string& txt, float x, float y, Color c) {
        popups.push_back({txt, x, y, 0.8f, c});
    }
    // PRD §8.2 fruit history (level numbers).
    std::vector<int> fruit_history;
    void record_fruit(int level) {
        fruit_history.push_back(level);
        if (fruit_history.size() > 7) fruit_history.erase(fruit_history.begin());
    }
    // coop shared
    Score shared_score;
    int shared_lives = 0;
    int team_level = 1;
    int clyde_target_id = 1;
    float clyde_timer = 0.f;
    bool rescue_revive_on = false;
    // alternating swap
    float swap_banner_t = 0.f;
    int swap_banner_from = 1;
    int swap_banner_to = 1;
    // Input is owned by App. We just borrow refs.

    MultiGameplay(App& app, const std::string& mode_name);

    void update(float dt);
    void handle_sdl_event(const SDL_Event& ev);

    // helpers used by Mode impls
    void tick_one_player(float dt, PlayerState& ps);
    void tick_two_players(float dt);
    void collisions_solo(PlayerState& ps);
    void collisions_coop();
    void apply_pickup_solo(PlayerState& ps, PowerUp k, float x, float y);
    void apply_pickup_coop(PlayerState& ps, PowerUp k, float x, float y);
    void die_solo(PlayerState& ps);
    void enter_level_complete();
    void enter_game_over();
    void next_level();
    void after_life_lost();
    int next_alive_idx() const;
    void reset_mode_timing();
    void tick_mode_timer(float dt);
    void magnet_pull(PlayerState& ps, float dt);
};

} // namespace pac
