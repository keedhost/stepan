#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#  Stepan — macOS build script
#  Supports Apple Silicon (arm64) and Intel (x86_64)
# ─────────────────────────────────────────────────────────────
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-macos"

# ── Locate Qt6 ───────────────────────────────────────────────
if [ -z "${CMAKE_PREFIX_PATH:-}" ]; then
    # Try Homebrew first
    if command -v brew &>/dev/null; then
        QT_BREW=$(brew --prefix qt 2>/dev/null || brew --prefix qt@6 2>/dev/null || true)
        if [ -n "$QT_BREW" ] && [ -d "$QT_BREW" ]; then
            export CMAKE_PREFIX_PATH="$QT_BREW"
            echo "==> Found Qt6 via Homebrew: $QT_BREW"
        fi
    fi

    # Try common manual install paths
    if [ -z "${CMAKE_PREFIX_PATH:-}" ]; then
        for candidate in \
            "$HOME/Qt/6.7.0/macos" \
            "$HOME/Qt/6.6.0/macos" \
            "$HOME/Qt/6.5.0/macos" \
            "/usr/local/opt/qt6" \
            "/opt/homebrew/opt/qt"
        do
            if [ -d "$candidate" ]; then
                export CMAKE_PREFIX_PATH="$candidate"
                echo "==> Found Qt6 at: $candidate"
                break
            fi
        done
    fi

    if [ -z "${CMAKE_PREFIX_PATH:-}" ]; then
        echo "ERROR: Qt6 not found. Set CMAKE_PREFIX_PATH, e.g.:"
        echo "  export CMAKE_PREFIX_PATH=~/Qt/6.7.0/macos"
        echo "  or install via Homebrew: brew install qt"
        exit 1
    fi
fi

# ── Ninja check ──────────────────────────────────────────────
GENERATOR="Xcode"
if command -v ninja &>/dev/null; then
    GENERATOR="Ninja"
fi

# ── Configure ────────────────────────────────────────────────
echo "==> Configuring (generator: $GENERATOR)..."
cmake -B "$BUILD_DIR" \
      -G "$GENERATOR" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH" \
      -S "$ROOT_DIR"

# ── Build ────────────────────────────────────────────────────
echo "==> Building..."
cmake --build "$BUILD_DIR" --config Release --parallel "$(sysctl -n hw.logicalcpu)"

# ── Deploy ───────────────────────────────────────────────────
APP_BUNDLE="$BUILD_DIR/Stepan.app"
if [ -d "$APP_BUNDLE" ]; then
    echo "==> Running macdeployqt..."
    MACDEPLOYQT="$CMAKE_PREFIX_PATH/bin/macdeployqt"
    if [ -f "$MACDEPLOYQT" ]; then
        "$MACDEPLOYQT" "$APP_BUNDLE" -dmg
        echo ""
        echo "✓ Build & deploy complete."
        echo "  App bundle:   $APP_BUNDLE"
        echo "  DMG:          $BUILD_DIR/Stepan.dmg"
    else
        echo "  macdeployqt not found at $MACDEPLOYQT — skipping DMG creation."
        echo "✓ Build complete. App bundle: $APP_BUNDLE"
    fi
else
    echo "✓ Build complete. Check $BUILD_DIR for output."
fi
