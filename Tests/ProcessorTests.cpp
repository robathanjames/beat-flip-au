#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "AllocationGuard.h"
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void require (bool condition, const char* message)
{
    if (! condition) throw std::runtime_error (message);
}

void setValue (BeatFlipProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.parameters.getParameter (id);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

float value (BeatFlipProcessor& processor, const char* id)
{
    return processor.parameters.getRawParameterValue (id)->load();
}

void stateRecallAndLegacyDefaults()
{
    BeatFlipProcessor original, restored;
    const juce::StringArray oldParameterIds { "enabled", "seed", "amount", "mix", "output", "tempo" };
    for (auto* parameter : original.getParameters())
    {
        const auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter);
        require (parameter->getVersionHint() == (oldParameterIds.contains (ranged->getParameterID()) ? 1 : 2),
                 "Existing AU parameter ordering must retain its original version hints");
    }
    setValue (original, "tempo", 137.5f);
    for (int preset = 0; preset < static_cast<int> (beatflip::factoryPresets().size()); ++preset)
    {
        original.loadFactoryPreset (preset);
        require (value (original, "tempo") == 137.5f, "Preset loading must preserve Free Tempo");
        juce::MemoryBlock state;
        original.getStateInformation (state);
        restored.loadFactoryPreset (5);
        restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        for (auto* parameter : original.getParameters())
        {
            const auto id = dynamic_cast<juce::RangedAudioParameter*> (parameter)->getParameterID();
            require (std::abs (original.parameters.getRawParameterValue (id)->load()
                            - restored.parameters.getRawParameterValue (id)->load()) < 0.00001f,
                     "Every preset control must survive project state recall");
        }
    }

    setValue (original, "seed", 8123.0f);
    setValue (original, "mix", 0.67f);
    setValue (original, "output", -3.0f);
    auto legacy = original.parameters.copyState();
    const juce::StringArray oldIds { "enabled", "seed", "amount", "mix", "output", "tempo" };
    for (int i = legacy.getNumChildren(); --i >= 0;)
        if (! oldIds.contains (legacy.getChild (i).getProperty ("id").toString())) legacy.removeChild (i, nullptr);
    juce::MemoryBlock state;
    juce::AudioProcessor::copyXmlToBinary (*legacy.createXml(), state);
    restored.loadFactoryPreset (5);
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    require (value (restored, "seed") == 8123.0f && std::abs (value (restored, "mix") - 0.67f) < 0.00001f
             && value (restored, "output") == -3.0f, "Legacy state must retain existing controls");
    require (value (restored, "swing") == 0.0f && value (restored, "repeats") == 0.0f
             && value (restored, "autoFlip") == 0.0f && value (restored, "protect") == 1.0f,
             "Legacy projects must restore the original pattern defaults");
    for (const auto* id : BeatFlipProcessor::effectParameterIds)
        require (value (restored, id) == 1.0f, "Legacy projects must enable the full original effect palette");
}

void keepAndRealTimeProcessing()
{
    BeatFlipProcessor processor;
    processor.prepareToPlay (8192.0, 256);
    processor.loadFactoryPreset (5);
    juce::AudioBuffer<float> audio (2, 256);
    juce::MidiBuffer midi;
    for (int i = 0; i < 140; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int frame = 0; frame < audio.getNumSamples(); ++frame)
                audio.setSample (ch, frame, 0.2f * std::sin (static_cast<float> (i * 256 + frame) * 0.04f));
        processor.processBlock (audio, midi);
    }
    const auto kept = processor.displayedSeed.load();
    require (kept != static_cast<std::uint32_t> (value (processor, "seed")), "Auto Flip must generate a variation through the plugin wrapper");
    processor.keepPattern();
    require (value (processor, "seed") == static_cast<float> (kept) && value (processor, "autoFlip") == 0.0f,
             "KEEP must adopt the audible seed and disable automatic changes");
    allocationGuard::start();
    for (int i = 0; i < 300; ++i) processor.processBlock (audio, midi);
    const auto allocations = allocationGuard::stop();
    require (allocations == 0, "The complete plugin audio callback must allocate no memory");
    require (processor.displayedSeed.load() == kept, "A kept pattern must remain active across later bars");
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    BeatFlipProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    require (value (restored, "seed") == static_cast<float> (kept) && value (restored, "autoFlip") == 0.0f,
             "A kept pattern must survive a project reload");
}

void editorSnapshot()
{
    BeatFlipProcessor processor;
    processor.loadFactoryPreset (4);
    processor.prepareToPlay (8192.0, 256);
    juce::AudioBuffer<float> audio (2, 256);
    audio.clear();
    juce::MidiBuffer midi;
    for (int i = 0; i < 130; ++i) processor.processBlock (audio, midi);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    require (editor->getWidth() == 860 && editor->getHeight() == 580, "The editor must have room for the expanded controls");
    for (auto* child : editor->getChildren())
        require (editor->getLocalBounds().contains (child->getBounds()), "Editor controls must stay within its bounds");
    const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
    auto stream = juce::File::getCurrentWorkingDirectory().getChildFile ("Beat-Flip-Editor.png").createOutputStream();
    require (stream != nullptr && juce::PNGImageFormat().writeImageToStream (image, *stream), "Editor preview must render successfully");
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    const std::pair<const char*, void (*)()> tests[] {
        { "Preset state recall and v0.1 project migration", stateRecallAndLegacyDefaults },
        { "KEEP and allocation-free plugin processing", keepAndRealTimeProcessing },
        { "Expanded editor layout and PNG rendering", editorSnapshot }
    };
    int failures = 0;
    for (const auto& test : tests)
    {
        try { test.second(); std::cout << "PASS  " << test.first << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL  " << test.first << ": " << error.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
