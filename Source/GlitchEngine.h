#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace beatflip
{
constexpr int stepCount = 16;
constexpr int maxChannels = 2;

enum class Effect : std::uint8_t { clean, repeat, reverse, shuffle, gate, halfSpeed };

struct Step
{
    Effect effect = Effect::clean;
    std::uint8_t source = 0;
    std::uint8_t repeats = 2;
    float threshold = 1.0f;
};

using Pattern = std::array<Step, stepCount>;
Pattern makePattern (std::uint32_t seed) noexcept;
const char* effectName (Effect effect) noexcept;

struct Settings
{
    std::uint32_t seed = 1;
    float amount = 0.7f;
    float mix = 0.85f;
    float outputGain = 1.0f;
    bool enabled = false;
};

// Position describes the FIRST sample in a block. PPQ is measured in quarter notes.
struct Transport
{
    double bpm = 120.0;
    double ppq = 0.0;
    double barStartPpq = 0.0;
    int numerator = 4;
    int denominator = 4;
    bool hasPosition = false;
    bool hasBarStart = false;
    bool playing = true;
    bool looping = false;
    double loopStartPpq = 0.0;
    double loopEndPpq = 0.0;
};

class GlitchEngine
{
public:
    void prepare (double newSampleRate);
    void reset() noexcept;

    // In-place mono/stereo. All storage is allocated by prepare(), never process().
    void process (float* const* channels, int numChannels, int numSamples,
                  const Settings&, const Transport&) noexcept;

    int getCurrentStep() const noexcept { return currentStep; }
    std::uint32_t getActiveSeed() const noexcept { return activeSeed; }
    bool isCapturing() const noexcept { return capturing; }

private:
    float readHistory (int channel, double absolutePosition) const noexcept;
    bool windowAvailable (double start, double length) const noexcept;
    void clearHistory() noexcept;

    static constexpr double maxBarSeconds = 16.0;
    std::array<std::vector<float>, maxChannels> history;
    std::array<float, maxChannels> lastWet {};
    std::array<float, maxChannels> fadeFrom {};
    Pattern pattern = makePattern (1);
    Step activeStep {};
    double sampleRate = 48000.0;
    double freePpq = 0.0;
    double expectedPpq = 0.0;
    double lastPhase = -1.0;
    double previousBpm = 120.0;
    double previousBarBeats = 4.0;
    double sourceStart = 0.0;
    double sliceSamples = 1.0;
    double repeatSamples = 1.0;
    double smoothing = 0.004;
    std::uint64_t written = 0;
    std::size_t capacity = 0;
    std::uint32_t activeSeed = 1;
    int currentStep = -1;
    int lastRepeat = -1;
    int fadeLength = 64;
    int fadeRemaining = 0;
    float smoothedMix = 0.0f;
    float smoothedGain = 1.0f;
    bool hadPosition = false;
    bool wasPlaying = false;
    bool capturing = true;
    bool sliceReady = false;
    bool fadeClean = false;
};
}
