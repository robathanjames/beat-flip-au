#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace beatflip {
struct SynthSettings {
    int bank = 0;
    float position = .35f, level = .65f, tune = 0;
    float attack = .01f, decay = .25f, sustain = .65f, release = .5f, cutoff = 9000;
};
class WavetableSynth {
public:
    static constexpr int polyphony = 16;
    void prepare(double sampleRate);
    void reset() noexcept;
    void setSettings(const SynthSettings&) noexcept;
    void noteOn(int channel, int note, float velocity) noexcept;
    void noteOff(int channel, int note) noexcept;
    void sustainPedal(int channel, bool down) noexcept;
    void pitchWheel(int channel, int value) noexcept;
    void allNotesOff(int channel) noexcept;
    void allSoundOff(int channel = 0) noexcept;
    void render(float* const* output, int channels, int start, int count) noexcept;
    int activeVoices() const noexcept;
private:
    enum class Stage { off, attack, decay, sustain, release };
    struct Voice {
        Stage stage = Stage::off;
        int note = 60, channel = 1;
        bool held = false;
        double phase = 0;
        float envelope = 0, velocity = 0, filtered = 0, last = 0, tail = 0;
        int tailSamples = 0;
        std::uint64_t age = 0;
    };
    static constexpr std::size_t tableSize = 512, frames = 4, bands = 8;
    using Table = std::array<float, tableSize + 1>;
    std::array<std::array<std::array<Table, bands>, frames>, 3> tables {};
    std::array<Voice, polyphony> voices {};
    std::array<bool,16> pedal {};
    std::array<float,16> bend {};
    SynthSettings settings;
    double rate = 48000;
    float morph = .35f, gain = .65f, cutoff = 9000, tune = 0;
    float smoothing = .002f;
    std::uint64_t clock = 0;
    float oscillator(Voice&, double frequency) noexcept;
};
}
