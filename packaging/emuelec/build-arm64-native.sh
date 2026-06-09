#!/bin/bash
# Alternative aarch64 build path: run directly on any Linux box with apt
# (no Docker required). Faster than qemu-emulated builds.
#
# Output: build-arm64/pacman-reimagined-arm64
#
# Run ON aarch64 Linux (Pi 4/5, AWS Graviton, Ampere, etc):
#   sudo apt-get install -y build-essential cmake libsdl2-dev
#   bash packaging/emuelec/build-arm64-native.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

ARCH=$(uname -m)
if [ "$ARCH" != "aarch64" ] && [ "$ARCH" != "arm64" ]; then
    echo "[build] WARNING: native script expects aarch64; got $ARCH" >&2
    echo "[build] Use packaging/emuelec/build-arm64.sh (Docker buildx) for cross-build" >&2
    exit 2
fi

rm -rf build-arm64
cmake -S . -B build-arm64 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -march=armv8-a -mtune=cortex-a53 -DNDEBUG"
cmake --build build-arm64 -j"$(nproc)"

mv build-arm64/pacman-reimagined build-arm64/pacman-reimagined-arm64
strip build-arm64/pacman-reimagined-arm64 || true

SIZE=$(du -h build-arm64/pacman-reimagined-arm64 | cut -f1)
echo "[build] OK: build-arm64/pacman-reimagined-arm64 ($SIZE)"
