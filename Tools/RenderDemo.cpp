#include "GlitchEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace
{
void word (std::ofstream& file, std::uint32_t value, int bytes)
{
    for (int i = 0; i < bytes; ++i) file.put (static_cast<char> ((value >> (i * 8)) & 0xffu));
}
}

int main (int argc, char** argv)
{
    constexpr double rate = 44100.0, bpm = 140.0;
    constexpr double pi = 3.14159265358979323846;
    const auto beatLength = rate * 60.0 / bpm;
    const auto barLength = beatLength * 4.0;
    const auto frames = static_cast<int> (barLength * 8.0);
    std::vector<float> input (static_cast<std::size_t> (frames));
    std::uint32_t noise = 12345;
    for (int frame = 0; frame < frames; ++frame)
    {
        const auto inBar = std::fmod (static_cast<double> (frame), barLength);
        const auto sixteenth = static_cast<int> (inBar / (beatLength * 0.25));
        const auto time = std::fmod (inBar, beatLength * 0.25) / rate;
        noise ^= noise << 13; noise ^= noise >> 17; noise ^= noise << 5;
        const auto random = static_cast<double> (noise & 0xffffu) / 32767.5 - 1.0;
        double value = random * std::exp (-time * 180.0) * (sixteenth % 2 == 0 ? 0.13 : 0.065);
        if (sixteenth == 0 || sixteenth == 6 || sixteenth == 8 || sixteenth == 11)
            value += std::sin (2.0 * pi * (48.0 * time + 1.5 * (1.0 - std::exp (-time * 45.0)))) * std::exp (-time * 22.0) * 0.55;
        if (sixteenth == 4 || sixteenth == 12)
            value += (random * 0.55 + std::sin (2.0 * pi * 180.0 * time) * 0.16) * std::exp (-time * 42.0);
        input[static_cast<std::size_t> (frame)] = static_cast<float> (value);
    }

    beatflip::GlitchEngine engine;
    engine.prepare (rate);
    beatflip::Settings settings;
    settings.amount = settings.mix = 1.0f;
    settings.seed = 8123;
    for (int frame = 0; frame < frames;)
    {
        const auto change = static_cast<int> (barLength * 4.0);
        auto count = std::min (256, frames - frame);
        if (frame < change) count = std::min (count, change - frame);
        settings.enabled = frame >= change;
        beatflip::Transport transport;
        transport.bpm = bpm;
        transport.hasPosition = true;
        transport.ppq = static_cast<double> (frame) / beatLength;
        float* channel = input.data() + frame;
        engine.process (&channel, 1, count, settings, transport);
        frame += count;
    }

    const auto path = argc > 1 ? argv[1] : "Beat-Flip-AB.wav";
    std::ofstream file (path, std::ios::binary);
    if (! file) { std::cerr << "Cannot write " << path << '\n'; return 1; }
    file.write ("RIFF", 4); word (file, 36u + static_cast<std::uint32_t> (frames) * 2u, 4);
    file.write ("WAVEfmt ", 8); word (file, 16, 4); word (file, 1, 2); word (file, 1, 2);
    word (file, 44100, 4); word (file, 88200, 4); word (file, 2, 2); word (file, 16, 2);
    file.write ("data", 4); word (file, static_cast<std::uint32_t> (frames) * 2u, 4);
    for (const auto value : input)
        word (file, static_cast<std::uint16_t> (static_cast<std::int16_t> (std::round (std::clamp (value, -1.0f, 1.0f) * 32767.0f))), 2);
    if (! file) { std::cerr << "Write failed\n"; return 1; }
    std::cout << path << ": 4 bars dry, then 4 bars flipped; 140 BPM\n";
}
