# JG Designs Mastering EQ — Claude Code Handoff Brief

Nov 23, 2026 · @Josh

## How to use this brief

This is the single source of truth for building the JG Designs Mastering EQ in Claude Code: where the design landed, every concept agreed so far, and what is still open. Export it to Markdown and save it at the root of the plugin repo as `docs/BRIEF.md`, then point `CLAUDE.md` at it.

**Status:** the visual design is settled (Ice Grey colourway, board C13). The parameter concepts are not final; the next phase is iterating on them, stage by stage, with real audio in a DAW.

**Working agreement for Claude Code**

- Work one stage at a time. Each stage is a loop: propose, build the smallest testable version, Josh listens in his DAW, adjust. Expect many rounds per stage.
- Never change a parameter ID once it has shipped in any build Josh has saved a session with. Rename display names freely; IDs are permanent (see Parameters).
- Keep every iteration comparable: tag builds (`v0.3-presence-curve-b`) so Josh can A/B an older build against a new one, the same way every design board was kept on the canvas.
- Ask before changing anything marked **Decided** here. Anything marked **Open** is fair game to propose.
- Every DSP change ships with a test (frequency response, null test or a golden-file comparison). Ears decide; tests stop regressions.
- Update this brief when a decision lands. Note what changed and why in a short changelog at the end of `BRIEF.md`.

**Who decides what:** Josh owns sound, feel, naming and scope. Claude proposes DSP approaches, flags trade-offs (CPU, latency, aliasing) and writes the code and tests.

## Product vision

A mastering EQ that sounds like premium analogue hardware, looks like a luxury instrument, and quietly helps without taking over. It is the first plugin under the **JG Designs** name, made by a two-time Grammy-winning mastering engineer, and it carries the Love Science brand.

**What it is:** a three-stage chain in one window: a simple input stage, a subtle realtime "Smart EQ", and a seven-band passive-style EQ that is the centrepiece. Output metering includes full loudness (LUFS) and true peak.

**Sound goals**

- Passive-EQ character: broad, musical curves with the interaction and "insertion loss" feel of RLC hardware (the Pultec boost-and-cut trick should be possible), at Manley-level polish.
- Saturation only where it earns it: a band pushed past its redline starts to add colour. Below the redline the EQ stays clean.
- The Smart EQ must never be heavy-handed: small, cut-first moves, capped at ±3 dB.
- Musical low end: the SUB band can add harmonics so bass translates on small speakers (MaxxBass-style), instead of only adding raw level.

**Feel goals:** hardware you want to touch. Contoured, hand-grippable dials; a calm, uncluttered layout; digital displays only where they add information.

**Design references:** modern Rolex and IWC watches (Top Gun ceramic), high-end BMW and the Porsche 911 interior and paint. The final colourway is Ice Grey.

**Market context:** sits alongside Ozone Clarity and Soundtheory Gullfoss (both dynamic EQs that steer toward a target), but leads with analogue-style passive EQ and keeps the "smart" part deliberately subtle. Business options discussed: a free, brand-building release of a reduced version, then a paid full version.

## Signal flow

The order is fixed: Input, then Smart EQ, then the passive EQ, then output metering. The Smart EQ sits before the passive EQ so the passive stage shapes an already-balanced signal.

&#91;embedded content: signal flow · 3 stages + output metering\]

Each stage has its own bypass, plus a global bypass; output gain sits after the passive EQ, before the meters. Oversampling applies to every non-linear part (the input character, Lift, redline saturation, bass harmonics). Whether Lift runs before or after the input character is still open. **Latency (Decided 23 Nov 2026):** realtime oversampling is a fixed factor, with a higher factor used only for offline renders, so the latency reported to the DAW never changes during a session.

## Stage 1: Input

One fixed character, designed by Josh's ears and baked in. There are no modes to choose, so the EQ is always voiced against the same input sound. (Decided 23 Nov 2026; replaces the earlier Analogue Clean/Rich switch.)

| Control | What it does | Status |
| --- | --- | --- |
| Drive | Pushes the signal into the input character: more drive, more colour. Automatic level compensation keeps loudness roughly constant, so turning it up never wins an A/B by being louder. | Decided; range and curve open |
| Input bypass | Takes the whole input stage out for transparent jobs. With it bypassed and every band flat, the plugin should null. | Decided |
| Lift | One button: fixed 1.3:1 compression plus a +0.1 dB high shelf. A glue-and-sheen preset, not a full compressor. | Decided |
| Input meter | Stereo IN meter in dBFS with numeric peak readouts per channel. | Decided |

**Default setting:** at the default Drive position the stage should already sound like "the JG input" (subtle enhancement), at matched loudness.

**Open:** what the character is built from (tube-like, transformer-like, a blend) and how much it does at default; Drive range and taper; Lift threshold, attack and release (auto, programme-dependent is the working idea), shelf corner (proposed 10–12 kHz), and whether Lift sits before or after the character.

**Saturation notes:** every non-linear stage needs oversampling (2× to 4×, chosen by CPU tests) to keep aliasing out of the top octave. The input character and the passive EQ's saturation modes can share one saturation engine.

**History:** Clean/Rich modes were dropped because a switch upstream changes the sound of everything after it. A Dangerous Compressor-style bus compressor panel was dropped earlier; Lift is what remains of it.

## Stage 2: Smart EQ

A realtime dynamic EQ that listens and makes small corrective moves. It sits second in the chain, before the passive EQ, so the passive stage shapes an already-balanced signal. Its whole identity is subtlety.

**Concept lineage:** Ozone Clarity steers the spectrum toward a target slope; Gullfoss steers toward a model of audibility and masking (it is not phase correction). Our Smart EQ borrows the "steer gently toward balance" idea but is capped and conservative by design.

**Subtlety rules (Decided)**

1. Maximum ±3 dB of correction at any frequency, ever.
2. Cut first. Boosts are off by default (the CUT ONLY pill is on in the mockup).
3. Amount defaults to 30%.
4. Confidence gating: if the analysis isn't sure (sparse material, transients, silence), it does less, not more.
5. Slow, smoothed movement. No audible pumping or chasing.
6. FREEZE holds the current correction curve static.
7. Δ LISTEN (delta) solos only what the Smart EQ is removing or adding, so Josh can hear exactly what it does.

| Control | What it does | Status |
| --- | --- | --- |
| Amount | 0–100%, scales all corrections. Default 30%. | Decided |
| MAX ±3 dB | Shows the cap; may become a selectable range (±1.5 / ±3). | Open |
| CUT ONLY | Restricts to cuts. Default on. | Decided |
| FREEZE | Holds the curve. | Decided |
| Δ LISTEN | Delta solo. | Decided |

**Display:** a wide realtime panel: the magenta spectrum, the cyan correction curve with nodes and dB labels (mockup: −1.2 dB at about 250 Hz, −2.0 dB at about 3.5 kHz), dashed ±3 dB limit lines, and a perspective grid floor.

**Open questions:** what the target is (a tilt slope, a reference profile, or a masking model); lookahead and latency; number of correction bands; behaviour on very sparse or very dense material. Build this stage last; it is closer to a research project than the rest.

## Stage 3: Passive EQ (the centrepiece)

Seven bands of passive-style EQ, each a large glass-cased dial. This is where the plugin's sound lives, so it gets the most iteration.

**Each band has two hand controls**

- **Frequency:** the red, hourglass-shaped needle dial that sweeps the printed scale.
- **Gain:** the concentric centre knob. Turn left to cut, right to boost; a coloured arc shows the amount (white for boost, cyan for cut, red once past the redline).
- **Q has no knob.** It is set in the Q window under the bands (see below).

| Band | Default freq | Sweep range (dial scale) | Shape | Default Q | Mockup gain |
| --- | --- | --- | --- | --- | --- |
| SUB | 40 Hz | 16–100 Hz | Low shelf | 0.7 | +5 dB (harmonics) |
| LOW | 100 Hz | 40–300 Hz | Bell | 0.8 | +1.5 dB |
| LOW MID | 300 Hz | 150 Hz–1 kHz | Bell | 1.2 | −1.5 dB |
| MID | 1 kHz | 500 Hz–4 kHz | Bell | 1.0 | 0 dB |
| HIGH MID | 3 kHz | 1.5–6 kHz | Bell | 1.4 | −1 dB |
| PRESENCE | 6 kHz | 3–12 kHz | Bell | 2.0 | +4.5 dB (saturating) |
| AIR | 16 kHz | 6–24 kHz | High shelf | 0.7 | +2.5 dB |

All frequencies and sweep ranges above are provisional: the values printed on each dial will be redesigned. Gain range in the mockup is ±10 dB per band. Ranges overlap on purpose, as on hardware.

**Redline saturation (Decided concept).** Each band has a redline at +4 dB of boost. Below it the band is clean. Past it the band adds harmonic colour that grows with the boost, and its stem, gain arc and Q icon turn red. The panel header says "past redline = color". Mid and high bands are tagged SATURATING; SUB and LOW are tagged HARMONICS.

**Saturation follows the boost, not the track's level (Decided 23 Nov 2026).** The plugin does not react to how loud the incoming mix is. Colour only engages when Josh pushes a band past its redline, so a quiet master and a hot master behave the same. (Level-reactive, more analogue behaviour can be tested later.)

**Bass harmonics on SUB and LOW (Decided concept).** Instead of only raising level, boosting past the redline adds psychoacoustic harmonics of the fundamental (MaxxBass-like), so low end translates on small speakers without eating headroom.

**Saturation modes: TUBE / IRON / SS / TAPE (Decided for testing, 23 Nov 2026).** The selector sets the flavour of the redline saturation. Order and names may be revised after listening.

| Mode | Character to aim for |
| --- | --- |
| Tube | Smooth, mostly even-order harmonics, gentle rounding of peaks |
| Iron (transformer) | Weight in the lows and low mids; harmonics rise at low frequencies and higher drive |
| SS (solid state) | Tighter, more odd-order harmonics, punchier edge when pushed |
| Tape | Soft peak compression, slight top-end softening, low-end weight |

**Linked first, per-band later.** Phase one tests the four modes applied globally to all seven bands (linked). An unlinked mode, with a per-band menu (likely a second chip beside "BELL ▾" in the Q window), may follow. If built: unlinking starts every band on the current global mode, and each band's choice is remembered when relinking.

**DSP challenge to research:** colouring only a band's boost means isolating the boosted portion, saturating it and summing it back without phase problems. Wrap the whole passive EQ in one oversampled block rather than oversampling seven saturators separately.

**Q window (Decided: concept #2, "quiet at rest").** One long, dark display under the seven bands. At rest each band shows only a small shape icon and its Q number. The band being adjusted expands into a full curve with two drag handles that set the width, a "Q 2.0" readout and a shape chip (BELL ▾) to change the shape. Hot bands show in red. Thin stems connect each dial to its slot.

**Passive behaviour to model:** gentle insertion-loss feel, band interaction (boost and cut on nearby bands should interact like RLC networks, enabling the Pultec trick), minimum-phase response. Passive versus active topology was discussed in depth; both are minimum-phase, and the goal is the passive *sound*, not literal insertion loss the user has to make up.

**Removed along the way:** HPF/LPF filter dials (knobs, then digital arcs) and a spectrum panel under the bands. Both were cut to keep the panel calm.

## Output and loudness

The top-right module mirrors the input module in size (the top row is symmetrical): an output meter plus a loudness column, in one rounded panel.

| Element | Spec | Status |
| --- | --- | --- |
| OUT meter | Stereo, dBFS, numeric peak readout per channel (mockup −6.2 / −0.9, the hot one in red). | Decided |
| Integrated | Large readout in LUFS (mockup −13.8). | Decided |
| Momentary | LUFS, 400 ms window (mockup −11.2). | Decided |
| Short-term | LUFS, 3 s window (mockup −12.6). | Decided |
| True peak | dBTP, oversampled peak detection; red above the ceiling (mockup −0.9). | Decided |
| Target pill | "−14 LUFS ▾" dropdown to pick a loudness target (e.g. −14 streaming, −9 loud master). | Decided (concept) |

**Standards:** loudness per ITU-R BS.1770 / EBU R128 gating; true peak with at least 4× oversampling. These are well-defined, so they are good early wins with exact tests against reference files.

**Output gain (Decided 23 Nov 2026):** the plugin gets an output gain control, placed before the meters so OUT, LUFS and true peak read what leaves the plugin. The output module needs a redesign pass on the canvas to fit it.

**Considered and dropped:** a separate output colour stage (Tape/Tube/Iron on the whole mix). All saturation choices now live in the passive EQ.

**Open:** an output ceiling or limiter (none planned so far); reset for integrated loudness; whether the target only colours the readout (it should not auto-gain unless Josh decides so); gain-matched A/B.

## Parameters: draft list and ID rules

Lock parameter **IDs** as early as possible; keep display names, ranges and curves flexible until the first build Josh uses in a real session. DAWs store automation and session state against the ID, so changing an ID later silently breaks saved sessions and automation.

**ID rules**

1. IDs are short, lowercase and stage-prefixed: `in_`, `seq_` (Smart EQ), `peq_` (passive EQ), `out_`, `glb_` (global).
2. Bands are numbered by position (`b1` to `b7`), not named, so renaming "PRESENCE" never touches an ID.
3. The display name is separate from the ID and can change freely.
4. Never delete-and-reuse an ID. A retired parameter keeps its ID reserved forever.
5. Use JUCE's versioned parameter IDs (version hint) from day one; AU hosts rely on it when parameters are added later.
6. Prefer adding a new parameter over changing an existing one's meaning or range in a breaking way.

**Draft list (v0, for iteration)**

| ID | Display name | Type | Range / options | Default |
| --- | --- | --- | --- | --- |
| `in_drive` | Drive | float | range and taper open (auto level-compensated) | voiced default |
| `in_bypass` | Input Bypass | bool | off / on | off |
| `in_lift` | Lift | bool | off / on | off |
| `seq_amount` | Smart Amount | float, % | 0–100 | 30 |
| `seq_range` | Smart Range | choice | ±1.5 dB, ±3 dB | ±3 dB |
| `seq_cut_only` | Cut Only | bool | off / on | on |
| `seq_freeze` | Freeze | bool | off / on | off |
| `seq_delta` | Delta Listen | bool | off / on | off |
| `seq_bypass` | Smart EQ Bypass | bool | off / on | off |
| `peq_bN_freq` (×7) | Band N Freq | float, Hz | per band (provisional) | per band |
| `peq_bN_gain` (×7) | Band N Gain | float, dB | −10 to +10 | 0 |
| `peq_bN_q` (×7) | Band N Q | float | 0.3–4.0 (open) | per band |
| `peq_bN_shape` (×7) | Band N Shape | choice | Bell, Shelf (per band options open) | per band |
| `peq_sat` | Saturation Mode | choice | Tube, Iron, SS, Tape | Tube |
| `peq_sat_link` | Link Modes | bool | linked / unlinked (reserved; linked only for now) | linked |
| `peq_bN_sat` (×7) | Band N Saturation | choice | Tube, Iron, SS, Tape (reserved for unlinked mode) | follows `peq_sat` |
| `peq_bypass` | Passive EQ Bypass | bool | off / on | off |
| `out_gain` | Output Gain | float, dB | −12 to +12 (open) | 0 |
| `out_target` | Loudness Target | choice | −23, −16, −14, −11, −9 LUFS (open) | −14 |
| `glb_bypass` | Bypass | bool | off / on | off |
| `glb_oversampling` | Oversampling | choice | 1×, 2×, 4× (open) | 2× |

That is about 55 automatable parameters. The per-band saturation and link IDs are reserved now so unlinked mode can be added later without breaking anything. Meters, LUFS readouts and the Smart EQ curve are outputs, not parameters.

**Decide before locking:** whether the redline threshold (+4 dB) is fixed or a parameter; whether frequency dials are continuous or stepped like hardware (affects the parameter type); per-band bypass; mid/side.

## Open decisions to iterate on

These are Josh's calls. Work through them stage by stage, ideally by hearing options in the DAW rather than deciding on paper.

**Whole plugin**

- [ ] Stereo only, or mid/side in v1?
- [ ] A/B snapshots, undo/redo and a preset browser in v1?
- [ ] Output ceiling or limiter (output gain itself is decided).
- [ ] Free version scope versus paid version scope.

**Input**

- [ ] What the fixed input character is built from, and how much it does at the default Drive setting.
- [ ] Drive range and taper.
- [ ] Lift: threshold, attack/release, shelf frequency, and its position before or after the character.

**Smart EQ**

- [ ] What it steers toward: tilt slope, reference profile or masking model.
- [ ] Fixed ±3 dB cap or a selectable range.
- [ ] Latency budget (lookahead or none).

**Passive EQ**

- [ ] Frequencies printed on each dial (being redesigned).
- [x] Continuous frequency sweep (log taper); readout in whole Hz below 1 kHz, 0.01 kHz above. Gain free-rotating, 0.1 dB resolution. (Decided 23 Nov 2026)
- [ ] Final sweep ranges and gain range per band.
- [ ] Q range and which shapes each band offers (e.g. can SUB switch to bell?).
- [ ] Redline: fixed at +4 dB or adjustable; how fast colour grows past it.
- [ ] Saturation modes after listening: keep all four (Tube, Iron, SS, Tape), rename, reorder?
- [ ] Whether to build unlinked, per-band saturation menus.
- [ ] Level-reactive saturation as a later option (currently boost-only).
- [ ] Per-band bypass.
- [ ] How literally to model passive interaction and insertion loss.

## Visual design

The reference design is board **C13 · Ice Grey** on the [design canvas](https://claude.ai/artifact/DCaYmZ3Jvf7q7z1qPkaDR7) (file `JGC13.dc.html`). Every earlier iteration (A through B18, colourways C01–C13, D1) is kept there for comparison. Bring the C13 HTML into the repo as `design/reference/C13.html`.

**Layout (1360 × 662 reference size; body 1336 × 638, 40 px corner radius)**

- Top centre: black pill with red light lines and LOVE SCIENCE.
- Top row, symmetrical: Input module (left, 230 px wide) · Smart EQ (centre, 788 px) · Output + loudness (right, 230 px).
- Middle: the passive EQ panel, 1276 × 340 px, with seven graduated dials (outer largest 196 px, middle smallest 166 px, rising and falling in an arc) and the Q window beneath.
- Bottom: MASTERING EQ engraved at left; JG DESIGNS emblem centred between two vent grilles; a brushed-steel Nº 0001 serial plate at right.
- Six steel screws: four corners and one on each side.

**Design system**

| Token | Value |
| --- | --- |
| Paint (Ice Grey) | gradient #76828E → #606B77 → #67737F → #515B66 → #727E8A, top to bottom, plus fine metallic flake and a diagonal clear-coat highlight |
| Needles, redline, hot states | #FF3B3F |
| Digital readouts | cyan #5CE1FF |
| Spectrum | magenta #FF4FD8 |
| Display / logo font | Michroma (JG mark and dial numerals) |
| UI font | Archivo (variable width axis; condensed for values, expanded for labels) |
| Lighting | straight-down shadows (a top-left key light was tried and rejected) |

**Finish details kept from the premium pass:** sunburst dial faces, raised polished hour-marker bars at major ticks, engraved labels (dark top edge, light bottom edge), the chrome-rimmed enamel JG plaque, textured crosshatch in the band panel.

**Rejected along the way (don't reintroduce without asking):** perimeter sliders (B11), the unified spectrum panel (B13, too busy), HPF/LPF dials, glowing filter arcs, the top-left key light, purple paint, swapping the JG emblem to the top (D1).

**UI technology:** JUCE 8 supports building the editor as an embedded web view, which lets most of the C13 HTML/CSS carry over. Risk: the flake (SVG noise filters), repeating sunburst gradients and 60 fps meters may be heavy, especially with several instances open. Prototype performance in week one; fall back to pre-rendered image layers or native JUCE drawing for the heaviest parts. The UI must also scale (resizable, HiDPI).

## Tech stack, formats and distribution

**Decided: JUCE.** It is the industry-standard C++ framework for exactly this: one codebase builds VST3, AU and AAX (plus a standalone app for quick testing). The current major version is JUCE 9; web-view editors arrived in JUCE 8.

**JUCE licence** ([juce.com/get-juce](https://juce.com/get-juce/), checked 23 Nov 2026)

| Tier | Price | Annual revenue limit |
| --- | --- | --- |
| Starter | Free, perpetual, all features | $20,000 |
| Indie | $40/month or $800 perpetual | $300,000 |
| Pro | $175/month or $3,500 perpetual | No limit |

The limit counts revenue from the licensee's use of JUCE (for a company, the company's revenue), so check the EULA wording against how Love Science Music is structured before release. Starting on Starter costs nothing; move up before revenue crosses the limit.

**Formats and what each needs**

| Format | Hosts | What it takes |
| --- | --- | --- |
| AU | Logic, GarageBand, many Mac DAWs | Built by JUCE on macOS; validate with Apple's `auval`. |
| VST3 | Most DAWs on Mac and Windows | Built by JUCE; validate with Steinberg's validator and pluginval. |
| AAX | Pro Tools | Free Avid developer account and AAX SDK; test unsigned builds in the Pro Tools Developer build; release builds must be signed with PACE's signing tools (free via Avid) using an iLok account and an iLok USB key. Start the Avid application early; replies can be slow. |

**Shipping basics (later, but plan for them):** Apple Developer Program membership for code signing and notarisation on macOS; a code-signing certificate for Windows; an installer per platform; copy protection or licensing (PACE signing for AAX is only signing, not protection); a version scheme and update channel.

**Repo and build conventions to set up first:** CMake-based JUCE project; `docs/BRIEF.md` (this brief) and `CLAUDE.md`; `design/reference/` for C13; `src/dsp/` separated from `src/ui/` so DSP can be tested headless; `tests/` with the harness below; continuous integration building all formats on every push. Targets: macOS first (Josh's rig), Windows before public release; sample rates 44.1–192 kHz.

## Testing and QA

Set the harness up before the first DSP code. Ears decide what's good; automated tests make sure it stays good after every change.

| Test | What it catches | When |
| --- | --- | --- |
| pluginval (strictness 10) | Crashes, threading bugs, bad parameter handling, state save/load failures | Every build, in CI |
| auval / VST3 validator | Format-specific compliance | Every build, in CI |
| Null test | With the input stage and Smart EQ bypassed, every band flat and output gain at 0 dB, the plugin leaves audio bit-identical. Oversampling must be off when nothing non-linear is active; otherwise the test uses a stated tolerance. (Decided 23 Nov 2026) | Every DSP change |
| Frequency-response golden files | Each band's curve at set freq/gain/Q matches the saved reference within ±0.05 dB | Every DSP change |
| Golden audio renders | Fixed test material through fixed presets matches stored renders; differences flagged | Every DSP change |
| Aliasing and THD sweeps | Saturation stays clean above the redline target; no fold-back in the top octave | Saturation work |
| Loudness reference files | LUFS and true peak match EBU test signals | Metering work |
| Automation and parameter sweeps | No zipper noise or clicks when any knob moves fast | Every control |
| CPU benchmark | Per-instance CPU at 44.1, 96 and 192 kHz; tracked over time | Weekly |
| Session recall | Save, close and reopen a DAW session; every value returns | Before any build Josh uses for real work |

**Listening protocol:** Josh picks a small, fixed set of reference masters (dense pop, sparse acoustic, bass-heavy, bright) and A/Bs every candidate build against the previous one at matched loudness. Builds are tagged so any two can be compared.

**Hosts to check:** Logic (AU), Pro Tools (AAX), Ableton Live and Reaper (VST3) at minimum.

## Build roadmap

Sound first, Smart EQ last. Josh is tuning real audio in his DAW within the first couple of weeks, and every phase ends at a gate he signs off by ear. No dates yet; each phase takes as many rounds as it needs.

&#91;embedded content: build roadmap · 5 phases, 5 gates\]

The UI is not built first. Phase 0 proves C13 can run smoothly inside the plugin. Bands start with simple placeholder visuals and get the full C13 finish in Phase 3, once the DSP has stopped pushing back on the design.

## Risks, lessons and the first session

**Risks to manage**

| Risk | Why it matters | Mitigation |
| --- | --- | --- |
| Parameter IDs change after real use | Breaks saved sessions and automation | Lock IDs in Phase 1; follow the ID rules |
| Web-view UI too heavy | Laggy meters, high CPU with several instances | Performance test in week one; pre-render or go native for heavy layers |
| Saturation aliasing | Harsh top end, fails A/B | Oversampling, aliasing sweeps in tests |
| Smart EQ becomes audible or pumpy | Breaks the plugin's core promise | Hard ±3 dB cap, cut-first, confidence gating, build it last |
| Drive or saturation wins only by being louder | Misleading A/B | Level-match all non-linear modes |
| AAX approval delays | Pro Tools release slips | Apply to Avid early; ship VST3/AU first |
| Scope creep | Never ships | Phases with clear gates; a smaller free release first |

**Lessons from the design phase**

- Keeping every iteration side by side made decisions faster and safer. Do the same with builds.
- "Too busy" was the most common rejection. When in doubt, remove.
- Seeing it beat describing it. For DSP, that means hearing options rather than debating them.

**Suggested first Claude Code session**

1. Create the JUCE CMake project, repo layout, `CLAUDE.md`, and save this brief as `docs/BRIEF.md`.
2. Set up CI with pluginval and the format validators on an empty pass-through plugin.
3. Review the draft parameter list together and lock v1 IDs.
4. Build the input stage end to end: Drive with automatic level matching, input bypass, IN meter and output gain, with automation, state save/load and a null test with the stage bypassed.
5. Load it in Josh's DAW and listen. Start the iteration loop.

## Sources

- [JUCE pricing and licence tiers](https://juce.com/get-juce/)
- [JUCE 8 licence changes (JUCE forum)](https://forum.juce.com/t/amendments-to-the-juce-end-user-licence-agreement-for-juce-8/61265)
- [Avid AAX SDK and developer program](https://developer.avid.com/aax/)
- [PACE: getting started with AAX code signing](https://paceap.com/getting-started-with-aax-code-signing-for-pro-tools-plugins/)
- [Design canvas with every iteration (C13 is the reference)](https://claude.ai/artifact/DCaYmZ3Jvf7q7z1qPkaDR7)

## Changelog

- **23 Nov 2026 (repo setup):** Null test now also requires the Smart EQ bypassed, with oversampling off when nothing non-linear is active (otherwise the test runs against a stated tolerance). Latency: realtime oversampling is fixed, with a higher setting for offline renders, so the latency reported to the DAW never changes mid-session. Band frequency is continuous (log taper); readout shows whole Hz below 1 kHz and 0.01 kHz steps above. Gain is free-rotating at 0.1 dB resolution. Still open: the shape options per band (suggested: Bell, Low Shelf, High Shelf on every band), to be settled when v1 IDs are locked. This brief moved into the repo as `docs/BRIEF.md`.
- **23 Nov 2026:** Input becomes one fixed character with Drive (auto level-compensated) and an input bypass; Clean/Rich modes removed. Output gain added. Output colour stage considered and dropped. Passive EQ saturation modes set to Tube, Iron, SS and Tape, tested globally first; per-band (unlinked) mode reserved. Saturation responds to boost past the redline, not to track level. Roadmap reordered: input stage before the passive EQ. Dial frequencies marked provisional.
- **23 Nov 2026:** Brief created from the design-phase thread.
