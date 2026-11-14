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

echo "=== Running C++ tests ==="
ctest --test-dir "$BUILD_DIR" --parallel 4 --output-on-failure

echo "=== Running JavaScript tests ==="
if command -v node >/dev/null 2>&1 &&
   command -v npm >/dev/null 2>&1; then
    npm test
else
    echo "ERROR: Node.js and npm are required for frontend tests." >&2
    exit 1
fi

echo "=== All MiniFab tests passed ==="

echo "=== Done ==="
