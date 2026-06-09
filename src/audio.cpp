#include "pacman/audio.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace pac {

namespace {

constexpr int SAMPLE_RATE = 22050;
constexpr int N_VOICES = 4;

struct Voice {
    bool active = false;
    float freq_hz = 440.0f;       // starting freq
    float freq_target = 440.0f;   // sweep target
    float phase = 0.0f;
    int samples_left = 0;
    int total_samples = 0;
    float amp = 0.20f;
};

std::mutex g_mu;
Voice g_voices[N_VOICES];

void mix_callback(void* /*user*/, Uint8* stream, int len) {
    std::int16_t* out = reinterpret_cast<std::int16_t*>(stream);
    int n_samples = len / 2;
    std::memset(stream, 0, len);

    std::lock_guard<std::mutex> lock(g_mu);
    for (auto& v : g_voices) {
        if (!v.active) continue;
        for (int i = 0; i < n_samples && v.samples_left > 0; ++i) {
            // Sweep frequency linearly toward target
            float t = 1.0f - static_cast<float>(v.samples_left) / v.total_samples;
            float f = v.freq_hz * (1.0f - t) + v.freq_target * t;
            v.phase += f / SAMPLE_RATE;
            if (v.phase >= 1.0f) v.phase -= 1.0f;
            // 50% square wave
            float s = (v.phase < 0.5f) ? 1.0f : -1.0f;
            // Envelope: fade in/out
            float env = 1.0f;
            int pos = v.total_samples - v.samples_left;
            if (pos < 200) env = pos / 200.0f;
            else if (v.samples_left < 200) env = v.samples_left / 200.0f;
            int sample = static_cast<int>(s * v.amp * env * 32000);
            int mix = out[i] + sample;
            if (mix > 32767) mix = 32767;
            if (mix < -32768) mix = -32768;
            out[i] = static_cast<std::int16_t>(mix);
            --v.samples_left;
        }
        if (v.samples_left <= 0) v.active = false;
    }
}

void play_tone(float f_start, float f_end, int duration_ms, float amp) {
    std::lock_guard<std::mutex> lock(g_mu);
    for (auto& v : g_voices) {
        if (v.active) continue;
        v.active = true;
        v.freq_hz = f_start;
        v.freq_target = f_end;
        v.phase = 0;
        v.total_samples = (duration_ms * SAMPLE_RATE) / 1000;
        v.samples_left = v.total_samples;
        v.amp = amp;
        return;
    }
    // All voices busy — drop the request rather than tearing
}

} // namespace

Audio* g_audio = nullptr;

Audio::Audio() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::fprintf(stderr, "[audio] SDL_InitSubSystem AUDIO failed: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want{}, have{};
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = mix_callback;
    want.userdata = nullptr;
    dev_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (dev_ == 0) {
        std::fprintf(stderr, "[audio] SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(dev_, 0);
    ok_ = true;
    g_audio = this;
    std::fprintf(stderr, "[audio] opened %d Hz mono S16\n", have.freq);
}

Audio::~Audio() {
    if (dev_) {
        SDL_PauseAudioDevice(dev_, 1);
        SDL_CloseAudioDevice(dev_);
    }
    g_audio = nullptr;
}

void Audio::play_pellet() {
    static bool alt = false;
    alt = !alt;
    play_tone(alt ? 880.f : 660.f, alt ? 880.f : 660.f, 30, 0.10f);
}

void Audio::play_energizer() {
    play_tone(880.f, 220.f, 350, 0.20f);
}

void Audio::play_ghost_eat() {
    play_tone(220.f, 880.f, 200, 0.25f);
}

void Audio::play_death() {
    play_tone(660.f, 110.f, 800, 0.30f);
}

void Audio::play_extra_life() {
    play_tone(660.f, 880.f, 120, 0.25f);
    play_tone(880.f, 1320.f, 180, 0.20f);
}

void Audio::play_level_clear() {
    play_tone(523.f, 660.f, 150, 0.25f);
    play_tone(660.f, 880.f, 200, 0.25f);
}

void Audio::play_fruit() {
    play_tone(1200.f, 1500.f, 80, 0.18f);
}

} // namespace pac
