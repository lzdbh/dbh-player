#!/bin/bash
# Build a self-contained macOS .app + .dmg (unsigned dev package).
# Usage: scripts/package-macos.sh [qt-prefix]
#   qt-prefix defaults to ~/Qt/6.12.0/macos
# Requires: cmake/ninja on PATH, Xcode CLT, Qt 6.12 (universal).
set -euo pipefail
cd "$(dirname "$0")/.."

QT_PREFIX="${1:-$HOME/Qt/6.12.0/macos}"
VERSION=$(grep -m1 -E '^[[:space:]]+VERSION [0-9]' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
BUILD_DIR=build-release
OUT_DIR=dist

# NOTE: libmpv (third_party/libmpv) is currently x86_64-only, so the package
# is x86_64. A universal package requires a universal libmpv build.
cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="x86_64"
cmake --build "$BUILD_DIR"

"$QT_PREFIX/bin/macdeployqt" "$BUILD_DIR/dbhx-player.app" -qmldir=qml
# SQL drivers are unused and reference missing system libs; drop them.
rm -rf "$BUILD_DIR/dbhx-player.app/Contents/PlugIns/sqldrivers"
# Bundle libmpv (macdeployqt does not know about it) and sign it.
mkdir -p "$BUILD_DIR/dbhx-player.app/Contents/Frameworks"
cp third_party/libmpv/lib/libmpv.2.dylib "$BUILD_DIR/dbhx-player.app/Contents/Frameworks/"
codesign --force -s - "$BUILD_DIR/dbhx-player.app/Contents/Frameworks/libmpv.2.dylib"
# macdeployqt rewrites binaries; re-apply the ad-hoc signature (required for
# local network / SSDP, see HANDOVER pitfall #9).
codesign --force --deep -s - "$BUILD_DIR/dbhx-player.app"

STAGE=$(mktemp -d)
cp -R "$BUILD_DIR/dbhx-player.app" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
mkdir -p "$OUT_DIR"
hdiutil create -volname "DBHX Player" -srcfolder "$STAGE" -ov -format UDZO \
  "$OUT_DIR/dbhx-player-$VERSION-macos-universal.dmg"
rm -rf "$STAGE"
echo "Done: $OUT_DIR/dbhx-player-$VERSION-macos-universal.dmg"