#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#  Stepan — Linux build script (x64)
#  Tested on Ubuntu 22.04 / Debian 12
# ─────────────────────────────────────────────────────────────
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-linux"

# ── Dependency hint ───────────────────────────────────────────
echo "==> Checking dependencies..."
if ! command -v cmake &>/dev/null; then
    echo "cmake not found. Install: sudo apt install cmake ninja-build"
    exit 1
fi
if ! pkg-config --exists Qt6Core 2>/dev/null; then
    echo ""
    echo "Qt6 not found via pkg-config. Install with:"
    echo "  sudo apt install qt6-base-dev qt6-webengine-dev"
    echo "  sudo apt install libqt6webenginewidgets6 libqt6webchannel6-dev"
    echo "  sudo apt install cmake ninja-build git"
    echo ""
    echo "Or set CMAKE_PREFIX_PATH to your Qt6 installation:"
    echo "  export CMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64"
    echo ""
fi

# ── Configure ────────────────────────────────────────────────
echo "==> Configuring..."
cmake -B "$BUILD_DIR" \
      -G "Ninja" \
      -DCMAKE_BUILD_TYPE=Release \
      -S "$ROOT_DIR"

# ── Build ────────────────────────────────────────────────────
echo "==> Building..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ""
echo "✓ Build complete."
echo "  Executable: $BUILD_DIR/stepan"
echo ""
echo "To create a portable package (requires linuxdeploy):"
echo "  linuxdeploy --appdir AppDir --executable $BUILD_DIR/stepan --output appimage"
