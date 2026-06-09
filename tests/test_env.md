# WSL Test Environment

Native x86_64 Linux build under WSL Ubuntu — for debug + iteration
without flashing the x2 device.

## Build

```bash
wsl -d Ubuntu
cd /mnt/c/Users/ethan/Documents/AgentA/AgentA/workspace/PACMAN_CPP
cmake -S . -B build-native -G Ninja
cmake --build build-native -j
```

Produces:
- `build-native/pacman-reimagined` — game binary (x86_64 Linux)
- `build-native/pacman-tests`      — unit tests
- `build-native/pacman-sim-tests`  — full-scene simulation tests
- `build-native/pacman-fuzz-tests` — random-input invariant fuzz

## Run

```bash
# All tests via CTest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    ctest --test-dir build-native --output-on-failure

# Headless smoke (3600 frames per mode)
bash tests/smoke_modes.sh

# Direct mode run (60s headless)
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    ./build-native/pacman-reimagined --headless --mode coop --frames 3600

# GUI run (needs WSLg display server)
./build-native/pacman-reimagined --mode coop
```

## Current coverage

| Suite | Cases | Lines |
|---|---|---|
| `pacman-tests` (unit) | 91 | maze, ghosts, score, powerups, pacman entity |
| `pacman-sim-tests` (scene) | 22 | mode timer transitions, ghost-house release, coop lives, fruit history |
| `pacman-fuzz-tests` (fuzz) | 86 400 invariant checks | random input across single/alternating/coop, 18 000 ticks total |
| Smoke per mode | 3 × 60s | full boot + 3600 frames headless |

Total: **86 513 checks** — all pass.

## Debug recipes

### gdb on crash

```bash
gdb --args ./build-native/pacman-reimagined --mode coop
(gdb) run
(gdb) bt           # backtrace on segfault
```

### Address sanitiser build

```bash
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address -g" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
cmake --build build-asan -j
./build-asan/pacman-fuzz-tests   # asan flags any UB
```

### Add new test case

Add to `tests/test_simulation.cpp` (scene-level) or `tests/test_game.cpp`
(pure logic). Rebuild: `cmake --build build-native -j -t pacman-sim-tests`.
