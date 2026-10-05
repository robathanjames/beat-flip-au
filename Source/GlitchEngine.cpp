#include "GlitchEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace beatflip
{
namespace
{
std::uint32_t nextRandom (std::uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

double positiveModulo (double value, double modulus) noexcept
{
    const auto result = std::fmod (value, modulus);
    return result < 0.0 ? result + modulus : result;
}

template <typename T>
T finiteOr (T value, T fallback) noexcept
{
    return std::isfinite (value) ? value : fallback;
}

PatternOptions safeOptions (PatternOptions options) noexcept
{
    options.effectMask &= allEffects;
    if (options.repeats != 2 && options.repeats != 4 && options.repeats != 8 && options.repeats != 16)
        options.repeats = 0;
    return options;
}

bool sameOptions (const PatternOptions& a, const PatternOptions& b) noexcept
{
    return a.effectMask == b.effectMask && a.repeats == b.repeats && a.protectDownbeat == b.protectDownbeat;
}

std::uint32_t variationSeed (std::uint32_t seed, std::uint64_t variation) noexcept
{
    if (variation == 0) return seed;
    // A deterministic full-period walk through the host's seed range. No random
    // device or host parameter notification is needed on the audio thread.
    return static_cast<std::uint32_t> ((static_cast<std::uint64_t> (seed - 1u)
                                      + (variation % 1048575u) * 7919u) % 1048575u + 1u);
}
}

Pattern makePattern (std::uint32_t seed, const PatternOptions& requestedOptions) noexcept
{
    const auto options = safeOptions (requestedOptions);
    constexpr std::array<Effect, 10> weightedEffects {
        Effect::repeat, Effect::repeat, Effect::repeat, Effect::shuffle, Effect::shuffle,
        Effect::reverse, Effect::reverse, Effect::gate, Effect::gate, Effect::halfSpeed
    };
    std::array<Effect, 10> available {};
    unsigned count = 0;
    for (auto effect : weightedEffects)
        if ((options.effectMask & effectBit (effect)) != 0) available[count++] = effect;
    // A fixed integer PRNG makes a saved seed identical across machines and runs.
    auto state = seed ^ 0x9e3779b9u;
    if (state == 0) state = 0x6d2b79f5u;
    Pattern result {};
    for (int i = 0; i < stepCount; ++i)
    {
        auto& step = result[static_cast<std::size_t> (i)];
        const auto pick = nextRandom (state);
        step.effect = count == 0 ? Effect::clean : available[pick % count];
        step.source = static_cast<std::uint8_t> (nextRandom (state) % stepCount);
        if (step.source == i) step.source = static_cast<std::uint8_t> ((i + 5) % stepCount);
        step.repeats = static_cast<std::uint8_t> (2u << (nextRandom (state) % 3u));
        if (options.repeats != 0) step.repeats = static_cast<std::uint8_t> (options.repeats);
        step.threshold = 0.08f + 0.90f * static_cast<float> (nextRandom (state) & 0xffffu) / 65535.0f;
        if (step.effect != Effect::shuffle) step.source = static_cast<std::uint8_t> (i);
    }
    // Keep the downbeat recognizable. The rest of the bar is fair game.
    if (options.protectDownbeat) result[0].effect = Effect::clean;
    return result;
}

const char* effectName (Effect effect) noexcept
{
    switch (effect)
    {
        case Effect::clean: return "LIVE";
        case Effect::repeat: return "STUT";
        case Effect::reverse: return "REV";
        case Effect::shuffle: return "JUMP";
        case Effect::gate: return "CUT";
        case Effect::halfSpeed: return "DRAG";
    }
    return "LIVE";
}

void GlitchEngine::prepare (double newSampleRate)
{
    sampleRate = std::clamp (finiteOr (newSampleRate, 48000.0), 8000.0, 384000.0);
    capacity = static_cast<std::size_t> (std::ceil (sampleRate * (2.0 * maxBarSeconds + 1.0))) + 4;
    for (auto& channel : history) channel.assign (capacity, 0.0f);
    fadeLength = std::max (8, static_cast<int> (std::round (sampleRate * 0.0015)));
    smoothing = 1.0 - std::exp (-1.0 / (sampleRate * 0.005));
    reset();
}

void GlitchEngine::clearHistory() noexcept
{
    // Old memory is invalidated by the absolute sample count; no buffer clearing on the audio thread.
    written = 0;
    currentStep = -1;
    lastPhase = -1.0;
    lastRepeat = -1;
    fadeRemaining = 0;
    lastWet.fill (0.0f);
    fadeFrom.fill (0.0f);
    capturing = true;
    sliceReady = false;
    fadeClean = false;
    activeStep = {};
    autoBarCount = autoVariation = 0;
    activeAutoFlipBars = -1;
}

void GlitchEngine::reset() noexcept
{
    clearHistory();
    freePpq = expectedPpq = 0.0;
    previousBpm = 120.0;
    previousBarBeats = 4.0;
    hadPosition = wasPlaying = false;
    smoothedMix = 0.0f;
    smoothedGain = 1.0f;
}

bool GlitchEngine::windowAvailable (double start, double length) const noexcept
{
    const auto oldest = written > capacity ? static_cast<double> (written - capacity) : 0.0;
    return start >= oldest && start + length <= static_cast<double> (written);
}

float GlitchEngine::readHistory (int channel, double position) const noexcept
{
    if (! std::isfinite (position) || position < 0.0) return 0.0f;
    const auto index = static_cast<std::uint64_t> (std::floor (position));
    if (index >= written || (written > capacity && index < written - capacity)) return 0.0f;
    const auto first = history[static_cast<std::size_t> (channel)][static_cast<std::size_t> (index % capacity)];
    const auto next = index + 1 < written
        ? history[static_cast<std::size_t> (channel)][static_cast<std::size_t> ((index + 1) % capacity)] : first;
    return first + static_cast<float> (position - std::floor (position)) * (next - first);
}

void GlitchEngine::process (float* const* channels, int numChannels, int numSamples,
                           const Settings& settings, const Transport& transport) noexcept
{
    if (capacity == 0 || channels == nullptr || numChannels < 1 || numSamples < 1) return;
    numChannels = std::min (numChannels, maxChannels);
    const auto bpm = std::clamp (finiteOr (transport.bpm, 120.0), 20.0, 400.0);
    const auto numerator = std::clamp (transport.numerator, 1, 32);
    const auto denominator = std::clamp (transport.denominator, 1, 32);
    const auto barBeats = static_cast<double> (numerator) * 4.0 / denominator;
    const auto beatSamples = sampleRate * 60.0 / bpm;
    const auto barSamples = beatSamples * barBeats;
    const auto stepBeats = barBeats / stepCount;
    const auto ppqIncrement = 1.0 / beatSamples;
    const auto positionValid = transport.hasPosition && std::isfinite (transport.ppq);
    const auto amount = std::clamp (finiteOr (settings.amount, 0.7f), 0.0f, 1.0f);
    const auto mix = std::clamp (finiteOr (settings.mix, 0.85f), 0.0f, 1.0f);
    const auto gain = std::clamp (finiteOr (settings.outputGain, 1.0f), 0.0f, 2.0f);
    const auto swing = static_cast<double> (std::clamp (finiteOr (settings.swing, 0.0f), 0.0f, 0.75f));
    const auto options = safeOptions (settings.pattern);
    const auto autoFlipBars = std::clamp (settings.autoFlipBars, 0, 8);
    auto ppq = positionValid ? transport.ppq : freePpq;
    const auto anchor = transport.hasBarStart && std::isfinite (transport.barStartPpq)
        ? transport.barStartPpq : 0.0;

    bool seek = false;
    if (positionValid && hadPosition && transport.playing && wasPlaying)
    {
        const auto difference = ppq - expectedPpq;
        const auto tolerance = std::max (1.0e-6, ppqIncrement * 4.0);
        if (std::abs (difference) > tolerance)
        {
            const auto loopLength = transport.loopEndPpq - transport.loopStartPpq;
            const auto expectedLoopPosition = transport.loopStartPpq
                + positiveModulo (expectedPpq - transport.loopStartPpq, std::max (loopLength, 1.0e-9));
            const auto wrapsLoop = transport.looping && std::isfinite (loopLength) && loopLength > 0.0
                && expectedPpq >= transport.loopEndPpq - tolerance
                && std::abs (ppq - expectedLoopPosition) <= tolerance;
            seek = ! wrapsLoop;
        }
    }
    const auto tempoChanged = std::abs (bpm - previousBpm) > 1.0e-6
                           || std::abs (barBeats - previousBarBeats) > 1.0e-6;
    if (seek || tempoChanged || (transport.playing && ! wasPlaying)
        || (positionValid != hadPosition && wasPlaying))
        clearHistory();

    const auto supportedBar = barSamples <= sampleRate * maxBarSeconds;
    const auto targetMix = settings.enabled && transport.playing && supportedBar ? mix : 0.0f;

    for (int frame = 0; frame < numSamples; ++frame)
    {
        std::array<float, maxChannels> dry {};
        for (int ch = 0; ch < numChannels; ++ch)
            dry[static_cast<std::size_t> (ch)] = finiteOr (channels[ch][frame], 0.0f);

        const auto phase = positiveModulo (ppq - anchor, barBeats);
        const auto index = std::clamp (static_cast<int> (std::floor ((phase + 1.0e-10) / stepBeats)), 0, stepCount - 1);
        const auto stepPhase = std::clamp ((phase - index * stepBeats) * beatSamples, 0.0, barSamples / stepCount);
        const auto stepBoundary = index != currentStep || phase < lastPhase - 1.0e-7;

        if (stepBoundary)
        {
            const auto previousEffect = activeStep.effect;
            const auto barBoundary = currentStep >= 0 && index == 0 && phase < lastPhase - 1.0e-7;
            currentStep = index;
            if (activeBaseSeed != settings.seed || activeAutoFlipBars != autoFlipBars)
            {
                activeBaseSeed = settings.seed;
                activeAutoFlipBars = autoFlipBars;
                autoBarCount = autoVariation = 0;
            }
            else if (barBoundary && settings.enabled && autoFlipBars > 0)
            {
                ++autoBarCount;
                if (autoBarCount % static_cast<unsigned> (autoFlipBars) == 0) ++autoVariation;
            }
            const auto nextSeed = variationSeed (activeBaseSeed, autoVariation);
            if (activeSeed != nextSeed || ! sameOptions (options, activeOptions))
            {
                activeSeed = nextSeed;
                activeOptions = options;
                pattern = makePattern (activeSeed, activeOptions);
            }
            activeStep = pattern[static_cast<std::size_t> (index)];
            if (amount < activeStep.threshold) activeStep.effect = Effect::clean;
            sliceSamples = barSamples / stepCount;
            repeatSamples = std::max (1.0, sliceSamples / activeStep.repeats);
            const auto previousBarStart = static_cast<double> (written) - phase * beatSamples - barSamples;
            sourceStart = previousBarStart + activeStep.source * sliceSamples;
            sliceReady = windowAvailable (sourceStart, sliceSamples);
            capturing = ! supportedBar || previousBarStart < 0.0;
            if (capturing || ! sliceReady) activeStep.effect = Effect::clean;
            fadeFrom = lastWet;
            fadeRemaining = fadeLength;
            fadeClean = previousEffect != Effect::clean;
            lastRepeat = -1;
        }
        lastPhase = phase;

        const auto pairPhase = positiveModulo (stepPhase, repeatSamples * 2.0);
        const auto firstPulseLength = repeatSamples * (1.0 + swing);
        const auto secondPulse = pairPhase >= firstPulseLength;
        const auto repeatIndex = static_cast<int> (stepPhase / (repeatSamples * 2.0)) * 2 + (secondPulse ? 1 : 0);
        const auto pulsePhase = secondPulse ? pairPhase - firstPulseLength : pairPhase;
        const auto pulseLength = repeatSamples * (secondPulse ? 1.0 - swing : 1.0 + swing);
        if (activeStep.effect == Effect::repeat && repeatIndex != lastRepeat)
        {
            fadeFrom = lastWet;
            fadeRemaining = fadeLength;
        }
        lastRepeat = repeatIndex;

        smoothedMix += static_cast<float> (smoothing) * (targetMix - smoothedMix);
        smoothedGain += static_cast<float> (smoothing) * (gain - smoothedGain);
        const auto fade = fadeRemaining > 0 ? 1.0f - static_cast<float> (fadeRemaining) / static_cast<float> (fadeLength) : 1.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto channelIndex = static_cast<std::size_t> (ch);
            auto wet = dry[channelIndex];
            if (transport.playing && supportedBar)
            {
                switch (activeStep.effect)
                {
                    case Effect::clean: break;
                    case Effect::repeat:
                        wet = readHistory (ch, sourceStart + pulsePhase);
                        break;
                    case Effect::reverse:
                        wet = readHistory (ch, sourceStart + std::max (0.0, sliceSamples - 1.0 - stepPhase));
                        break;
                    case Effect::shuffle:
                        wet = readHistory (ch, sourceStart + std::min (stepPhase, sliceSamples - 1.0));
                        break;
                    case Effect::gate:
                    {
                        const auto gatePhase = pulsePhase;
                        const auto half = pulseLength * 0.5;
                        const auto ramp = std::min (static_cast<double> (fadeLength), half * 0.25);
                        const auto envelope = std::clamp (std::min (gatePhase, half - gatePhase) / std::max (ramp, 1.0), 0.0, 1.0);
                        wet *= static_cast<float> (envelope);
                        break;
                    }
                    case Effect::halfSpeed:
                        wet = readHistory (ch, sourceStart + stepPhase * 0.5);
                        break;
                }
            }
            // Clean cells remain bit-identical at unity gain. Glitch discontinuities get a short ramp.
            if (fadeRemaining > 0 && (activeStep.effect != Effect::clean || fadeClean))
                wet = fadeFrom[channelIndex] + fade * (wet - fadeFrom[channelIndex]);
            lastWet[channelIndex] = wet;
            channels[ch][frame] = (dry[channelIndex] + smoothedMix * (wet - dry[channelIndex])) * smoothedGain;
            history[channelIndex][static_cast<std::size_t> (written % capacity)] = dry[channelIndex];
        }
        if (numChannels == 1) history[1][static_cast<std::size_t> (written % capacity)] = dry[0];
        ++written;
        if (fadeRemaining > 0) --fadeRemaining;
        if (transport.playing) ppq += ppqIncrement;
    }

    freePpq = ppq;
    expectedPpq = ppq;
    hadPosition = positionValid;
    wasPlaying = transport.playing;
    previousBpm = bpm;
    previousBarBeats = barBeats;
}
}
