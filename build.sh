#!/usr/bin/env bash
set -e

BUILD_DIR="build"
BUILD_WEB="${BUILD_WEB:-OFF}"

echo "=== Configuring MiniFab ==="
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DMINIFAB_BUILD_WEB="$BUILD_WEB"

echo "=== Building MiniFab ==="
cmake --build "$BUILD_DIR" -j$(nproc)

echo "=== Running tests ==="
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "=== Done ==="
