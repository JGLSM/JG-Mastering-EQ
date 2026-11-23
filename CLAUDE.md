# CLAUDE.md

**Read `docs/BRIEF.md` first.** It is the single source of truth for the JG Designs
Mastering EQ: product vision, signal flow, every decision (Decided / Open), the
draft parameter list and ID rules, testing plan and roadmap. Follow its
"Working agreement for Claude Code".

## Working with Josh
- Josh owns sound, feel, naming and scope. He is new to coding and GitHub:
  explain things plainly, and check in before big steps.
- Ask before changing anything marked **Decided** in the brief. Update the brief's
  changelog whenever a decision lands.
- Never change a parameter ID once a build with it has been used in a saved
  session. Prototype builds (before v1 IDs are locked) are the exception and
  are labelled as such.
- The repo must stay **private**: the Avid AAX SDK is confidential and will be
  added later. Never commit it before then, and never make the repo public.

## Layout
- `src/` plugin code. `src/dsp/` audio code only (no UI includes) so it can be
  tested headless; `src/ui/` native side of the interface.
- `src/Parameters.{h,cpp}` the parameter list (IDs follow the brief's rules).
- `ui/` the web interface (ported from design board C13), compiled into the plugin.
  `index.html` holds the template, `app.js` the logic. Open `ui/index.html` in a
  browser to work on it with fake data. Keep these files ASCII (CI checks).
- `tests/` Catch2 tests (null test, state round trip, layouts). Every DSP change
  ships with a test.
- `design/reference/` C13 reference board and notes on how the prototype differs.
- `.github/workflows/build.yml` CI: Linux build+tests on every push; macOS
  VST3/AU build with auval, Steinberg validator and pluginval (strictness 10) on
  main, PRs, tags and manual runs. Version tags (`v*`) publish a release zip.

## Build
```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```
JUCE 9 is fetched automatically (pinned in `CMakeLists.txt`).

## Conventions
- Tag builds Josh listens to (e.g. `v0.3-presence-curve-b`) so any two can be A/B'd.
- Plugin codes: manufacturer `Jgds`, plugin `Jgme`. Never change after release.
- AAX: waits for PACE signing tools. Add the AAX format then, with the SDK kept
  out of git history except in this private repo.
