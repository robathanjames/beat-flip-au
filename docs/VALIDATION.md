# Validation status — 0.2.0

## Verified locally

- The portable engine compiles with GCC 13.3 using C++17 and compiler warnings enabled.
- All thirteen behavioral test groups pass in the optimized build.
- The same groups pass with AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection is disabled in this local run; GitHub CI uses the default sanitizer settings.
- Tests include effect selection, downbeat protection, 2/4/8/16 subdivisions, repeat/gate swing, automatic changes across one-bar loops, buffer-size independence, and all six presets.
- Audio processing and pattern changes allocate no memory in the allocation probe.

## macOS checks for 0.2.0

The GitHub workflow builds universal Intel + Apple Silicon AU and standalone bundles. The macOS integration tests check preset state recall, existing AU parameter version hints, v0.1 project migration, KEEP, allocation-free processing through the complete wrapper, and an editor PNG. The workflow then installs the AU and runs Apple's validator.

The result of the feature build will be recorded here after it completes. Do not infer 0.2 validation from the prior 0.1 build.

## Previously verified baseline — 0.1.0

[Build run #2](https://github.com/robathanjames/beat-flip-au/actions/runs/37156414576) passed on October 3, 2026: Linux tests and default sanitizer checks, universal macOS compilation, and **AU VALIDATION SUCCEEDED** after installation and ad hoc signing.

## Logic host check

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
