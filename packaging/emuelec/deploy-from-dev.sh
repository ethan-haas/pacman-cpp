#!/bin/bash
# deploy-from-dev.sh — push aarch64 build + launcher to x2 Pro from dev box.
# Conforms to PRD §5.5/§5.7 — uses ed25519 key, dry-runs rsync first,
# stops emustation before touching gamelist, restarts after.
#
# Usage:
#   bash packaging/emuelec/deploy-from-dev.sh                # uses host emuelec
#   bash packaging/emuelec/deploy-from-dev.sh emuelec-x2pro  # explicit host

set -euo pipefail

HOST="${1:-emuelec}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [ ! -f build-arm64/pacman-reimagined-arm64 ]; then
    echo "[deploy] missing build-arm64/pacman-reimagined-arm64" >&2
    echo "[deploy] run packaging/emuelec/build-arm64.sh first" >&2
    exit 1
fi

PORT_DIR="/storage/roms/ports"
APP_DIR="$PORT_DIR/pacman-reimagined"

echo "[deploy] target host: $HOST"

# Pre-flight ssh
ssh -o BatchMode=yes "$HOST" "echo connected" >/dev/null || {
    echo "[deploy] ssh to $HOST failed; check ~/.ssh/config or use ssh-copy-id" >&2
    exit 1
}

# Backup
TS=$(date +%Y%m%d-%H%M%S)
echo "[deploy] backing up /storage/roms/ports/ -> /storage/.cache/pacman-backups/$TS/"
ssh "$HOST" "mkdir -p /storage/.cache/pacman-backups/$TS && \
             [ -d $PORT_DIR ] && cp -r $PORT_DIR /storage/.cache/pacman-backups/$TS/ports.bak || true"

# Dry-run rsync first (PRD §5.5 rule 7)
echo "[deploy] dry-run rsync..."
rsync -avzn --itemize-changes \
    build-arm64/pacman-reimagined-arm64 \
    packaging/emuelec/pacman-reimagined.sh \
    packaging/emuelec/gamelist.xml \
    "$HOST:/tmp/pacman-stage/"

read -r -p "[deploy] proceed with actual push? [y/N] " ans
[ "$ans" = "y" ] || [ "$ans" = "Y" ] || { echo "[deploy] aborted"; exit 0; }

# Stop ES
ssh "$HOST" "systemctl stop emustation || true"

# Push
ssh "$HOST" "mkdir -p $APP_DIR/images"
rsync -avz build-arm64/pacman-reimagined-arm64 "$HOST:$APP_DIR/pacman-reimagined-arm64"
rsync -avz packaging/emuelec/pacman-reimagined.sh "$HOST:$PORT_DIR/pacman-reimagined.sh"
ssh "$HOST" "chmod +x $APP_DIR/pacman-reimagined-arm64 $PORT_DIR/pacman-reimagined.sh"

# Gamelist merge
if ssh "$HOST" "[ -f $PORT_DIR/gamelist.xml ]"; then
    if ssh "$HOST" "grep -q pacman-reimagined.sh $PORT_DIR/gamelist.xml"; then
        echo "[deploy] gamelist.xml already has entry, skipping"
    else
        # use python3 on device for safe merge
        rsync -avz packaging/emuelec/gamelist.xml "$HOST:/tmp/pacman-gamelist.xml"
        ssh "$HOST" "python3 - <<'PY'
import re
src = open('/tmp/pacman-gamelist.xml').read()
dst_path = '$PORT_DIR/gamelist.xml'
dst = open(dst_path).read()
body = re.search(r'<gameList>(.*?)</gameList>', src, re.S).group(1)
open(dst_path, 'w').write(dst.replace('</gameList>', body + '\n</gameList>'))
print('merged')
PY"
    fi
else
    rsync -avz packaging/emuelec/gamelist.xml "$HOST:$PORT_DIR/gamelist.xml"
fi

# Restart
ssh "$HOST" "systemctl restart emustation"

# Verify (PRD §5.7)
sleep 3
ssh "$HOST" "systemctl is-active emustation"
ssh "$HOST" "journalctl -u emustation -n 50 --no-pager | grep -iE 'error|fail|segfault' || echo '[deploy] journal clean'"

echo "[deploy] DONE. Tile should appear under PORTS on $HOST."
