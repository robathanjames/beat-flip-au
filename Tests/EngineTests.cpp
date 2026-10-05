#include "GlitchEngine.h"
#include "AllocationGuard.h"
#include "FactoryPresets.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr double sampleRate = 8192.0;
constexpr int barSamples = 16384; // 120 BPM, 4/4
constexpr int sliceSamples = barSamples / beatflip::stepCount;

void require (bool condition, const char* message)
{
    if (! condition) throw std::runtime_error (message);
}

std::vector<float> signal (int size)
{
    std::vector<float> result (static_cast<std::size_t> (size));
    for (int i = 0; i < size; ++i)
        result[static_cast<std::size_t> (i)] = static_cast<float> (0.42 * std::sin (i * 0.041) + 0.16 * std::cos (i * 0.007));
    return result;
}

std::vector<float> render (std::vector<float> input, beatflip::Settings settings,
                           const std::vector<int>& blocks, bool hostPosition = true)
{
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    std::size_t blockIndex = 0;
    for (int position = 0; position < static_cast<int> (input.size());)
    {
        const auto size = std::min (blocks[blockIndex++ % blocks.size()], static_cast<int> (input.size()) - position);
        beatflip::Transport transport;
        transport.hasPosition = hostPosition;
        transport.ppq = static_cast<double> (position) / (sampleRate * 0.5);
        float* channel = input.data() + position;
        engine.process (&channel, 1, size, settings, transport);
        position += size;
    }
    return input;
}

beatflip::Settings wetSettings()
{
    beatflip::Settings result;
    result.enabled = true;
    result.amount = result.mix = 1.0f;
    result.seed = 8123;
    return result;
}

void transparentPaths()
{
    const auto input = signal (barSamples * 4);
    beatflip::Settings settings;
    require (render (input, settings, { 257, 64 }) == input, "Disabled insert must leave audio unchanged");
    settings.enabled = true;
    settings.mix = 0.0f;
    require (render (input, settings, { 33, 1024 }) == input, "Zero mix must not delay or change the drum track");
    settings.mix = 1.0f;
    settings.amount = 0.0f;
    require (render (input, settings, { 1, 128 }) == input, "Zero amount must leave audio unchanged");
}

void deterministicAcrossBlocks()
{
    const auto input = signal (barSamples * 5);
    const auto settings = wetSettings();
    const auto a = render (input, settings, { 64 });
    const auto b = render (input, settings, { 1, 511, 127, 1024, 3 });
    const auto c = render (input, settings, { 193 }, false);
    double error = 0.0, difference = 0.0, freeRunError = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        error = std::max (error, std::abs (static_cast<double> (a[i] - b[i])));
        freeRunError = std::max (freeRunError, std::abs (static_cast<double> (a[i] - c[i])));
        if (i > barSamples * 2) difference += std::abs (a[i] - input[i]);
    }
    require (error < 0.0001, "Host buffer size must not change the generated rhythm");
    require (freeRunError < 0.0001, "The fallback clock must match the host clock");
    require (difference > 100.0, "FLIP must audibly change a non-repeating drum source");
}

void stereoCoherenceAndRingWrap()
{
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    auto left = signal (static_cast<int> (sampleRate * 38));
    auto right = left;
    for (auto& value : right) value *= 0.3f;
    const auto settings = wetSettings();
    for (int position = 0; position < static_cast<int> (left.size());)
    {
        const auto count = std::min (256, static_cast<int> (left.size()) - position);
        float* channels[] { left.data() + position, right.data() + position };
        beatflip::Transport transport;
        transport.hasPosition = true;
        transport.ppq = position / (sampleRate * 0.5);
        engine.process (channels, 2, count, settings, transport);
        position += count;
    }
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        require (std::isfinite (left[i]) && std::abs (left[i]) <= 0.59f, "Ring wrap must not corrupt samples or increase peaks");
        require (std::abs (right[i] - left[i] * 0.3f) < 0.00001f, "Both channels must use the same slices and phase");
    }
}

void slicesReadRecordedInput()
{
    for (const auto effect : { beatflip::Effect::shuffle, beatflip::Effect::reverse, beatflip::Effect::halfSpeed, beatflip::Effect::repeat })
    {
        auto settings = wetSettings();
        int chosenStep = -1;
        for (int i = 2; i < beatflip::stepCount; ++i)
            if (beatflip::makePattern (settings.seed)[static_cast<std::size_t> (i)].effect == effect) { chosenStep = i; break; }
        require (chosenStep >= 0, "Test seed must cover the slice effects");
        std::vector<float> input (barSamples * 4);
        for (std::size_t i = 0; i < input.size(); ++i) input[i] = static_cast<float> (i) / 200000.0f;
        const auto audio = render (input, settings, { 64 });
        const auto pattern = beatflip::makePattern (settings.seed);
        const auto step = pattern[static_cast<std::size_t> (chosenStep)];
        const int offset = 100; // Past the transition ramp and before the first repeat wrap.
        const auto frame = barSamples * 2 + chosenStep * sliceSamples + offset;
        double cursor = offset;
        if (effect == beatflip::Effect::reverse) cursor = sliceSamples - 1 - offset;
        if (effect == beatflip::Effect::halfSpeed) cursor *= 0.5;
        const auto source = barSamples + step.source * sliceSamples + cursor;
        const auto expected = static_cast<float> (source / 200000.0);
        require (std::abs (audio[static_cast<std::size_t> (frame)] - expected) < 0.00001f,
                 "Glitches must read the previous bar's dry input at the intended slice/rate");
    }
}

void flipQuantizedToNextStep()
{
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    auto audio = signal (sliceSamples + 128);
    auto settings = wetSettings();
    beatflip::Transport transport;
    transport.hasPosition = true;
    float* channel = audio.data();
    engine.process (&channel, 1, 128, settings, transport);
    require (engine.getActiveSeed() == settings.seed, "Initial pattern must activate");
    settings.seed = 72;
    transport.ppq = 128.0 / (sampleRate * 0.5);
    channel = audio.data() + 128;
    engine.process (&channel, 1, 128, settings, transport);
    require (engine.getActiveSeed() != settings.seed, "A button press in a cell waits for the next cell");
    transport.ppq = 256.0 / (sampleRate * 0.5);
    channel = audio.data() + 256;
    engine.process (&channel, 1, sliceSamples - 256 + 1, settings, transport);
    require (engine.getActiveSeed() == settings.seed, "The queued seed must apply at the next boundary");
}

void loopAndSeek()
{
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    auto settings = wetSettings();
    auto input = signal (256);
    beatflip::Transport transport;
    transport.hasPosition = transport.looping = true;
    transport.loopStartPpq = 0.0;
    transport.loopEndPpq = 4.0;
    for (int frame = 0; frame < barSamples * 4; frame += 256)
    {
        auto audio = input;
        float* channel = audio.data();
        transport.ppq = static_cast<double> (frame % barSamples) / (sampleRate * 0.5);
        engine.process (&channel, 1, 256, settings, transport);
        if (frame >= barSamples * 2) require (! engine.isCapturing(), "A one-bar host loop must retain its captured audio");
    }
    transport.looping = false;
    transport.ppq = 43.0;
    auto audio = input;
    float* channel = audio.data();
    engine.process (&channel, 1, 256, settings, transport);
    require (engine.isCapturing(), "A timeline seek must invalidate the old recorded bar");
    require (audio == input, "A seek must not replay audio from the previous timeline location");
    transport.playing = false;
    channel = audio.data();
    engine.process (&channel, 1, 256, settings, transport);
    transport.playing = true;
    transport.ppq = 0.0;
    channel = audio.data();
    engine.process (&channel, 1, 256, settings, transport);
    require (engine.isCapturing(), "Restarting playback must capture fresh input");
}

void unusualTransportAndBadValues()
{
    for (const auto numerator : { 3, 7, 32 })
    {
        beatflip::GlitchEngine engine;
        engine.prepare (sampleRate);
        auto settings = wetSettings();
        beatflip::Transport transport;
        transport.hasPosition = transport.hasBarStart = true;
        transport.numerator = numerator;
        transport.denominator = 8;
        transport.bpm = 30.0;
        transport.barStartPpq = -8.5;
        for (int frame = 0; frame < 100000; frame += 257)
        {
            auto audio = signal (257);
            transport.ppq = -7.0 + frame / (sampleRate * 2.0);
            float* channel = audio.data();
            engine.process (&channel, 1, 257, settings, transport);
            for (auto value : audio) require (std::isfinite (value), "Odd meters and negative PPQ must stay finite");
            require (engine.getCurrentStep() >= 0 && engine.getCurrentStep() < 16, "Step index must remain valid");
        }
    }
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    auto settings = wetSettings();
    settings.mix = settings.amount = settings.outputGain = std::numeric_limits<float>::quiet_NaN();
    beatflip::Transport transport;
    transport.hasPosition = true;
    transport.ppq = transport.bpm = std::numeric_limits<double>::quiet_NaN();
    float audio[] { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.5f };
    float* channel = audio;
    engine.process (&channel, 1, 3, settings, transport);
    for (auto value : audio) require (std::isfinite (value), "Invalid host/input values must not produce NaN output");
    engine.process (nullptr, 0, 0, settings, transport);
}

void realTimeNoAllocations()
{
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    auto audio = signal (2048);
    auto settings = wetSettings();
    beatflip::Transport transport;
    float* channel = audio.data();
    allocationGuard::start();
    for (int i = 0; i < 100; ++i)
    {
        settings.seed = static_cast<std::uint32_t> (i + 1);
        settings.pattern.effectMask = static_cast<std::uint8_t> (i % 32);
        settings.pattern.repeats = i % 2 == 0 ? 16 : 4;
        settings.pattern.protectDownbeat = i % 3 == 0;
        settings.swing = 0.5f;
        settings.autoFlipBars = 2;
        engine.process (&channel, 1, 2048, settings, transport);
    }
    const auto count = allocationGuard::stop();
    require (count == 0, "Audio processing and pattern changes must allocate no memory");
}

void effectPaletteAndDownbeat()
{
    const auto input = signal (barSamples * 4);
    auto settings = wetSettings();
    settings.pattern.effectMask = 0;
    require (render (input, settings, { 31, 257 }) == input, "An empty effect palette must be transparent");
    for (const auto effect : { beatflip::Effect::repeat, beatflip::Effect::reverse, beatflip::Effect::shuffle,
                               beatflip::Effect::gate, beatflip::Effect::halfSpeed })
    {
        settings.pattern.effectMask = beatflip::effectBit (effect);
        settings.pattern.protectDownbeat = false;
        const auto pattern = beatflip::makePattern (settings.seed, settings.pattern);
        for (const auto& step : pattern) require (step.effect == effect, "Only selected effects may be generated");
    }
    settings.pattern.effectMask = beatflip::effectBit (beatflip::Effect::reverse);
    settings.pattern.protectDownbeat = true;
    auto anchored = render (input, settings, { 64 });
    settings.pattern.protectDownbeat = false;
    auto unanchored = render (input, settings, { 64 });
    const auto frame = barSamples * 2 + 100;
    require (anchored[frame] == input[frame], "Downbeat protection must keep the first cell live");
    require (std::abs (unanchored[frame] - input[frame]) > 0.01f, "Turning off protection must allow a downbeat glitch");
}

void repeatSpeedAndSwing()
{
    std::vector<float> input (barSamples * 4);
    for (std::size_t i = 0; i < input.size(); ++i) input[i] = static_cast<float> (i) / 200000.0f;
    auto settings = wetSettings();
    settings.pattern.effectMask = beatflip::effectBit (beatflip::Effect::repeat);
    settings.pattern.protectDownbeat = false;
    const auto source = barSamples + 3 * sliceSamples;
    for (const auto repeats : { 2, 4, 8, 16 })
    {
        settings.pattern.repeats = repeats;
        const auto audio = render (input, settings, { 19, 511, 3 });
        const auto offset = 220;
        const auto cursor = offset % (sliceSamples / repeats);
        const auto expected = static_cast<float> (source + cursor) / 200000.0f;
        require (std::abs (audio[barSamples * 2 + 3 * sliceSamples + offset] - expected) < 0.00001f,
                 "Stutter speed must set the actual number of repeats per cell");
    }
    settings.pattern.repeats = 4;
    settings.swing = 0.5f;
    const auto swung = render (input, settings, { 17, 256 });
    for (const auto offset : { 320, 420 })
    {
        const auto cursor = offset < 384 ? offset : offset - 384;
        const auto expected = static_cast<float> (source + cursor) / 200000.0f;
        require (std::abs (swung[barSamples * 2 + 3 * sliceSamples + offset] - expected) < 0.00001f,
                 "Swing must delay the second pulse without changing the read speed");
    }
    settings.pattern.effectMask = beatflip::effectBit (beatflip::Effect::gate);
    input.assign (barSamples * 4, 0.5f);
    settings.swing = 0.0f;
    const auto straightGate = render (input, settings, { 64 });
    settings.swing = 0.5f;
    const auto swungGate = render (input, settings, { 64 });
    const auto frame = barSamples * 2 + 3 * sliceSamples + 300;
    require (straightGate[frame] > 0.49f && swungGate[frame] < 0.00001f, "Swing must also move gate openings");
}

void automaticVariations()
{
    const auto input = signal (barSamples * 10);
    auto settings = wetSettings();
    settings.autoFlipBars = 2;
    settings.pattern.repeats = 16;
    settings.swing = 0.43f;
    const auto small = render (input, settings, { 1, 127, 64 });
    const auto large = render (input, settings, { 4093, 31, 1024 });
    for (std::size_t i = 0; i < small.size(); ++i)
        require (std::abs (small[i] - large[i]) < 0.0001f, "Automatic variations and swing must be independent of buffer size");

    for (const auto bars : { 1, 2, 4, 8 })
    {
        beatflip::GlitchEngine engine;
        engine.prepare (sampleRate);
        settings.autoFlipBars = bars;
        beatflip::Transport transport;
        transport.hasPosition = transport.hasBarStart = transport.looping = true;
        transport.loopStartPpq = 0.0;
        transport.loopEndPpq = 4.0;
        auto audio = signal (256);
        for (int position = 0; position <= barSamples * bars; position += 256)
        {
            float* channel = audio.data();
            transport.ppq = (position % barSamples) / (sampleRate * 0.5);
            engine.process (&channel, 1, 256, settings, transport);
            if (position < barSamples * bars)
                require (engine.getActiveSeed() == settings.seed, "Auto Flip must wait for the selected number of bars");
        }
        require (engine.getActiveSeed() != settings.seed, "Auto Flip must advance across one-bar host loops");
        require (engine.getBaseSeed() == settings.seed, "Automatic variation must leave the saved base seed intact");
        const auto variation = engine.getActiveSeed();
        engine.reset();
        float* channel = audio.data();
        transport.ppq = 0.0;
        engine.process (&channel, 1, 256, settings, transport);
        require (engine.getActiveSeed() == settings.seed, "Playback restart must reproduce the base pattern");
        transport.looping = false;
        transport.ppq = 43.0;
        engine.process (&channel, 1, 256, settings, transport);
        require (engine.getActiveSeed() == settings.seed && engine.isCapturing(), "Seek must reset the variation sequence and capture");
        require (variation != settings.seed, "A generated variation must differ from the base");
    }
}

void paletteChangesAreQuantized()
{
    beatflip::GlitchEngine engine;
    engine.prepare (sampleRate);
    auto audio = signal (sliceSamples + 1);
    auto settings = wetSettings();
    beatflip::Transport transport;
    transport.hasPosition = true;
    float* channel = audio.data();
    engine.process (&channel, 1, 128, settings, transport);
    const auto before = engine.getPattern();
    settings.pattern = { beatflip::effectBit (beatflip::Effect::reverse), 16, false };
    transport.ppq = 128.0 / (sampleRate * 0.5);
    channel = audio.data() + 128;
    engine.process (&channel, 1, 128, settings, transport);
    require (engine.getPattern()[0].effect == before[0].effect, "Palette changes must wait for a grid boundary");
    transport.ppq = 256.0 / (sampleRate * 0.5);
    channel = audio.data() + 256;
    engine.process (&channel, 1, sliceSamples - 256 + 1, settings, transport);
    for (const auto& step : engine.getPattern())
        require (step.effect == beatflip::Effect::reverse && step.repeats == 16, "New palette and speed must apply at the next cell");
}

void factoryPresetsAndInvalidControls()
{
    const auto input = signal (barSamples * 5);
    for (const auto& preset : beatflip::factoryPresets())
    {
        const auto audio = render (input, preset.settings, { 37, 511 });
        double difference = 0.0;
        for (std::size_t i = barSamples * 2; i < audio.size(); ++i)
        {
            require (std::isfinite (audio[i]) && std::abs (audio[i]) <= 0.59f, "Every preset must produce finite, bounded output");
            difference += std::abs (audio[i] - input[i]);
        }
        require (difference > 1.0, "Every factory preset must audibly process the drums");
    }
    auto invalid = wetSettings();
    invalid.swing = std::numeric_limits<float>::quiet_NaN();
    invalid.pattern.effectMask = 255;
    invalid.pattern.repeats = -99;
    invalid.autoFlipBars = std::numeric_limits<int>::max();
    const auto safe = render (input, invalid, { 127 });
    for (const auto value : safe) require (std::isfinite (value), "Invalid new controls must remain safe");
}
}

int main()
{
    const std::pair<const char*, void (*)()> tests[] {
        { "Transparent dry / disabled / zero amount", transparentPaths },
        { "Deterministic rhythm across host buffer sizes", deterministicAcrossBlocks },
        { "Stereo coherence and long-run ring wrap", stereoCoherenceAndRingWrap },
        { "Recorded slice positions, reverse, repeats and pitch", slicesReadRecordedInput },
        { "FLIP quantizes to the next grid step", flipQuantizedToNextStep },
        { "Host loops, seeks and playback restart", loopAndSeek },
        { "Odd meter, pickup positions and invalid host values", unusualTransportAndBadValues },
        { "No allocations on the audio thread", realTimeNoAllocations },
        { "Effect palette and optional downbeat protection", effectPaletteAndDownbeat },
        { "Stutter speed and swung repeat/gate timing", repeatSpeedAndSwing },
        { "Automatic variations, buffer sizes, loops and restart", automaticVariations },
        { "Palette edits quantize to the next cell", paletteChangesAreQuantized },
        { "Factory presets and invalid new controls", factoryPresetsAndInvalidControls }
    };
    int failures = 0;
    for (const auto& test : tests)
    {
        try { test.second(); std::cout << "PASS  " << test.first << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL  " << test.first << ": " << error.what() << '\n'; }
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
