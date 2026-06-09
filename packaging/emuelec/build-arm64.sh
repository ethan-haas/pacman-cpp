#!/bin/bash
# Cross-compile aarch64 binary on x86_64 host via Docker buildx + qemu.
#
# Output: ./build-arm64/pacman-reimagined-arm64
#
# One-time host setup:
#   docker run --privileged --rm tonistiigi/binfmt --install all

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if ! docker buildx version >/dev/null 2>&1; then
    echo "[build] docker buildx required" >&2
    exit 1
fi

IMG="pacman-cpp-arm64-builder"
docker buildx build \
    --platform=linux/arm64 \
    -f packaging/emuelec/Dockerfile.aarch64 \
    -t "$IMG" \
    --load \
    .

# extract binary
mkdir -p build-arm64
CID=$(docker create "$IMG")
trap "docker rm -f $CID >/dev/null 2>&1 || true" EXIT
docker cp "$CID:/pacman-reimagined-arm64" build-arm64/

# report
SIZE=$(du -h build-arm64/pacman-reimagined-arm64 | cut -f1)
echo "[build] aarch64 binary: build-arm64/pacman-reimagined-arm64  ($SIZE)"
file build-arm64/pacman-reimagined-arm64 2>/dev/null || true
