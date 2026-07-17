#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────
#  Stepan — styled .dmg builder
#  Packages an already-built Stepan.app into a nice-looking
#  installer disk image: branded background, Applications alias,
#  an arrow showing where to drag the app, and a custom volume icon.
#
#  Built on dmgbuild (https://github.com/dmgbuild/dmgbuild) rather than
#  hand-rolled `hdiutil` + Finder AppleScript: dmgbuild writes the .DS_Store
#  view-options record directly, so it isn't subject to Finder's habit of
#  silently resetting icon size / arrangement on some macOS versions when
#  a window is closed and reopened via AppleScript, and it works headlessly
#  in CI (no live Finder session required).
#
#  Requires: pipx install dmgbuild   (see BUILD.md)
#
#  Usage: make-dmg.sh <path-to-Stepan.app> <output.dmg> [VolumeName]
# ─────────────────────────────────────────────────────────────
set -euo pipefail

APP_BUNDLE="${1:?Usage: make-dmg.sh <Stepan.app> <output.dmg> [VolumeName]}"
OUT_DMG="${2:?Usage: make-dmg.sh <Stepan.app> <output.dmg> [VolumeName]}"
VOL_NAME="${3:-Stepan}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKGROUND_PNG="$ROOT_DIR/resources/dmg/background.png"
VOLUME_ICNS="$ROOT_DIR/resources/app.icns"
SETTINGS_FILE="$ROOT_DIR/scripts/dmgbuild_settings.py"

APP_BUNDLE="$(cd "$(dirname "$APP_BUNDLE")" && pwd)/$(basename "$APP_BUNDLE")"

DMGBUILD="$(command -v dmgbuild || true)"
if [ -z "$DMGBUILD" ] && [ -x "$HOME/.local/bin/dmgbuild" ]; then
    DMGBUILD="$HOME/.local/bin/dmgbuild"
fi
if [ -z "$DMGBUILD" ]; then
    echo "ERROR: dmgbuild not found. Install it with:" >&2
    echo "  brew install pipx && pipx install dmgbuild" >&2
    exit 1
fi

rm -f "$OUT_DMG"
"$DMGBUILD" \
    -D "app=$APP_BUNDLE" \
    -D "icon=$VOLUME_ICNS" \
    -D "background=$BACKGROUND_PNG" \
    -s "$SETTINGS_FILE" \
    "$VOL_NAME" "$OUT_DMG"

echo "✓ Styled DMG created: $OUT_DMG"
