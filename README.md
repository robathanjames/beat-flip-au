# Dustbox

A lo-fi drum machine, 16-step sequencer and beat-flipping effect for **Logic Pro and other macOS Audio Unit hosts**.

Put it on a drum loop or drum bus, let a full bar play, and press **FLIP**. The plugin generates a new tempo-synced pattern of stutters, reverse slices, rearranged hits, rhythmic cuts, and half-speed fragments. Keep a pattern you like, or let Auto Flip generate variations at bar boundaries. Downbeat protection keeps the groove anchored by default.

**Version 0.3.1 introduces the Dustbox name and vintage hardware interface. Version 0.3.0 added the Drum Lab from the web app: eight synthesized voices, an editable 16-step sequencer, groove presets, track mutes and levels, Dust, and groove swing.** The portable DSP engine passes behavioral and sanitizer tests. See the current [validation status](docs/VALIDATION.md) before using it in a production session. There is no finished, signed installer.

The AU identity, bundle ID and automation parameter IDs retain the original Beat Flip identifiers, so saved projects still recall the same plugin. When upgrading, move the old `Beat Flip.component` out of the Components folder before installing `Dustbox.component` to avoid duplicate AU registrations.

## Drum Lab

Choose **Drum machine** as Audio Source, load **Dusty Pocket**, **Warehouse 909**, or **Broken Circuit**, and turn **PLAY DRUMS** on. In Logic, insert Dustbox as an Audio FX on a track and start the host transport. The standalone runs at Free Tempo. **Blank** clears the grid. Click each step to cycle off → hit → accent. Click a voice name to audition it, use M to mute, and adjust its level slider. Muted voices are silent during audition too.

The eight voices are kick, snare, clap, closed hat, open hat, low tom, rim, and ride, synthesized with the web app's voice equations. Closed hats choke open hats. Groove Swing delays alternate drum steps; Repeat Swing separately shapes the FLIP pulses. Dust combines filtering, saturation, reduced bit depth and sample hold. Pattern steps, mutes, levels and all controls are automatable and recalled with the project.

Sequence a beat, let the plugin capture a full bar, then press **FLIP**. Switch **GLITCH ON** off to compare the original. Auto Flip and KEEP work on the sequenced beat. The native engine rearranges captured audio from the previous bar, so grid edits can take a bar to reach glitched slices. This retains the native effect's zero-latency live path; the web app renders whole bars ahead of playback. The two implementations share the workflow and effect families, rather than bit-identical FLIP audio. Bounce the track in your DAW to export the result.

**Audio input** remains the default for compatibility with existing projects. Loading a drum groove selects the drum source. Stop drum playback before changing back to external input if you want the next drum session to remain stopped. New drum parameters use AU version hint 3; all older parameter IDs and version hints are unchanged.

## Controls

| Control | What it does |
| --- | --- |
| **FLIP** | Generates a new pattern and turns Glitch on. The new pattern starts at the next grid step. |
| **Glitch On** | Switches the pattern on/off with a short mix ramp. The input keeps being captured. |
| **Amount** | Sets how many of the 16 cells get glitched. Zero keeps the original beat. |
| **Mix** | Blends the live drums with the flipped version. |
| **Output** | Trims the final output from −24 to +6 dB. |
| **Free Tempo** | Sets the fallback tempo if the host does not supply one. Logic's tempo takes precedence. |
| **Pattern Seed** | A host-automatable parameter. The same seed and controls recreate the same pattern and are saved with the project. |
| **KEEP THIS PATTERN** | Saves the current audible variation as Pattern Seed and turns Auto Flip off. |
| **Stutter / Gate Speed** | Random, or 2, 4, 8 or 16 pulses per grid cell. Random retains the original 2/4/8 behavior. |
| **Repeat Swing** | Delays alternating stutter and gate pulses from 0–75%. Live cells and the main 16-cell grid keep their timing. |
| **Auto Flip** | Generates a new variation every 1, 2, 4 or 8 bar boundaries; Off holds the base seed. |
| **Effect Palette** | Choose any combination of stutter, reverse, shuffle, gate and half speed. With none selected, the effect passes through. |
| **Protect Downbeat** | Keeps cell 1 live. Turn off to let the whole bar change. |
| **Factory Presets** | Loads one of six starting points, preserving your Free Tempo setting. |

The grid labels are **LIVE**, **STUT** (repeat), **REV** (reverse), **JUMP** (a different slice), **CUT** (gate), and **DRAG** (half speed, one octave lower).

## Factory presets

| Preset | Starting point |
| --- | --- |
| **Subtle Pocket** | Sparse shuffle and short repeats, low mix, a little swing. |
| **Stutter Lab** | Dense eight-pulse repeats with the downbeat protected. |
| **Reverse Cuts** | Reversed slices and swung rhythmic gating. |
| **Half-time** | Half-speed fragments at full wet mix, with the first cell live. |
| **Evolving Groove** | Balanced effects, swung repeats, a variation every two bars. |
| **Full Chaos** | All effects, sixteen-pulse repeats, downbeat glitches and a variation every bar. |

Auto Flip derives a repeatable sequence from the saved base seed. It advances across one-bar host loops and restarts from the base after playback restarts, seeks, or tempo/meter changes. It counts bar starts while Glitch is on; cycles that exclude all bar starts cannot advance it. Pressing FLIP restarts the sequence from a new base seed at the next cell. Selecting Off returns to the base pattern; use **KEEP THIS PATTERN** to retain the current automatic variation instead.

Palette, speed and downbeat changes also take effect at the next grid cell. The grid shows the engine's active pattern. Existing 0.1 projects retain their original controls and load the new controls with their original-sound defaults.

## Build on your Mac

Requires Xcode or Xcode Command Line Tools, CMake 3.22 or later, Git, and an internet connection for the first JUCE download. [Get CMake](https://cmake.org/download/). If you use Homebrew, `brew install cmake` installs it.

```bash
xcode-select --install # Only if you don't already have Apple's developer tools.
git clone https://github.com/robathanjames/beat-flip-au.git
cd beat-flip-au
bash scripts/build-macos.sh
bash scripts/install-au.sh
auval -v aufx BtFp Rbjm
```

The build creates a universal **Apple Silicon + Intel** AU and standalone app:

```text
build-macos/BeatFlip_artefacts/Release/AU/Dustbox.component
build-macos/BeatFlip_artefacts/Release/Standalone/Dustbox.app
```

The install script copies the AU to your user Audio Units folder. Quit and reopen Logic after installation, find **Rob James → Dustbox** in the Audio FX menu, and insert it on a drum track. Use Logic's Plug-in Manager to rescan it if needed. The script stops if a previous installation already exists, so move that version aside before replacing it.

The standalone can play the built-in drum sequencer or route live audio into the effect; it does not load audio files itself. On a Mac, grant microphone permission if you want to use an audio input. For drum files, use the AU in Logic.

If you already have JUCE 8.0.15 locally, configure manually:

```bash
cmake -S . -B build-macos -G 'Unix Makefiles' \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DCMAKE_BUILD_TYPE=Release \
  -DBEATFLIP_JUCE_SOURCE_DIR=/absolute/path/to/JUCE
cmake --build build-macos --config Release --parallel 4
```

## GitHub builds

The included workflow runs portable engine tests on Linux, builds the AU on macOS, and checks preset recall, legacy project migration, KEEP, and the complete audio callback. It also renders an editor PNG under the **Beat-Flip-editor-preview** artifact. The macOS job installs the component on its runner and runs Apple's `auval`. A completed build attaches ZIPs containing the universal AU and standalone app under **Actions → Build and test Dustbox → Artifacts**.

Those are development builds, without Developer ID signing or notarization. Artifacts are retained even if AU validation fails; check the validation job result before installing. See [validation status](docs/VALIDATION.md).

## Engine tests

The DSP engine has no JUCE dependency. On Linux or macOS:

```bash
bash scripts/test-core.sh
```

Or use CMake:

```bash
cmake -S . -B build-core -DBEATFLIP_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
```

Thirteen test groups cover dry transparency, repeatability across buffer sizes, stereo coherence, ring-buffer wrap, source slice accuracy, quantized changes, host loops and seeks, odd meters, effect selection, stutter speed, swung pulse timing, automatic variation, presets, invalid controls, and allocation-free processing. A sanitizer run is also included in CI.

## Hear a generated A/B example

The demo renders a synthetic drum loop: four bars dry, then four bars flipped at 140 BPM. It uses the same engine as the AU.

The downloadable project package includes `Examples/Beat-Flip-AB.wav` so you can hear it before building. The generated WAV is omitted from Git history.

```bash
cmake --build build-core --target beatflip_demo
./build-core/beatflip_demo Beat-Flip-AB.wav
```

With the macOS build, use `cmake --build build-macos --config Release --target beatflip_demo`, then `./build-macos/beatflip_demo`.

## How the effect works

The plugin continuously records the **dry input** into a fixed-size stereo history buffer. It divides each bar into 16 equal cells, aligned to host PPQ and the current time signature. Glitched cells read slices from the previous bar; clean cells use live audio. This keeps the dry path at zero latency while allowing both earlier and later hits from a recorded bar to be rearranged.

It needs a fully captured bar before slices become available. Starting playback partway through a bar can take up to two bars to capture an aligned one. Looping preserves history; seeks, playback restarts, and tempo/meter changes invalidate it and capture a fresh bar. Very slow or unusual meters whose bars exceed 16 seconds pass through safely.

Slice transitions have short ramps; mix and output changes are smoothed. Both channels share the same pattern and timing. Audio processing performs no allocations, locking, file I/O, or UI calls. Parameters and the seed are saved by JUCE's `AudioProcessorValueTreeState`; recorded audio is captured fresh when playback starts.

This is an **audio effect**, so it changes what you hear and what you bounce. It does not rewrite the drum region, create MIDI notes, or export a new Logic pattern. To keep a result as audio, bounce the processed track in your DAW.

## Project layout

| Path | Purpose |
| --- | --- |
| `Source/GlitchEngine.*` | Portable pattern generator and DSP. |
| `Source/PluginProcessor.*` | AU parameters, transport, state, and audio processing. |
| `Source/PluginEditor.*` | Performance controls, presets, and live pattern grid. |
| `Source/FactoryPresets.h` | The six factory starting points. |
| `Tests/` | Offline engine tests and allocation probe. |
| `Tools/RenderDemo.cpp` | Deterministic synthetic drum A/B renderer. |
| `scripts/` | Build, install, test, and GitHub creation helpers. |
| `.github/workflows/build.yml` | Linux tests and macOS AU build/validation. |

The project fetches [JUCE 8.0.15](https://github.com/juce-framework/JUCE/releases/tag/8.0.15). JUCE is licensed separately; see [license notes](LICENSE-NOTES.md).
