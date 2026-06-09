#!/bin/bash
# Local Windows + Linux smoke battery. Headless. Validates each mode
# boots, ticks N frames, exits 0. Mirrors the Python pytest suite's
# multiplayer_smoke coverage for the C++ port.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/pacman-reimagined.exe"
[ -f "$BIN" ] || BIN="$ROOT/build/pacman-reimagined"
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 1; }

FRAMES="${FRAMES:-600}"
PASS=0
FAIL=0

run() {
    local label="$1"; shift
    if SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$BIN" "$@" >/dev/null 2>&1; then
        echo "PASS $label"
        PASS=$((PASS+1))
    else
        echo "FAIL $label"
        FAIL=$((FAIL+1))
    fi
}

run "boot-menu             " --headless --frames "$FRAMES"
run "mode=single            " --headless --mode single --frames "$FRAMES"
run "mode=alternating       " --headless --mode alternating --frames "$FRAMES"
run "mode=coop              " --headless --mode coop --frames "$FRAMES"
run "long-coop-3600         " --headless --mode coop --frames 3600
run "long-alt-3600          " --headless --mode alternating --frames 3600

echo
echo "RESULT: $PASS passed, $FAIL failed (frames=$FRAMES per short run)"
[ "$FAIL" -eq 0 ]
