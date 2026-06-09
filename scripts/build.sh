#!/bin/bash
# build.sh — native build of PAC-MAN Reimagined C++ port for x2 superconsole.
#
# Run ON the x2 (or matching-arch Linux box). No cross-compile.
#
# Prereqs (Batocera / Debian-based):
#   apt-get install -y build-essential cmake libsdl2-dev libsdl2-ttf-dev
#
# Output: build/pacman-reimagined

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "[build] arch:       $(uname -m)"
echo "[build] compiler:   $(${CXX:-g++} --version | head -n1)"
echo "[build] sdl2:       $(pkg-config --modversion sdl2)"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

BIN="$ROOT/build/pacman-reimagined"
if [ -x "$BIN" ]; then
    SIZE=$(du -h "$BIN" | cut -f1)
    echo "[build] OK: $BIN ($SIZE)"
else
    echo "[build] ERROR: binary not produced" >&2
    exit 1
fi
