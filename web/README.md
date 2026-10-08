# Dustbox · Rhythm + Wavetable Workstation

Browser-based Dustbox with eight synthesized lo-fi 909-inspired drum voices, separate 16-step drum and synth sequencers, a playable 16-voice polyphonic wavetable synthesizer, and the FLIP engine.

[Play the hosted app](https://beat-flip-web.tj25h4ksw8.chatgpt.site) (owner access follows the Site sharing settings).

## Run locally

From the repository root:

```sh
python3 -m http.server 8080 --directory web
```

Open http://localhost:8080 in a modern browser. Serve the folder over HTTP; opening index.html directly as a file will not load the JavaScript modules reliably. No npm install or build step is required.

## Make a beat

1. Press PLAY to enable audio, or click a voice pad to audition it.
2. Click a step to cycle through off, hit, and accent. Arrow keys move between steps. Each voice has mute and level controls.
3. Start with Dusty Pocket, Warehouse 909, Broken Circuit, or a blank pattern. Set tempo, groove swing, and Dust for saturation, filtering, and reduced bit depth.
4. Play the two-octave synth keyboard, use **A W S E D F T G Y H U J K**, or enable Web MIDI. Choose Classic, Warm, or Spectral and shape Wave Position, Level, Tune, Cutoff, and ADSR.
5. Select a synth step and set a note or REST, Single/Major/Minor/Sus2/Octave, velocity, and gate. Try Bassline or Chord stabs, then enable SEQUENCE ON. The synth and drum patterns share transport and groove swing.
6. Press FLIP to process the full programmed bar into stutters, reverse slices, shuffled slices, gates, and half-speed fragments. Programmed synth notes enter the bar before FLIP. Live keys remain immediately audible throughout playback, independent of FLIP; they are never held until a bar boundary.
7. Use Amount, Mix, effect toggles, repeat speed/swing, and Protect Downbeat to shape the variation. Original/Flipped compares the two. Pattern edits take effect at bar boundaries; live keys do not wait.
8. Auto Flip generates repeatable variations every 1/2/4/8 bars. KEEP THIS FLIP holds the audible seed and turns Auto Flip off. PANIC immediately clears live notes and stops the synth sequence.

Patterns and kept flips last for the current session; refreshing restores the starting groove. Playback pauses when the tab is hidden. Browser playback requires a user gesture. This version does not import the AU's project state, record external audio, or export MIDI/WAV. Its JavaScript engine recreates the same categories of effects using the programmed drum bar; it is not a bit-for-bit port of the JUCE engine. The voices are synthesized, not Roland sample recordings.

## Validate

```sh
cd web
node --check app.mjs
node --check engine.mjs
node --check synth-sequence.mjs
node verify.mjs
```

Nine checks cover drum voices, grooves/accents/mutes, deterministic flips and source preservation, transparent bypass, all effects and swung subdivisions, tempo/sample-rate boundaries, wavetable banks/polyphony/pre-FLIP routing, multi-note synth sequencing, gate/rest/swing timing, and static assets. Live keyboard response during playback, visual layout, Web MIDI, and the optional WebMCP integration require an actual-browser check.

With Playwright and Chromium installed, run `node browser-check.mjs` for desktop/mobile Web Audio regression checks. These verify three live notes and note-offs within one bar at 100% FLIP, live monitoring after STOP, PANIC, programmed chord playback, sequence editing and overflow. Set `DUSTBOX_PLAYWRIGHT_MODULE` and `DUSTBOX_BROWSER_BIN` when using externally installed runtimes; optional `DUSTBOX_SCREENSHOT_DIR` saves the tested layouts. Web MIDI still needs a connected-device check.

## Files

- `index.html`: instrument interface and metadata.
- `styles.css`: responsive drum-machine styling. Fonts have local fallbacks.
- `engine.mjs`: synthesized drum/wavetable voices, bar rendering, and seeded flip processing.
- `app.mjs`: sequencer and synth controls, Web Audio scheduling, MIDI, audition, waveform, and optional WebMCP tools.
- `synth-sequence.mjs`: note/chord patterns, presets and timing shared by the controls and renderer.
- `verify.mjs`: dependency-free audio-engine and static-entrypoint checks.
- `browser-check.mjs`: optional actual-browser playback and layout regression checks.
