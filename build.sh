set -e

BUILD_DIR="build"

echo "=== Configuring MiniFab ==="
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug

echo "=== Building MiniFab ==="
cmake --build "$BUILD_DIR" -j$(nproc)

echo "=== Running tests ==="
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "=== Done ==="
