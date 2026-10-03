# Beat Flip

A one-button drum glitch effect for **Logic Pro and other macOS Audio Unit hosts**.

Put it on a drum loop or drum bus, let a full bar play, and press **FLIP**. The plugin generates a new tempo-synced pattern of stutters, reverse slices, rearranged hits, rhythmic cuts, and half-speed fragments. It keeps playing that pattern until you flip again. The first downbeat stays live so the groove keeps a recognizable anchor.

**Version 0.1.0 is a source-code prototype.** The portable DSP engine is tested; the AU wrapper and editor still need a macOS build and validation. This repository includes the code and a GitHub Actions workflow for that validation. It does not include a finished, signed installer.

## Controls

| Control | What it does |
| --- | --- |
| **FLIP** | Generates a new pattern and turns Glitch on. The new pattern starts at the next grid step. |
| **Glitch On** | Switches the pattern on/off with a short mix ramp. The input keeps being captured. |
| **Amount** | Sets how many of the 16 cells get glitched. Zero keeps the original beat. |
| **Mix** | Blends the live drums with the flipped version. |
| **Output** | Trims the final output from −24 to +6 dB. |
| **Free Tempo** | Sets the fallback tempo if the host does not supply one. Logic's tempo takes precedence. |
| **Pattern Seed** | A host-automatable parameter. The same seed recreates the same pattern and is saved with the project. |

The grid labels are **LIVE**, **STUT** (repeat), **REV** (reverse), **JUMP** (a different slice), **CUT** (gate), and **DRAG** (half speed, one octave lower).

## Build on your Mac

Requires Xcode or Xcode Command Line Tools, CMake 3.22 or later, Git, and an internet connection for the first JUCE download. [Get CMake](https://cmake.org/download/). If you use Homebrew, `brew install cmake` installs it.

```bash
xcode-select --install # Only if you don't already have Apple's developer tools.
bash scripts/build-macos.sh
bash scripts/install-au.sh
auval -v aufx BtFp Rbjm
```

The build creates a universal **Apple Silicon + Intel** AU and standalone app:

```text
build-macos/BeatFlip_artefacts/Release/AU/Beat Flip.component
build-macos/BeatFlip_artefacts/Release/Standalone/Beat Flip.app
```

The install script copies the AU to your user Audio Units folder. Quit and reopen Logic after installation, find **Rob James → Beat Flip** in the Audio FX menu, and insert it on a drum track. Use Logic's Plug-in Manager to rescan it if needed. The script stops if a previous installation already exists, so move that version aside before replacing it.

The standalone app is for routing live audio into the effect; it does not load audio files itself. On a Mac, grant microphone permission if you want to use an audio input. For drum files, use the AU in Logic.

If you already have JUCE 8.0.15 locally, configure manually:

```bash
cmake -S . -B build-macos -G 'Unix Makefiles' \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DCMAKE_BUILD_TYPE=Release \
  -DBEATFLIP_JUCE_SOURCE_DIR=/absolute/path/to/JUCE
cmake --build build-macos --config Release --parallel 4
```

## GitHub builds

The included workflow runs portable engine tests on Linux and builds the AU on macOS. The macOS job installs the component on its runner and runs Apple's `auval`. A successful run attaches ZIPs containing the universal AU and standalone app under **Actions → Build and test Beat Flip → Artifacts**.

Those are development builds, without Developer ID signing or notarization. The workflow is included but has not been run while preparing this source project. See [validation status](docs/VALIDATION.md).

## Create your GitHub repository

After extracting this project, install [GitHub CLI](https://cli.github.com/) and sign in using its browser flow:

```bash
gh auth login --web
bash scripts/create-github-repo.sh
```

The script creates a **private** `beat-flip-au` repository in the signed-in account and pushes the source. Pass a different name as the first argument if you want one. It refuses to change an existing `origin` remote. If Git asks for an author identity, configure `git config user.name` and `git config user.email` in this project and rerun the script.

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

Tests cover dry transparency, repeatability across buffer sizes, stereo coherence, ring-buffer wrap, source slice accuracy, quantized pattern changes, host loops and seeks, odd meters, invalid input values, and allocation-free processing. A sanitizer run is also included in CI.

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
| `Source/PluginEditor.*` | FLIP button, knobs, and live pattern grid. |
| `Tests/` | Offline engine tests and allocation probe. |
| `Tools/RenderDemo.cpp` | Deterministic synthetic drum A/B renderer. |
| `scripts/` | Build, install, test, and GitHub creation helpers. |
| `.github/workflows/build.yml` | Linux tests and macOS AU build/validation. |

The project fetches [JUCE 8.0.15](https://github.com/juce-framework/JUCE/releases/tag/8.0.15). JUCE is licensed separately; see [license notes](LICENSE-NOTES.md).
