#include "pacman/app.hpp"

#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    bool headless = false;
    int max_frames = -1;
    std::string mode;  // empty = start at main menu
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0) headless = true;
        else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            // hand-roll atoi to avoid GLIBC_2.38 __isoc23_strtol pull-in
            const char* s = argv[++i];
            int v = 0;
            while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); ++s; }
            max_frames = v;
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            mode = argv[++i];
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::printf("Usage: pacman-reimagined [--headless] [--frames N] [--mode single|alternating|coop]\n");
            return 0;
        }
    }
    pac::App app(headless);
    if (!mode.empty()) {
        if (mode == "single" || mode == "alternating" || mode == "coop") {
            app.start_multi_game(mode);
        } else {
            std::fprintf(stderr, "unknown mode: %s\n", mode.c_str());
            return 2;
        }
    }
    app.run(max_frames);
    return 0;
}
