#!/usr/bin/env bash
# Stepan — твій друг в системі координат
# Copyright (C) 2026 Andrii Kondratiev
#
# Слава Україні! Слава ЗСУ!
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

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
