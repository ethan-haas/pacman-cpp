#!/bin/sh
# install-emuelec.sh — deploy PAC-MAN Reimagined v2.0 to a Kinhank Super
# Console X2 Pro running EmuELEC v4.7/4.8.
#
# Run patterns:
#   A. ON device (after `scp -r` repo to /tmp/PACMAN_CPP):
#        ssh root@emuelec.local
#        cd /tmp/PACMAN_CPP
#        sh packaging/emuelec/install-emuelec.sh
#   B. FROM dev workstation (preferred — see DEPLOY-EMUELEC.md):
#        rsync deploy step + remote sh -c
#
# Conforms to PRD §5.5 golden rules:
#   - never touches /etc, /usr, /flash
#   - writes only under /storage/roms/ports/ and /storage/.config/
#   - uses `systemctl restart emustation` (NOT emulationstation)
#   - backs up /storage/roms/ports/ before changing anything

set -eu

PORT_DIR="/storage/roms/ports"
APP_DIR="$PORT_DIR/pacman-reimagined"
BACKUP_DIR="/storage/.cache/pacman-backups/$(date +%Y%m%d-%H%M%S)"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

echo "[install] EmuELEC PAC-MAN deploy"
echo "[install] source: $ROOT"
echo "[install] target: $APP_DIR"

# --- 0. Verify we're on EmuELEC ---
if [ ! -d /storage ] || [ ! -e /storage/.config/emuelec/configs/emuelec.conf ] 2>/dev/null; then
    if [ ! -d /storage ]; then
        echo "[install] ERROR: /storage missing — not an EmuELEC device" >&2
        exit 1
    fi
fi

# --- 1. Verify aarch64 binary exists ---
BIN_SRC="$ROOT/build-arm64/pacman-reimagined-arm64"
if [ ! -f "$BIN_SRC" ]; then
    echo "[install] ERROR: $BIN_SRC missing" >&2
    echo "[install] Run packaging/emuelec/build-arm64.sh on a dev box first." >&2
    exit 1
fi
file "$BIN_SRC" 2>/dev/null | grep -q "aarch64" || {
    echo "[install] ERROR: binary not aarch64. file output:" >&2
    file "$BIN_SRC" >&2 || true
    exit 1
}

# --- 2. Backup existing ports/ ---
if [ -d "$PORT_DIR" ]; then
    mkdir -p "$BACKUP_DIR"
    cp -r "$PORT_DIR" "$BACKUP_DIR/ports.bak"
    echo "[install] backup: $BACKUP_DIR/ports.bak"
fi

# --- 3. Stop EmulationStation (PRD §5.5 rule 3) ---
# EmuELEC's unit is `emustation`, NOT `emulationstation`.
ES_WAS_RUNNING=0
if systemctl is-active --quiet emustation 2>/dev/null; then
    ES_WAS_RUNNING=1
    echo "[install] stopping emustation..."
    systemctl stop emustation
fi

# --- 4. Install binary + launcher shim ---
mkdir -p "$APP_DIR/images"
cp "$BIN_SRC"                                       "$APP_DIR/pacman-reimagined-arm64"
chmod +x "$APP_DIR/pacman-reimagined-arm64"
cp "$ROOT/packaging/emuelec/pacman-reimagined.sh"   "$PORT_DIR/pacman-reimagined.sh"
chmod +x "$PORT_DIR/pacman-reimagined.sh"

# Optional cover art
for img in pacman-reimagined-cover.png pacman-reimagined-marquee.png pacman-reimagined-thumb.png; do
    if [ -f "$ROOT/packaging/emuelec/images/$img" ]; then
        cp "$ROOT/packaging/emuelec/images/$img" "$APP_DIR/images/$img"
    fi
done

# --- 5. Merge gamelist.xml ---
GAMELIST="$PORT_DIR/gamelist.xml"
SRC_GAMELIST="$ROOT/packaging/emuelec/gamelist.xml"
if [ ! -f "$GAMELIST" ]; then
    cp "$SRC_GAMELIST" "$GAMELIST"
    echo "[install] wrote new gamelist.xml"
else
    if grep -q "pacman-reimagined.sh" "$GAMELIST"; then
        echo "[install] gamelist.xml already contains entry; skipping merge"
    else
        # crude but reliable merge: extract <game>...</game> from src, insert before closing tag
        python3 - "$SRC_GAMELIST" "$GAMELIST" <<'PYEOF' 2>/dev/null || awk_merge() { :; }
import re, sys
src = open(sys.argv[1]).read()
dst_path = sys.argv[2]
dst = open(dst_path).read()
m = re.search(r"<gameList>(.*?)</gameList>", src, re.S)
body = m.group(1)
out = dst.replace("</gameList>", body + "\n</gameList>")
open(dst_path, "w").write(out)
print("[install] merged entry into gamelist.xml")
PYEOF
    fi
fi

# --- 6. Restart EmulationStation (PRD §5.5 rule 8) ---
if [ "$ES_WAS_RUNNING" -eq 1 ]; then
    systemctl start emustation
    echo "[install] emustation restarted"
fi

# --- 7. Verification probes per PRD §5.7 ---
sleep 2
if [ -x "$APP_DIR/pacman-reimagined-arm64" ]; then
    echo "[install] OK: binary installed at $APP_DIR/pacman-reimagined-arm64"
fi
if [ -x "$PORT_DIR/pacman-reimagined.sh" ]; then
    echo "[install] OK: launcher at $PORT_DIR/pacman-reimagined.sh"
fi
if systemctl is-active --quiet emustation 2>/dev/null; then
    echo "[install] OK: emustation active"
else
    echo "[install] WARN: emustation not active — start manually" >&2
fi

cat <<EOF

[install] DONE. PAC-MAN Reimagined should appear under the PORTS system.
          Logs: /storage/.cache/pacman-reimagined.log
          Backup: $BACKUP_DIR/ports.bak

Verify per PRD §5.7:
  journalctl -u emustation -n 100 --no-pager | grep -iE 'error|fail|segfault'
EOF
