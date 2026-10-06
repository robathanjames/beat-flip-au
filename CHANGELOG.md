# Changelog

## 0.3.2

- Refined the native and web interfaces with a cream faceplate, larger controls, restrained step indicators, and a consistent charcoal and amber palette.
- Removed decorative wood, grain, and multicolored effect displays. Audio behavior and saved-state compatibility are unchanged.

## 0.3.1 — Dustbox

- Renamed the plugin and browser drum lab to Dustbox.
- Vintage hardware panels, wood cheeks, amber displays, beveled keys and ribbed rotary knobs.
- Retained AU identity and every automation parameter ID for project compatibility.
- Updated build packaging and installation paths; installation guards against duplicate legacy AU registrations.

# 0.3.0 — Drum Lab

- Eight web-inspired synthesized drum voices and a 16-step off/hit/accent sequencer.
- Three grooves plus Blank, voice audition, track mutes and levels, groove swing and lo-fi Dust.
- Input/drum source selection, transport-aware playback, original/FLIP comparison, Auto Flip and KEEP.
- Automated drum parameters and project recall; legacy projects default to external audio.
- Allocation-free portable drum tests and native state/editor integration tests.

# Changelog

## 0.2.0

- Six factory presets: Subtle Pocket, Stutter Lab, Reverse Cuts, Half-time, Evolving Groove, and Full Chaos.
- An effect palette to select stutter, reverse, shuffle, gate and half speed independently.
- Fixed 2/4/8/16 stutter and gate subdivisions, plus the original random setting.
- Swing for alternating repeat and gate pulses, without shifting the live input.
- Deterministic Auto Flip every 1/2/4/8 bars, including across one-bar host loops.
- KEEP THIS PATTERN captures an automatic variation as the saved seed and turns Auto Flip off.
- Optional downbeat protection and a larger editor with an active engine pattern display.
- Backward-compatible state migration and stable existing AU parameter IDs/version hints.
- Completed transition ramps leave clean cells bit-identical at unity output gain.
- Expanded engine tests, plugin state/callback checks, and an editor preview artifact in CI.

## 0.1.0

- Original one-button, tempo-synced drum glitch effect with portable DSP and universal macOS AU/standalone builds.
