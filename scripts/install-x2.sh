#!/bin/bash
# Deploy native C++ build to x2 superconsole, then register EmulationStation
# launcher tiles (reusing the launcher metadata from ../PACMAN_MULTIPLAYER/packaging/).
#
# Run on the x2:
#   sudo bash scripts/install-x2.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PY_PKG="$(cd "$ROOT/../PACMAN_MULTIPLAYER" && pwd)"   # for the shared launcher files

APP_DIR="/userdata/apps/pacman-reimagined"
PORTS_DIR="/userdata/roms/ports"

if [ ! -x "$ROOT/build/pacman-reimagined" ]; then
    echo "[install] No build/. Run scripts/build.sh first." >&2
    exit 1
fi

mkdir -p "$APP_DIR"
rm -rf "$APP_DIR"/*
cp "$ROOT/build/pacman-reimagined" "$APP_DIR/"
chmod +x "$APP_DIR/pacman-reimagined"

# Reuse launcher shim + companion + gamelist from Python packaging.
mkdir -p "$PORTS_DIR/.images"
if [ -d "$PY_PKG/packaging/launcher/emulationstation/ports" ]; then
    cp "$PY_PKG/packaging/launcher/emulationstation/ports/pacman-reimagined.sh" "$PORTS_DIR/"
    cp "$PY_PKG/packaging/launcher/emulationstation/ports/pacman-original.sh"  "$PORTS_DIR/"
    chmod +x "$PORTS_DIR/pacman-reimagined.sh" "$PORTS_DIR/pacman-original.sh"
    if [ ! -f "$PORTS_DIR/gamelist.xml" ]; then
        cp "$PY_PKG/packaging/launcher/emulationstation/ports/gamelist.xml" "$PORTS_DIR/"
    fi
fi

# Restart EmulationStation
if command -v batocera-es-swissknife >/dev/null 2>&1; then
    batocera-es-swissknife --restart || true
elif systemctl is-active --quiet emulationstation 2>/dev/null; then
    systemctl restart emulationstation || true
fi

echo "[install] DONE. C++ binary at $APP_DIR/pacman-reimagined"
echo "         Tile 'PAC-MAN Reimagined' now points at the native build."
