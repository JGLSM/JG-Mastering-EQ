#!/bin/bash
# Runs every automated check on this Mac and prints a plain summary:
#   1. our own audio tests (null test, parameters, recall, clicks, response...)
#   2. Apple's AU validator (auval)
#   3. pluginval at strictness 10 on the AU and the VST3
#   4. the CPU benchmark
# Builds and installs the latest code first (same as build-mac.sh).
# Run from Terminal:  bash ~/Downloads/jg-mastering-eq-main/scripts/test-mac.sh
set -uo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$HOME/JG-build"
PLUGINVAL_VER=v1.0.4
AU="$HOME/Library/Audio/Plug-Ins/Components/JG Mastering EQ.component"
VST3="$HOME/Library/Audio/Plug-Ins/VST3/JG Mastering EQ.vst3"
LOGS="$WORK/test-logs"
mkdir -p "$LOGS"
declare -a RESULTS=()
FAILED=0

step() { echo; echo "==> $1"; }
record() { if [ "$2" -eq 0 ]; then RESULTS+=("PASS  $1"); else RESULTS+=("FAIL  $1  (log: $3)"); FAILED=1; fi; }

step "Building and installing the latest code"
JGEQ_NO_OPEN=1 bash "$SRC/scripts/build-mac.sh" >"$LOGS/build.log" 2>&1 || { echo "Build failed, see $LOGS/build.log"; tail -20 "$LOGS/build.log"; exit 1; }
CMAKE="$(ls -d "$WORK"/cmake-*-macos-universal | head -1)/CMake.app/Contents/bin/cmake"
CTEST="$(dirname "$CMAKE")/ctest"

step "Building the test suite"
"$CMAKE" -S "$SRC" -B "$WORK/build" -DJGEQ_BUILD_TESTS=ON >/dev/null
"$CMAKE" --build "$WORK/build" --config Release --parallel "$(sysctl -n hw.ncpu)" --target JGMasteringEQTests JGMasteringEQBench >"$LOGS/build-tests.log" 2>&1
record "Test suite builds" $? "$LOGS/build-tests.log"

step "1/4 Our audio tests"
"$WORK/build/tests/JGMasteringEQTests" >"$LOGS/tests.log" 2>&1
r=$?; tail -3 "$LOGS/tests.log"; record "Audio tests (null, parameters, recall, clicks, response, level)" $r "$LOGS/tests.log"

step "2/4 Apple auval"
killall -9 AudioComponentRegistrar 2>/dev/null || true
auval -v aufx Jgme Jgds >"$LOGS/auval.log" 2>&1
r=$?; tail -2 "$LOGS/auval.log"; record "auval (Apple AU validation)" $r "$LOGS/auval.log"

step "3/4 pluginval (strictness 10)"
PV="$WORK/pluginval.app/Contents/MacOS/pluginval"
if [ ! -x "$PV" ]; then
  curl -fsL -o "$WORK/pv.zip" "https://github.com/Tracktion/pluginval/releases/download/$PLUGINVAL_VER/pluginval_macOS.zip" && \
  (cd "$WORK" && unzip -oq pv.zip && rm pv.zip)
  xattr -dr com.apple.quarantine "$WORK/pluginval.app" 2>/dev/null || true
fi
"$PV" --strictness-level 10 --validate-in-process "$AU" >"$LOGS/pluginval-au.log" 2>&1
r=$?; tail -1 "$LOGS/pluginval-au.log"; record "pluginval AU" $r "$LOGS/pluginval-au.log"
"$PV" --strictness-level 10 --validate-in-process "$VST3" >"$LOGS/pluginval-vst3.log" 2>&1
r=$?; tail -1 "$LOGS/pluginval-vst3.log"; record "pluginval VST3" $r "$LOGS/pluginval-vst3.log"

step "4/4 CPU benchmark"
"$WORK/build/tests/JGMasteringEQBench" 10 | tee "$LOGS/benchmark.log"

echo
echo "================ SUMMARY ================"
for line in "${RESULTS[@]}"; do echo "$line"; done
echo "========================================="
if [ $FAILED -eq 0 ]; then echo "Everything passed."; else echo "Something failed. Paste the FAIL lines (and the log's last lines) to Claude."; fi
exit $FAILED
