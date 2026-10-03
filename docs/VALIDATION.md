# Validation status — 0.1.0

## Verified locally

- The portable engine compiles with GCC 13.3 using C++17 and compiler warnings enabled.
- All eight behavioral test groups pass in the optimized build.
- The same test groups pass with AddressSanitizer and UndefinedBehaviorSanitizer. Leak detection is disabled for this local run because the preparation environment blocks LeakSanitizer's process inspection; the CI sanitizer command retains its defaults.
- The audio callback and pattern changes allocate no memory in the allocation probe.
- The A/B renderer produces a valid mono 44.1 kHz PCM WAV containing four dry bars and four flipped bars.

## Verified on GitHub Actions — October 3, 2026

[Build run #2](https://github.com/robathanjames/beat-flip-au/actions/runs/37156414576) passed both jobs:

- Linux behavioral tests and default AddressSanitizer/UndefinedBehaviorSanitizer checks passed.
- The AU wrapper, custom editor, and standalone app compiled as universal Intel + Apple Silicon macOS bundles.
- Apple's `auval -v aufx BtFp Rbjm` reported **AU VALIDATION SUCCEEDED** after installation, ad hoc signing, and refreshing AudioComponentRegistrar.
- The run includes downloadable AU and standalone development ZIPs.

## Still requires a Logic host check

Actual playback, editor interaction, automation, and project recall in Logic have not been tested. AU validation ran on the Intel macOS runner; native Apple Silicon host playback still needs a check.

After a successful AU build, check in Logic:

1. Insert on mono and stereo drum tracks. The initial insert should leave the audio unchanged.
2. Play a drum loop, wait for capture, and press FLIP. A new pattern should start on the next grid cell.
3. Loop a one-bar region for several repetitions. The effect should stay active across loop boundaries.
4. Turn Amount or Mix down to zero and compare with the original track. Allow the mix ramp/grid boundary to settle.
5. Stop, seek, and change tempo. The plugin should capture fresh input and resume after an aligned bar.
6. Save, close, and reopen the project. The controls and pattern seed should return to their saved values.
7. Automate Pattern Seed and Glitch Enabled, then bounce the track. Confirm that the result matches playback.

Development artifacts have no Developer ID signature, notarization, or installer. Treat 0.1.0 as a working DSP prototype with host integration still to validate.
