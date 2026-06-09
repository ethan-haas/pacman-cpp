# PAC-MAN Reimagined (C++ / SDL2)

A native **C++20 + SDL2** Pac-Man — co-op multiplayer, procedural chiptune audio, and a full
CMake build that targets both desktop and an **ARM aarch64 retro handheld** (EmuELEC). ~3,000
lines of modern C++, built with sanitizer / coverage / strict-warning presets and an automated
test suite. Not a tutorial port — a from-scratch game with its own architecture.

## Highlights
- **Modern C++20 + SDL2**, no game engine — hand-written game loop, rendering, input, and audio.
- **Co-op multiplayer** — two Pac-Men plus the four classic ghost AIs (chase / scatter / frightened).
- **Procedural chiptune** — a software audio mixer synthesizes sound in code (no audio asset files).
- **Cross-platform build** — desktop (native) and **ARM aarch64** (Amlogic S905X2 handheld via
  EmuELEC); CMake toolchain files for cross-compilation, plus device packaging scripts.
- **Engineering rigor** — separate CMake build presets for AddressSanitizer, coverage,
  strict warnings, and native/ARM targets; automated tests under `tests/`.

## Build & run (desktop)
```bash
cmake -S . -B build-native
cmake --build build-native
./build-native/pacman          # adjust to the actual target/exe name
```

## Tests
```bash
ctest --test-dir build-native
```

## Layout
- `src/`, `include/` — game source (loop, rendering, ghost AI, audio mixer, game state)
- `tests/` — automated test suite
- `cmake/`, `packaging/`, `scripts/` — cross-compile toolchain + device deploy
- `assets/` — game data
- `CMakeLists.txt` — build configuration (multiple presets)

## Why it's here
Modern C++ game built from scratch — game loop, enemy AI, collision, a procedural audio mixer —
cross-compiled to real ARM hardware, with sanitizer/coverage/strict build modes and tests. A
clean demonstration of systems-level engineering.

## Stack
C++20 · SDL2 · CMake.
