#!/bin/sh
# EmuELEC port launcher for PAC-MAN Reimagined v2.0.
# Drop into /storage/roms/ports/ on the Kinhank Super Console X2 Pro.
# EmulationStation reads this file as a "ports" entry per gamelist.xml.

APP_DIR="/storage/roms/ports/pacman-reimagined"
BIN="$APP_DIR/pacman-reimagined-arm64"
LOG="/storage/.cache/pacman-reimagined.log"

mkdir -p "$(dirname "$LOG")"

if [ ! -x "$BIN" ]; then
    echo "[pacman] binary missing at $BIN" >> "$LOG"
    echo "[pacman] run packaging/emuelec/install-emuelec.sh first" >> "$LOG"
    exit 1
fi

# EmuELEC kmsdrm path. Audio via alsathread per PRD §6.2.
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-kmsdrm}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
export PACMAN_FULLSCREEN=1
# SDL controller DB if user installed one
if [ -f /storage/.config/SDL_GameControllerDB/gamecontrollerdb.txt ]; then
    export SDL_GAMECONTROLLERCONFIG_FILE="/storage/.config/SDL_GameControllerDB/gamecontrollerdb.txt"
fi

# CPU governor already pinned by /storage/.config/custom_start.sh (PRD §4.3);
# inherit performance governor — no per-launch knob needed.

cd "$APP_DIR"
exec "$BIN" >> "$LOG" 2>&1
