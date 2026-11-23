#!/bin/bash
# Builds JG Mastering EQ on this Mac and installs the AU + VST3 for this user.
# Needs only Apple's Command Line Tools (xcode-select --install).
# Run from Terminal:  bash ~/Downloads/jg-mastering-eq-main/scripts/build-mac.sh
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$HOME/JG-build"
CMAKE_VER=3.31.6
CMAKE_DIR="$WORK/cmake-$CMAKE_VER-macos-universal"
CMAKE="$CMAKE_DIR/CMake.app/Contents/bin/cmake"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"

echo "==> Checking Apple Command Line Tools"
xcode-select -p >/dev/null 2>&1 || { echo "Please run: xcode-select --install  (then run this script again)"; exit 1; }

mkdir -p "$WORK"
if [ ! -x "$CMAKE" ]; then
  echo "==> Downloading CMake $CMAKE_VER (about 75 MB, one time only)"
  curl -fL --progress-bar -o "$WORK/cmake.tgz" "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VER/cmake-$CMAKE_VER-macos-universal.tar.gz"
  tar -xzf "$WORK/cmake.tgz" -C "$WORK" && rm "$WORK/cmake.tgz"
fi

echo "==> Configuring (first time downloads JUCE, about 2 minutes)"
"$CMAKE" -S "$SRC" -B "$WORK/build" -DCMAKE_BUILD_TYPE=Release -DJGEQ_BUILD_TESTS=OFF \
  -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" >/dev/null

echo "==> Building (about 5 to 15 minutes the first time)"
"$CMAKE" --build "$WORK/build" --config Release --parallel "$(sysctl -n hw.ncpu)" \
  --target JGMasteringEQ_AU JGMasteringEQ_VST3 JGMasteringEQ_Standalone

ART="$WORK/build/JGMasteringEQ_artefacts/Release"
echo "==> Installing"
mkdir -p "$AU_DIR" "$VST3_DIR"
rm -rf "$AU_DIR/JG Mastering EQ.component" "$VST3_DIR/JG Mastering EQ.vst3"
cp -R "$ART/AU/JG Mastering EQ.component" "$AU_DIR/"
cp -R "$ART/VST3/JG Mastering EQ.vst3" "$VST3_DIR/"
for b in "$AU_DIR/JG Mastering EQ.component" "$VST3_DIR/JG Mastering EQ.vst3" "$ART/Standalone/JG Mastering EQ.app"; do
  xattr -dr com.apple.quarantine "$b" 2>/dev/null || true
  codesign --force --deep --sign - "$b" >/dev/null 2>&1 || true
done
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "==> Checking the AU with Apple's validator"
if auval -v aufx Jgme Jgds >"$WORK/auval.log" 2>&1; then echo "    AU passed."; else echo "    AU check failed, see $WORK/auval.log"; fi

echo
echo "Done! Installed:"
echo "  $AU_DIR/JG Mastering EQ.component"
echo "  $VST3_DIR/JG Mastering EQ.vst3"
echo "Standalone app: $ART/Standalone/JG Mastering EQ.app"
echo "Restart your DAW (Logic: look under Audio Units > JG Designs)."
[ -z "${JGEQ_NO_OPEN:-}" ] && open "$ART/Standalone" || true
