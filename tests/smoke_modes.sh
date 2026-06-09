#!/bin/bash
# Headless smoke per mode. Picks build-native for x86_64, build-arm64 for ARM.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build-native/pacman-reimagined"
[ -x "$BIN" ] || BIN="$ROOT/build/pacman-reimagined.exe"
[ -x "$BIN" ] || { echo "no binary"; exit 1; }
FAIL=0
for m in single alternating coop; do
    if SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$BIN" --headless --mode "$m" --frames 3600 >/dev/null 2>&1; then
        echo "PASS mode=$m frames=3600"
    else
        echo "FAIL mode=$m"
        FAIL=$((FAIL+1))
    fi
done
echo "smoke_modes: $FAIL failure(s)"
[ "$FAIL" -eq 0 ]
