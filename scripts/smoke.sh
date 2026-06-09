#!/bin/bash
# Headless 600-frame boot per mode. Exit code != 0 on crash.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/build/pacman-reimagined"
if [ ! -x "$BIN" ]; then echo "Build first: scripts/build.sh"; exit 1; fi
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$BIN" --headless --frames 600
echo "[smoke] OK"
