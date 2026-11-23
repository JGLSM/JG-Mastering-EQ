# Design reference

- `C13.html` is board **C13 · Ice Grey** exactly as it was on the design canvas
  (file `JGC13.dc.html`). It is the visual reference for the plugin. It uses the
  canvas's template format (`{{holes}}`, `<sc-for>`), so it won't render on its
  own in a browser; the working port lives in `ui/`.
- Design canvas with every iteration: https://claude.ai/artifact/DCaYmZ3Jvf7q7z1qPkaDR7

Differences between C13 and the prototype interface in `ui/` (made to match
decisions in `docs/BRIEF.md` that came after C13):

- Input: GAIN knob is now DRIVE; the CLEAN/RICH switch is now ON/BYPASS for the
  input stage (Clean/Rich was dropped on 23 Nov 2026).
- Colour selector shows four modes: TUBE, IRON, SS, TAPE.
- Gain knobs turn right to boost, left to cut (C13 drew boosts to the left).
- Each dial shows its gain under the frequency readout.
- Stage numbers (1, 2, 3) are clickable stage bypass buttons.
- Output gain is not on the panel yet (the output module needs its redesign pass).
