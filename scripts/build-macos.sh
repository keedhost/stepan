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
FRAMEWORKS_DIR="$APP_BUNDLE/Contents/Frameworks"

if [ -d "$APP_BUNDLE" ]; then
    MACDEPLOYQT="$CMAKE_PREFIX_PATH/bin/macdeployqt"
    if [ -f "$MACDEPLOYQT" ]; then
        echo "==> Running macdeployqt..."
        # macdeployqt may print rpath warnings for transitive Homebrew deps —
        # those get fixed by the manual copy step below, so we ignore its exit code.
        "$MACDEPLOYQT" "$APP_BUNDLE" || true

        # ── Copy any missing Homebrew dylibs that macdeployqt couldn't resolve ──
        echo "==> Bundling missing Homebrew dylibs..."
        BREW_SEARCH_DIRS=(
            "/opt/homebrew/opt/brotli/lib"
            "/opt/homebrew/opt/webp/lib"
            "/opt/homebrew/opt/libwebp/lib"
            "/opt/homebrew/opt/qt/lib"
            "/opt/homebrew/lib"
        )
        # Find every @rpath reference inside the bundle that isn't already satisfied
        while IFS= read -r dylib; do
            while IFS= read -r ref; do
                libname=$(basename "$ref")
                if [ ! -f "$FRAMEWORKS_DIR/$libname" ]; then
                    for dir in "${BREW_SEARCH_DIRS[@]}"; do
                        if [ -f "$dir/$libname" ]; then
                            echo "  Copying $libname from $dir"
                            cp "$dir/$libname" "$FRAMEWORKS_DIR/"
                            # Rewrite the reference in the dylib that needs it
                            install_name_tool -change "$ref" \
                                "@executable_path/../Frameworks/$libname" \
                                "$dylib" 2>/dev/null || true
                            break
                        fi
                    done
                fi
            done < <(otool -L "$dylib" 2>/dev/null | awk '/^\t@rpath/{print $1}')
        done < <(find "$FRAMEWORKS_DIR" -name "*.dylib" -o -name "*.framework/Versions/A/*" 2>/dev/null | \
                 grep -v '\.dSYM')

        # ── Fix @executable_path refs so QtWebEngineProcess can find frameworks ─
        # macdeployqt uses @executable_path/../Frameworks/ everywhere.  This
        # works for Stepan's main binary but breaks QtWebEngineProcess, which is
        # a separate helper app with a different @executable_path.  We rewrite:
        #   • Framework binaries (Frameworks/X.framework/Versions/A/X):
        #       @executable_path/../Frameworks/Foo  →  @loader_path/../../../Foo
        #   • Flat dylibs (Frameworks/libX.dylib):
        #       @executable_path/../Frameworks/libY  →  @loader_path/libY
        # Both resolve to Stepan.app/Contents/Frameworks/ regardless of which
        # executable triggered the load.
        # Use process substitution (< <(find ...)) instead of piped while (find | while)
        # so the loop body runs in the current shell — required for install_name_tool
        # to work reliably under set -euo pipefail.
        echo "==> Fixing @executable_path refs (framework binaries)..."
        while IFS= read -r binary; do
            while IFS= read -r old_ref; do
                libpart="${old_ref#@executable_path/../Frameworks/}"
                install_name_tool -change "$old_ref" \
                    "@loader_path/../../../$libpart" "$binary" 2>/dev/null || true
            done < <(otool -L "$binary" 2>/dev/null | \
                     awk '/^\t@executable_path\/\.\.\/Frameworks\//{print $1}')
        done < <(find "$FRAMEWORKS_DIR" -path "*/Versions/A/*" -type f \
            ! -name "*.plist" ! -name "*.dat" ! -name "*.pak" ! -name "*.png" \
            ! -name "*.css" ! -name "*.js" ! -name "*.html" ! -name "*.nib" \
            ! -name "*.car" ! -name "*.qm" ! -name "*.conf")

        echo "==> Fixing @executable_path refs (flat dylibs)..."
        while IFS= read -r binary; do
            while IFS= read -r old_ref; do
                libpart="${old_ref#@executable_path/../Frameworks/}"
                install_name_tool -change "$old_ref" \
                    "@loader_path/$libpart" "$binary" 2>/dev/null || true
            done < <(otool -L "$binary" 2>/dev/null | \
                     awk '/^\t@executable_path\/\.\.\/Frameworks\//{print $1}')
        done < <(find "$FRAMEWORKS_DIR" -maxdepth 1 -name "*.dylib" -type f)

        # Fix absolute Homebrew paths in QtWebEngineProcess itself
        QWE_HELPER="$FRAMEWORKS_DIR/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess"
        if [ -f "$QWE_HELPER" ]; then
            echo "==> Fixing QtWebEngineProcess absolute Homebrew paths..."
            while IFS= read -r old_ref; do
                libpart=$(echo "$old_ref" | sed 's|.*/lib/||')
                install_name_tool -change "$old_ref" "@rpath/$libpart" \
                    "$QWE_HELPER" 2>/dev/null || true
            done < <(otool -L "$QWE_HELPER" 2>/dev/null | \
                     awk '/^\t\/opt\/homebrew\//{print $1}')
        fi

        # ── Pass 4: Safety net — fix residual /opt/homebrew/ paths ───────────
        # On some macOS/Qt version combinations (observed: macOS 26 + Qt 6.11)
        # macdeployqt may fail to rewrite references in the main binary or in
        # PlugIns/.  When that happens, both Homebrew Qt AND the bundled Qt get
        # loaded simultaneously, and Qt's platform init crashes with abort().
        # This pass rewrites any remaining absolute Homebrew references to the
        # correct bundle-relative form so only one Qt instance is ever loaded.
        #
        # Transformation rules (applied to the main binary and all PlugIns):
        #   /opt/homebrew/.../Foo.framework/Versions/A/Foo
        #       → @executable_path/../Frameworks/Foo.framework/Versions/A/Foo
        #   /opt/homebrew/.../libbar.dylib
        #       → @executable_path/../Frameworks/libbar.dylib
        _fix_brew_refs() {
            local bin="$1"
            while IFS= read -r old_ref; do
                # Extract "Foo.framework/..." for frameworks, basename for dylibs
                local libpart
                libpart=$(echo "$old_ref" | \
                          sed 's|.*/\([^/]*\.framework/[^[:space:]]*\)|\1|')
                if [ "$libpart" = "$old_ref" ]; then
                    libpart=$(basename "$old_ref")
                fi
                echo "    $(basename "$bin"): $old_ref"
                echo "      → @executable_path/../Frameworks/$libpart"
                install_name_tool -change "$old_ref" \
                    "@executable_path/../Frameworks/$libpart" \
                    "$bin" 2>/dev/null || true
            done < <(otool -L "$bin" 2>/dev/null | \
                     awk '/^\t\/opt\/homebrew\//{print $1}')
        }

        echo "==> Safety net: checking main binary for residual Homebrew paths..."
        _fix_brew_refs "$APP_BUNDLE/Contents/MacOS/Stepan"

        echo "==> Safety net: checking PlugIns for residual Homebrew paths..."
        while IFS= read -r plugin; do
            _fix_brew_refs "$plugin"
        done < <(find "$APP_BUNDLE/Contents/PlugIns" -type f -name "*.dylib" \
                      2>/dev/null)

        # ── Re-sign everything after macdeployqt modified the bundle ──────────
        echo "==> Re-signing bundle..."
        codesign --force --deep --sign - "$APP_BUNDLE"

        # ── Package as DMG ─────────────────────────────────────────────────────
        # Use hdiutil directly — macdeployqt -dmg re-runs deployment and reverts
        # the @loader_path fixes we applied above.
        echo "==> Creating DMG..."
        hdiutil create -volname "Stepan" -srcfolder "$APP_BUNDLE" \
            -ov -format UDZO "$BUILD_DIR/Stepan.dmg"

        echo ""
        echo "✓ Build & deploy complete."
        echo "  App bundle:   $APP_BUNDLE"
        echo "  DMG:          $BUILD_DIR/Stepan.dmg"
    else
        echo "  macdeployqt not found at $MACDEPLOYQT — skipping DMG creation."
        echo "==> Signing bundle (dev build)..."
        codesign --force --deep --sign - "$APP_BUNDLE"
        echo "✓ Build complete. App bundle: $APP_BUNDLE"
    fi
else
    echo "✓ Build complete. Check $BUILD_DIR for output."
fi
