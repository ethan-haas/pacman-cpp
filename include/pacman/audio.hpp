// Procedural 8-bit-style audio: SDL audio callback synthesises square
// waves on the fly. No WAV files shipped. Stateful — schedule short
// tones via play_*() and the callback mixes them.
#pragma once

#include <SDL2/SDL.h>

namespace pac {

class Audio {
public:
    Audio();
    ~Audio();

    void play_pellet();           // alt 2 pitches: waka / waka
    void play_energizer();        // descending sweep
    void play_ghost_eat();        // ascending fanfare
    void play_death();             // long downward sweep
    void play_extra_life();        // 3-note jingle
    void play_level_clear();       // 2-note fanfare
    void play_fruit();             // short chirp

private:
    SDL_AudioDeviceID dev_ = 0;
    bool ok_ = false;
};

extern Audio* g_audio;  // simple singleton — null if init failed

} // namespace pac
