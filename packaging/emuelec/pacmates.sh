#!/bin/sh
# EmuELEC port launcher: Pac-Mates Multiplayer
# Drop into /storage/roms/ports/ on the Kinhank Super Console X2 Pro.

APP_DIR="/storage/roms/ports/pacmates"
BIN="$APP_DIR/pacmates"
LOG="/storage/.cache/pacmates.log"

mkdir -p "$(dirname "$LOG")"

if [ ! -x "$BIN" ]; then
    echo "[pacmates] binary missing at $BIN" >> "$LOG"
    exit 1
fi

# EmuELEC v4.6 kmsdrm path + ALSA.
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-kmsdrm}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
export PACMAN_FULLSCREEN=1
if [ -f /storage/.config/SDL_GameControllerDB/gamecontrollerdb.txt ]; then
    export SDL_GAMECONTROLLERCONFIG_FILE="/storage/.config/SDL_GameControllerDB/gamecontrollerdb.txt"
fi

cd "$APP_DIR"
exec "$BIN" >> "$LOG" 2>&1
