# Dustbox DB-12 web workstation

The K.O. II-inspired performance face keeps the original Dustbox drum machine, 16-voice wavetable synth, both 16-step sequencers and FLIP engine. Twelve velocity pads, a smoked LCD, A–D source pattern banks and an assignable fader stay visible while DRUMS, SOUND, PATTERN and FX / FLIP select the editor below.

- DRUMS pads 1–8 play the original voices; pads 9–12 play softer kick, snare, clap and closed-hat variants. CHROMATIC KEYS plays C3–B3. Click higher on a pad for higher velocity; keys sustain until release. Number keys 1–9, 0, minus and equals play the pads. The existing A/W/S/E… piano keyboard remains available in SOUND.
- A–D each remember 99 independent **source pattern slots**, storing the drum grid, synth notes/chords, velocity/gate and sequence enable. These banks are alternate arrangements of the existing engines, not four simultaneous audio buses. Sound settings and FLIP remain global. Pattern changes during playback land on the next rendered bar. Web banks remain session-local.
- The master fader controls Dust, synth Pitch, Filter, Synth Level, FLIP Mix or Wave Morph. Changes update the existing controls; the target remains pinned during a pointer gesture.
- All previous grooves, mutes, levels, Dust, swing, synth banks/ADSR, on-screen keys, MIDI input, FLIP palette, KEEP and auto variations remain available. Live synth notes still bypass FLIP and respond immediately, including at 100% wet.

Run `node verify.mjs` from this directory for offline engine and pattern bank tests. `browser-check.mjs` uses Playwright for actual audio, note release, transport, bank recall, fader and desktop/mobile checks; GitHub Actions runs it and attaches screenshots.

This release implements the performance interface and source pattern workflow. Sample import/chopping, 46.875 kHz converter modes, 32 mono/16 stereo sample voices, per-group multi-outs, motion recording, full punch-in FX, pattern chains, stem drag export and controller templates remain the separate sampler-engine roadmap. No controls for those unfinished capabilities are shown.
