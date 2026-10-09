#pragma once
#include "GlitchEngine.h"
#include <array>
#include <atomic>
#include <vector>
namespace beatflip {
constexpr int drumTracks = 8;
using DrumPattern = std::array<std::array<int, 16>, drumTracks>;
inline constexpr std::array<const char*, 8> drumNames { "KICK", "SNARE", "CLAP", "CLOSED HAT", "OPEN HAT", "LOW TOM", "RIM", "RIDE" };
DrumPattern drumGroove (int index) noexcept;
struct DrumSettings {
    DrumPattern pattern {};
    std::array<float, 8> levels { .8f,.8f,.8f,.8f,.8f,.8f,.8f,.8f };
    std::array<bool, 8> muted {};
    float swing = 0, dust = .35f;
    bool playing = false;
};
class DrumMachine {
public:
    void prepare (double rate);
    void reset() noexcept;
    void process (float* const*, int channels, int frames, const DrumSettings&, const Transport&, unsigned audition = 0, const std::array<std::atomic<float>,8>* velocity = nullptr) noexcept;
    int currentStep() const noexcept { return step; }
private:
    std::array<std::vector<float>, 8> bank;
    std::array<int, 8> cursors {};
    std::array<float, 8> gains {};
    double sampleRate = 48000, freePpq = 0, expectedPpq = 0;
    float low = 0, held = 0;
    int step = -1, holdCounter = 0;
    bool wasPlaying = false;
};
}
