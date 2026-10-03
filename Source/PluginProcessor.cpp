#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

BeatFlipProcessor::BeatFlipProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "BeatFlipState", makeParameters())
{
    seedParameter = parameters.getRawParameterValue ("seed");
    amountParameter = parameters.getRawParameterValue ("amount");
    mixParameter = parameters.getRawParameterValue ("mix");
    outputParameter = parameters.getRawParameterValue ("output");
    enabledParameter = parameters.getRawParameterValue ("enabled");
    tempoParameter = parameters.getRawParameterValue ("tempo");
}

juce::AudioProcessorValueTreeState::ParameterLayout BeatFlipProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "enabled", 1 }, "Glitch Enabled", false));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "seed", 1 }, "Pattern Seed", 1, 1048575, 1));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "amount", 1 }, "Amount",
                juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.7f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
                juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.85f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output",
                juce::NormalisableRange<float> { -24.0f, 6.0f, 0.1f }, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tempo", 1 }, "Free Tempo",
                juce::NormalisableRange<float> { 20.0f, 400.0f, 0.1f }, 120.0f));
    return layout;
}

void BeatFlipProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    setLatencySamples (0);
}

void BeatFlipProcessor::reset()
{
    engine.reset();
}

bool BeatFlipProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && layouts.getMainInputChannelSet() == output;
}

void BeatFlipProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    beatflip::Settings settings;
    settings.seed = static_cast<std::uint32_t> (seedParameter->load (std::memory_order_relaxed));
    settings.amount = amountParameter->load (std::memory_order_relaxed);
    settings.mix = mixParameter->load (std::memory_order_relaxed);
    settings.outputGain = juce::Decibels::decibelsToGain (outputParameter->load (std::memory_order_relaxed));
    settings.enabled = enabledParameter->load (std::memory_order_relaxed) >= 0.5f;

    beatflip::Transport transport;
    transport.bpm = tempoParameter->load (std::memory_order_relaxed);
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            transport.playing = position->getIsPlaying();
            if (const auto bpm = position->getBpm()) transport.bpm = *bpm;
            if (const auto ppq = position->getPpqPosition())
            {
                transport.ppq = *ppq;
                transport.hasPosition = true;
            }
            if (const auto start = position->getPpqPositionOfLastBarStart())
            {
                transport.barStartPpq = *start;
                transport.hasBarStart = true;
            }
            if (const auto signature = position->getTimeSignature())
            {
                transport.numerator = signature->numerator;
                transport.denominator = signature->denominator;
            }
            transport.looping = position->getIsLooping();
            if (const auto loop = position->getLoopPoints())
            {
                transport.loopStartPpq = loop->ppqStart;
                transport.loopEndPpq = loop->ppqEnd;
            }
        }
    }

    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    engine.process (buffer.getArrayOfWritePointers(), getTotalNumInputChannels(), buffer.getNumSamples(), settings, transport);
    displayedStep.store (engine.getCurrentStep(), std::memory_order_relaxed);
    displayedSeed.store (engine.getActiveSeed(), std::memory_order_relaxed);
    displayedCapturing.store (engine.isCapturing(), std::memory_order_relaxed);
    displayedHostSync.store (transport.hasPosition, std::memory_order_relaxed);
    displayedPlaying.store (transport.playing, std::memory_order_relaxed);
    displayedBpm.store (transport.bpm, std::memory_order_relaxed);
    const auto safeBpm = std::isfinite (transport.bpm) ? juce::jlimit (20.0, 400.0, transport.bpm) : 120.0;
    displayedBarSeconds.store (juce::jmin (16.0, 60.0 / safeBpm * juce::jlimit (1, 32, transport.numerator)
                                         * 4.0 / juce::jlimit (1, 32, transport.denominator)), std::memory_order_relaxed);
}

void BeatFlipProcessor::flip()
{
    auto* seed = parameters.getParameter ("seed");
    const auto previous = static_cast<int> (seedParameter->load (std::memory_order_relaxed));
    auto next = juce::Random::getSystemRandom().nextInt ({ 1, 1048576 });
    if (next == previous) next = previous % 1048575 + 1;
    seed->beginChangeGesture();
    seed->setValueNotifyingHost (seed->convertTo0to1 (static_cast<float> (next)));
    seed->endChangeGesture();
    auto* enabled = parameters.getParameter ("enabled");
    enabled->beginChangeGesture();
    enabled->setValueNotifyingHost (1.0f);
    enabled->endChangeGesture();
}

juce::AudioProcessorEditor* BeatFlipProcessor::createEditor()
{
    return new BeatFlipEditor (*this);
}

void BeatFlipProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    const auto state = parameters.copyState();
    if (const auto xml = state.createXml()) copyXmlToBinary (*xml, destination);
}

void BeatFlipProcessor::setStateInformation (const void* data, int size)
{
    if (const auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BeatFlipProcessor();
}
