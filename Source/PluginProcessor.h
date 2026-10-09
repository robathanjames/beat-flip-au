#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GlitchEngine.h"
#include "DrumMachine.h"
#include "WavetableSynth.h"
#include "SynthSequencer.h"

class BeatFlipProcessor final : public juce::AudioProcessor
{
public:
    explicit BeatFlipProcessor(bool instrument = false);
    bool isInstrument() const noexcept { return instrumentMode; }
    void queueSynthNote(int note, bool down, float velocity = .8f) noexcept;
    void selectPerformancePattern(int group, int pattern);
    int performanceGroup() const noexcept { return selectedGroup.load(); }
    int performancePattern() const noexcept { return selectedPattern.load(); }
    void panicSynth() noexcept { panicRequested.store(true); }
    std::atomic<int> displayedVoices { 0 };
    std::atomic<int> displayedSynthStep { -1 };
    static juce::String synthStepId(int step,const char* field) { return "synthSeq"+juce::String(step)+field; }
    void editSynthStep(int step,const char* field,float value);
    void loadSynthPattern(int preset);
    void prepareToPlay (double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return instrumentMode ? "Dustbox Synth" : "Dustbox"; }
    bool acceptsMidi() const override { return instrumentMode; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return juce::jmax(displayedBarSeconds.load(std::memory_order_relaxed)*2.0, synthParameters[0]->load()>=.5f ? static_cast<double>(synthParameters[8]->load())*1.5 : 0.0); }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadDrumGroove (int index);
    void cycleDrumStep (int track, int step);
    void auditionDrum (int track, float velocity = 1.0f);
    static juce::String drumStepId (int track, int step) { return "drum" + juce::String(track) + "step" + juce::String(step); }
    std::atomic<int> displayedDrumStep { -1 };
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
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters(bool instrument);
    void setParameterValue (const char* id, float value);
    void publishPattern (float amount = 0.7f) noexcept;
    beatflip::GlitchEngine engine;
    beatflip::DrumMachine drums;
    beatflip::WavetableSynth synth;
    beatflip::SynthSequencer synthSequence;
    juce::AudioBuffer<float> liveSynthAudio;
    std::atomic<float>* synthSequencePlay = nullptr;
    std::array<std::array<std::atomic<float>*,4>,16> synthSteps {};
    const bool instrumentMode;
    bool previousSynthEnabled = false;
    bool previousSequencePlaying = false;
    double freeTransportPpq = 0;
    double processorRate = 48000;
    struct GuiNote { int note = 60; bool down = false; float velocity = .8f; };
    std::array<GuiNote,256> guiNotes {};
    std::atomic<unsigned> guiWrite { 0 }, guiRead { 0 };
    std::atomic<bool> panicRequested { false };
    std::array<std::atomic<float>*,10> synthParameters {};
    void handleSynthMidi(const juce::MidiMessage&) noexcept;
    std::array<std::array<std::atomic<float>*,16>,8> drumSteps {};
    std::array<std::atomic<float>*,8> drumLevels {}, drumMutes {};
    std::atomic<float>* sourceParameter = nullptr;
    std::atomic<float>* drumPlayParameter = nullptr;
    std::atomic<float>* grooveSwingParameter = nullptr;
    std::atomic<float>* dustParameter = nullptr;
    std::atomic<unsigned> auditionMask { 0 };
    std::array<std::atomic<float>,8> auditionVelocities {};
    juce::CriticalSection bankLock; // Never acquired by processBlock.
    juce::ValueTree performanceBanks { "PerformanceBanks" };
    std::atomic<int> selectedGroup { 0 }, selectedPattern { 1 };
    void capturePerformancePattern();
    bool previousDrumSource = false, previousDrumPlaying = false;
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
