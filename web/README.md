# Beat Flip · Drum Lab

Browser-based Beat Flip with eight synthesized lo-fi 909-inspired drum voices and a 16-step sequencer.

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
4. Press FLIP to process the full programmed bar into stutters, reverse slices, shuffled slices, gates, and half-speed fragments. Source steps stay intact.
5. Use Amount, Mix, effect toggles, repeat speed/swing, and Protect Downbeat to shape the variation. Original/Flipped compares the two. Changes take effect at bar boundaries.
6. Auto Flip generates repeatable variations every 1/2/4/8 bars. KEEP THIS FLIP holds the audible seed and turns Auto Flip off.

Patterns and kept flips last for the current session; refreshing restores the starting groove. Playback pauses when the tab is hidden. Browser playback requires a user gesture. This version does not import the AU's project state, record external audio, or export MIDI/WAV. Its JavaScript engine recreates the same categories of effects using the programmed drum bar; it is not a bit-for-bit port of the JUCE engine. The voices are synthesized, not Roland sample recordings.

## Validate

```sh
cd web
node --check app.mjs
node --check engine.mjs
node verify.mjs
```

Seven checks cover voices, grooves/accents/mutes, deterministic flips and source preservation, transparent bypass, all effects and swung subdivisions, tempo/sample-rate boundaries, and static assets. Browser audio listening, visual layout, and the optional WebMCP integration still require an actual-browser check.

## Files

- `index.html`: instrument interface and metadata.
- `styles.css`: responsive drum-machine styling. Fonts have local fallbacks.
- `engine.mjs`: synthesized voices, bar rendering, and seeded flip processing.
- `app.mjs`: sequencer controls, Web Audio scheduling, audition, waveform, and optional WebMCP tools.
- `verify.mjs`: dependency-free audio-engine and static-entrypoint checks.
