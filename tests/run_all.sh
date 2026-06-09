#!/bin/bash
# All-in-one: native ctest + ASAN ctest + coverage + mode smoke.
# Prereq: build-native, build-asan, build-cov dirs already configured.
set -e
cd "$(dirname "$0")/.."

echo "=== Native ctest ==="
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    ctest --test-dir build-native --output-on-failure | tail -5

echo
echo "=== ASAN+UBSAN ctest ==="
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 \
    ctest --test-dir build-asan --output-on-failure | tail -5

echo
echo "=== Coverage summary ==="
gcovr -r . --filter src/ --filter include/ --print-summary build-cov | tail -5

echo
echo "=== Headless mode smoke ==="
bash tests/smoke_modes.sh
