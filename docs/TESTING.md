# Testing the JG Mastering EQ

Ears decide what sounds good. These tests make sure it *stays* good after
every change, and catch the boring-but-fatal problems (crashes, lost
settings, clicks) before a build reaches a session.

## Two layers

| Layer | What it is | What it catches |
| --- | --- | --- |
| **Industry validators** | pluginval (strictness 10), Apple `auval`, Steinberg VST3 validator | Crashes, threading bugs, host compatibility, format rules |
| **Our own audio tests** (`tests/`) | The real plugin, run offline, fed test signals and measured | Anything about *the sound* and *your sessions*, which the validators don't look at |

pluginval checks that the plugin behaves like a well-made plugin. It does not
know what this plugin is supposed to sound like, so it can't tell if a band
curve moved, if Drive makes things louder, or if a session recalls a value
slightly off. That's what our own tests are for.

## What runs today

| Test | Checks | Status |
| --- | --- | --- |
| Null test | Input stage + Smart EQ bypassed, bands flat, output 0 dB: output is bit-identical to input, at 44.1-192 kHz, odd block sizes, 32/64-bit, mono, offline bounce | Active |
| Silence | Silence in gives silence out (no hiss, no DC) | Active |
| Latency | Reported latency never changes when controls move | Active |
| Parameter ID lock | IDs match `tests/parameter_ids.txt` exactly, follow the brief's naming rules, have version hints | Active (list provisional until v1 is signed off) |
| Session recall | Random settings save and reload exactly, and don't drift on re-save | Active |
| Display text | Values shown in the DAW can be typed back in | Active |
| Automation sweeps | Slamming every control end to end while a tone plays causes no clicks or zipper noise | Active |
| Frequency response | Flat within 0.05 dB at null settings, compared with a stored golden curve | Active; per-band curves added with the passive EQ |
| Distortion and aliasing | THD and non-harmonic (aliasing) energy of pure tones | Active for "must be clean"; redline saturation targets added with that DSP |
| Drive level matching | Loudness stays within 0.5 dB at any Drive setting | Active (becomes meaningful once Drive does something) |
| Golden render | Fixed test material through default settings matches a stored render | Active; your reference masters can be added |
| CPU benchmark | Per-instance CPU at 44.1, 96, 192 kHz | Printed every build |

## Added as each stage lands

- **Passive EQ:** per-band response goldens (each band, several freq/gain/Q/shape settings), band interaction (Pultec boost+cut), redline: clean below +4 dB, harmonics above, aliasing in the top octave below a set limit, every saturation mode level-matched.
- **Input stage:** Drive curve, Lift (1.3:1 and +0.1 dB shelf), level matching for real.
- **Metering:** LUFS momentary/short-term/integrated and true peak against the EBU R128 / ITU-R BS.1770 reference signals.
- **Smart EQ:** never exceeds the +/-3 dB cap, cut-only means no boosts, freeze holds, delta listen nulls against the difference.
- **Oversampling:** realtime vs offline factor, reported latency handled correctly.
- **Session recall in real DAWs:** a manual checklist (Logic, Ableton, Reaper, Pro Tools) before any build you use for real work.

## Running the tests

- **On your Mac, one command** (builds, installs, runs everything, prints a summary):
  `bash ~/Downloads/jg-mastering-eq-main/scripts/test-mac.sh`
- **On GitHub:** every push runs the Linux build and our tests; pushes to
  `main`, pull requests and version tags also run the full Mac build with
  auval, the VST3 validator and pluginval.
- **Developers:** `ctest --test-dir build --output-on-failure`

## Golden files

`tests/golden/` holds the stored reference curves and renders. When a sound
change is approved by ear, refresh them in the same change:
`JGEQ_UPDATE_GOLDENS=1 ./build/tests/JGMasteringEQTests`. Never refresh them just
to make a failing test pass.

## Your reference masters

To add golden renders of real music, put short excerpts (20-30 s, WAV) of
your fixed reference set (dense pop, sparse acoustic, bass-heavy, bright) in
the project folder. They stay in this private repo only.
