#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GlitchEngine.h"

class BeatFlipProcessor final : public juce::AudioProcessor
{
public:
    BeatFlipProcessor();
    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Beat Flip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return displayedBarSeconds.load (std::memory_order_relaxed) * 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void flip(); // Called only by the editor/message thread.
    void keepPattern();
    void loadFactoryPreset (int index);
    static constexpr std::array<int, 5> repeatChoices { 0, 2, 4, 8, 16 };
    static constexpr std::array<int, 5> autoFlipChoices { 0, 1, 2, 4, 8 };
    static constexpr std::array<const char*, 5> effectParameterIds { "stutter", "reverse", "shuffle", "gate", "halfSpeed" };
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<int> displayedStep { -1 };
    std::atomic<std::uint32_t> displayedSeed { 1 };
    std::atomic<std::uint32_t> displayedBaseSeed { 1 };
    std::atomic<std::uint64_t> displayedPattern { 0 };
    std::atomic<bool> displayedCapturing { true };
    std::atomic<bool> displayedHostSync { false };
    std::atomic<bool> displayedPlaying { false };
    std::atomic<double> displayedBpm { 120.0 };
    std::atomic<double> displayedBarSeconds { 2.0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    void setParameterValue (const char* id, float value);
    void publishPattern (float amount = 0.7f) noexcept;
    beatflip::GlitchEngine engine;
    std::atomic<float>* seedParameter = nullptr;
    std::atomic<float>* amountParameter = nullptr;
    std::atomic<float>* mixParameter = nullptr;
    std::atomic<float>* outputParameter = nullptr;
    std::atomic<float>* enabledParameter = nullptr;
    std::atomic<float>* tempoParameter = nullptr;
    std::atomic<float>* swingParameter = nullptr;
    std::atomic<float>* repeatsParameter = nullptr;
    std::atomic<float>* autoFlipParameter = nullptr;
    std::atomic<float>* protectParameter = nullptr;
    std::array<std::atomic<float>*, 5> effectParameters {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatFlipProcessor)
};
