#pragma once
#include "DrumMachine.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace beatflip {
// Standard MIDI File format 0, one 4/4 bar, channel 10, 960 ticks/quarter.
// Export the programmed source notes; audio slice effects have no MIDI equivalent.
inline std::vector<std::uint8_t> exportDrumMidi (const DrumSettings& settings, double bpm)
{
    constexpr std::array<int, 8> notes { 36, 38, 39, 42, 46, 41, 37, 51 };
    struct Event { int tick; std::vector<std::uint8_t> bytes; };
    std::vector<Event> events;
    bpm = std::isfinite(bpm) ? std::clamp(bpm, 20.0, 400.0) : 120.0;
    const auto tempo = static_cast<unsigned>(std::lround(60000000.0 / bpm));
    events.push_back({0, {0xff, 0x51, 3, static_cast<std::uint8_t>(tempo >> 16), static_cast<std::uint8_t>(tempo >> 8), static_cast<std::uint8_t>(tempo)}});
    events.push_back({0, {0xff, 0x58, 4, 4, 2, 24, 8}});
    const float swing = std::isfinite(settings.swing) ? std::clamp(settings.swing, 0.0f, .6f) : 0.0f;
    for (int track = 0; track < drumTracks; ++track) {
        const float level = std::isfinite(settings.levels[track]) ? std::clamp(settings.levels[track], 0.0f, 1.0f) : 0.0f;
        if (settings.muted[track] || level == 0) continue;
        for (int step = 0; step < 16; ++step) {
            const int hit = settings.pattern[track][step];
            if (hit <= 0) continue;
            const int tick = step * 240 + (step % 2 ? static_cast<int>(std::lround(swing * .48 * 240)) : 0);
            const auto velocity = static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(127 * level * (hit == 2 ? 1.0f : .68f))), 1, 127));
            const auto note = static_cast<std::uint8_t>(notes[track]);
            events.push_back({tick, {0x99, note, velocity}});
            events.push_back({tick + 60, {0x89, note, 0}});
        }
    }
    std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.tick < b.tick; });
    events.push_back({3840, {0xff, 0x2f, 0}}); // Preserve the complete bar, even for a blank pattern.
    std::vector<std::uint8_t> track;
    auto delta = [&track](unsigned value) {
        std::uint8_t bytes[4]; int count = 0;
        bytes[count++] = static_cast<std::uint8_t>(value & 0x7f);
        while ((value >>= 7) != 0) bytes[count++] = static_cast<std::uint8_t>((value & 0x7f) | 0x80);
        while (count > 0) track.push_back(bytes[--count]);
    };
    int previous = 0;
    for (const auto& event : events) {
        delta(static_cast<unsigned>(event.tick - previous)); previous = event.tick;
        track.insert(track.end(), event.bytes.begin(), event.bytes.end());
    }
    std::vector<std::uint8_t> file { 'M','T','h','d', 0,0,0,6, 0,0, 0,1, 3,0xc0, 'M','T','r','k' };
    const auto length = static_cast<std::uint32_t>(track.size());
    for (int shift : {24,16,8,0}) file.push_back(static_cast<std::uint8_t>(length >> shift));
    file.insert(file.end(), track.begin(), track.end());
    return file;
}
}
