#pragma once
#include "GlitchEngine.h"
#include "WavetableSynth.h"
#include <array>

namespace beatflip {
struct SynthStep {
    int note = -1; // -1 = rest; otherwise MIDI 0..127.
    int chord = 0; // single, major, minor, sus2, octave
    float velocity = .8f, gate = .65f;
};
using SynthPattern = std::array<SynthStep, 16>;
SynthPattern synthPattern(int preset) noexcept;
std::array<int, 3> synthChord(int root, int type) noexcept;
struct SynthSequenceSettings {
    SynthPattern pattern {};
    bool playing = false;
    float swing = 0;
};
class SynthSequencer {
public:
    void prepare(double rate);
    void reset() noexcept;
    void panic() noexcept;
    void process(float* const*, int channels, int frames, const SynthSequenceSettings&,
                 const SynthSettings&, const Transport&) noexcept;
    int currentStep() const noexcept { return step; }
    int activeVoices() const noexcept { return synth.activeVoices(); }
private:
    WavetableSynth synth;
    std::array<int,3> held { -1,-1,-1 };
    void release() noexcept;
    double rate = 48000, freePpq = 0, expectedPpq = 0, remaining = 0, lastPhase = -1;
    int step = -1;
    bool wasPlaying = false;
};
}
