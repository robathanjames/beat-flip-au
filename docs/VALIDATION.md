# Validation status — 0.5.0

[Build run #33](https://github.com/robathanjames/beat-flip-au/actions/runs/37799895924) passed on October 8, 2026. It tested source commit `2ddcdecbf7ccde69e88b9232b471f2a758cc8898`, which contains the live synth playback fix and dedicated synth sequencer. Later commits update documentation and screenshots only.

## Verified native behavior

- The universal Apple Silicon + Intel build produced both effect/instrument AUs and standalone apps.
- All six macOS test suites passed: FLIP engine, drums, drum MIDI export, wavetable synth, synth sequencing, and processor state/editor integration.
- Synth checks cover multiple notes/chords within one bar, rest/gate/swing timing, immediate live notes at full-wet FLIP, stop/seek/panic, project recall and legacy defaults, block-size invariance, and allocation-free audio processing.
- Apple's validator reported **AU VALIDATION SUCCEEDED** for both `aufx BtFp Rbjm` (Dustbox) and `aumu DbSy Rbjm` (Dustbox Synth).
- Linux portable tests and the workflow's FLIP AddressSanitizer/UndefinedBehaviorSanitizer check passed. Local sanitizer checks also passed for the new synth sequencer; local leak detection was disabled under ptrace.
- The native editor PNG was rendered by the processor test runner and visually reviewed. The drum/synth sequencers, step editor and keyboard fit inside the 1100 × 1380 layout. See the [current native and web screenshots](screenshots/README.md).

The run's **Dustbox-macOS-development-build** artifact contains all four AU/standalone ZIPs. **Beat-Flip-editor-preview** contains the native screenshot.

## Verified web behavior

Nine web engine/static checks passed. Real desktop and mobile browser checks passed for three immediate live notes and note-offs within the same first bar at 100% wet FLIP, held live notes through STOP, PANIC, programmed synth chords, note/chord editing, and responsive layout. The tested synth-sequencer update is published in the [web workstation](https://beat-flip-web.tj25h4ksw8.chatgpt.site).

## Remaining host checks

Manual playback, hardware MIDI, automation, and project recall in Logic on a physical Apple Silicon Mac remain unverified. The universal build and Intel-runner AU validation do not replace that host check. Development artifacts have no Developer ID signature, notarization, or installer.

## Earlier validation records

### Version 0.3 Drum Lab validation

Portable drum checks pass at 8, 44.1, 48 and 96 kHz: eight finite bounded voices, mutes, block-size invariance, stop, audition, swing, host seeks and allocation-free processing. Address/undefined behavior checks pass (local LeakSanitizer disabled because this execution host runs under ptrace; CI retains its default leak checks). All thirteen existing FLIP engine groups also pass.

The macOS workflow validates the universal AU, complete processor state/legacy migration, allocation-free drum processing, and the expanded editor snapshot. Check the latest Drum Lab branch or PR checks for v0.3 macOS results. The historical validation details below refer to v0.2.

### Validation status — 0.2.0

#### Verified locally

- The portable engine compiles with GCC 13.3 using C++17 and compiler warnings enabled.
- All thirteen behavioral test groups pass in the optimized build.
- The same groups pass with AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection is disabled in this local run; GitHub CI uses the default sanitizer settings.
- Tests include effect selection, downbeat protection, 2/4/8/16 subdivisions, repeat/gate swing, automatic changes across one-bar loops, buffer-size independence, and all six presets.
- Audio processing and pattern changes allocate no memory in the allocation probe.

#### macOS checks for 0.2.0

The GitHub workflow builds universal Intel + Apple Silicon AU and standalone bundles. The macOS integration tests check preset state recall, existing AU parameter version hints, v0.1 project migration, KEEP, allocation-free processing through the complete wrapper, and an editor PNG. The workflow then installs the AU and runs Apple's validator.

[Build run #3](https://github.com/robathanjames/beat-flip-au/actions/runs/37260189717) passed on October 4, 2026 (Pacific time): Linux behavioral tests and default sanitizer checks, universal Intel + Apple Silicon AU/standalone compilation, both macOS test suites, installation, and **AU VALIDATION SUCCEEDED**. The editor PNG was rendered and visually reviewed; all controls fit within the expanded layout.

The downloadable development build and editor preview are attached to that run. These checks cover the 0.2 feature commit `c290d99e5b465d6f8890136ec72f73a4c401f4cc`.

#### Previously verified baseline — 0.1.0

[Build run #2](https://github.com/robathanjames/beat-flip-au/actions/runs/37156414576) passed on October 3, 2026: Linux tests and default sanitizer checks, universal macOS compilation, and **AU VALIDATION SUCCEEDED** after installation and ad hoc signing.

#### Logic host check

Actual playback, editor interaction, automation, and project recall in Logic still require a host check. AU validation on an Intel runner does not establish native Apple Silicon playback.

1. Insert on mono and stereo drum tracks. The initial insert should leave audio unchanged.
2. Play a drum loop, wait for capture, and press FLIP. A new pattern should start at the next cell.
3. Select each preset. Change the effect palette, speed, swing and downbeat protection.
4. Use a one-bar Logic cycle with Auto Flip every two bars. Check that variations change at the expected bar starts.
5. Press KEEP, then play several more bars. The variation should stay fixed.
6. Turn Amount or Mix down to zero and compare with the original track after the ramp/cell boundary settles.
7. Stop, seek, and change tempo. The plugin should capture fresh input and restart its sequence.
8. Save, close, and reopen the project. All controls and the kept seed should return.
9. Open an existing 0.1 project after using a 0.2 preset. Check that the new controls return to the original-sound defaults.
10. Automate the controls and bounce the track. Confirm that the result matches playback.

Development artifacts have no Developer ID signature, notarization, or installer.

