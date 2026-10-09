#include "PluginProcessor.h"
#include "PluginEditor.h"
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
        require (parameter->getVersionHint() == (oldParameterIds.contains (ranged->getParameterID()) ? 1 : ranged->getParameterID().startsWith("synthSeq") ? 5 : ranged->getParameterID().startsWith("synth") ? 4 : ranged->getParameterID().startsWith("drum") || ranged->getParameterID() == "source" || ranged->getParameterID() == "dust" || ranged->getParameterID() == "grooveSwing" ? 3 : 2),
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
    require(value(restored,"synthEnabled")==0,"Legacy effect projects must keep synthesis disabled");
    require(value(restored,"synthSeqPlay")==0 && value(restored,"synthSeq0Note")==-1,"Legacy projects must keep the new sequencer off and empty");
    require(value(restored,"source")==0 && value(restored,"drumPlay")==0,"Legacy projects must use audio input with drums stopped");
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

void drumStateAndProcessing()
{
    BeatFlipProcessor original, restored;
    original.loadDrumGroove(2);
    original.cycleDrumStep(7,15);
    setValue(original,"drumMute2",1); setValue(original,"drumLevel0",.42f);
    setValue(original,"dust",.8f); setValue(original,"grooveSwing",.31f); setValue(original,"drumPlay",1); setValue(original,"enabled",1);
    juce::MemoryBlock state; original.getStateInformation(state);
    restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for(auto* parameter:original.getParameters()) {
        const auto id=dynamic_cast<juce::RangedAudioParameter*>(parameter)->getParameterID();
        require(std::abs(original.parameters.getRawParameterValue(id)->load()-restored.parameters.getRawParameterValue(id)->load())<.00001f,"Every drum control and step must survive project recall");
    }
    restored.prepareToPlay(8192,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
    allocationGuard::start();for(int i=0;i<100;++i) restored.processBlock(audio,midi);const auto allocations=allocationGuard::stop();
    require(allocations==0,"Drum source and FLIP callback must allocate no memory");
    setValue(restored,"drumPlay",0);restored.processBlock(audio,midi);
    require(audio.getMagnitude(0,audio.getNumSamples())==0,"Stopping drums must silence output");
    require(value(restored,"source")==1,"Loading a drum groove must select the drum source");
}

void synthMidiAndState()
{
    BeatFlipProcessor instrument(true);
    require(instrument.acceptsMidi() && instrument.getTotalNumInputChannels()==0,"Instrument accepts MIDI with no input bus");
    require(!BeatFlipProcessor().acceptsMidi(),"Legacy FX keeps its original MIDI contract");
    instrument.prepareToPlay(48000,256);
    setValue(instrument,"synthPosition",.76f); setValue(instrument,"synthBank",2); setValue(instrument,"synthRelease",.01f);
    juce::AudioBuffer<float> audio(2,256); juce::MidiBuffer midi;
    midi.ensureSize(2048); midi.addEvent(juce::MidiMessage::noteOn(1,60,1.0f),64);
    midi.addEvent(juce::MidiMessage::noteOn(1,64,.8f),96);
    midi.addEvent(juce::MidiMessage::noteOn(1,67,.7f),128);
    const juce::uint8 sysex[32] {}; midi.addEvent(juce::MidiMessage::createSysExMessage(sysex,32),180);
    allocationGuard::start(); instrument.processBlock(audio,midi); const auto allocations=allocationGuard::stop();
    require(allocations==0,"MIDI synthesis including ignored SysEx must allocate no audio-thread memory");
    require(audio.getMagnitude(0,64)==0 && audio.getMagnitude(64,192)>0,"MIDI note timing must be sample accurate");
    require(instrument.displayedVoices.load()==3 && midi.isEmpty(),"Instrument plays a chord without MIDI output");
    juce::MemoryBlock state; instrument.getStateInformation(state);
    BeatFlipProcessor restored(true); restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for(const auto* id:{"synthEnabled","synthBank","synthPosition","synthLevel","synthTune","synthAttack","synthDecay","synthSustain","synthRelease","synthCutoff"})
        require(value(instrument,id)==value(restored,id),"Every synth control survives project recall");
    instrument.panicSynth(); instrument.processBlock(audio,midi);
    require(instrument.displayedVoices.load()==0 && audio.getMagnitude(0,256)==0,"Panic must silence instrument notes");
    setValue(instrument,"source",1); instrument.queueSynthNote(48,true); instrument.processBlock(audio,midi);
    require(audio.getMagnitude(0,256)>0,"Keyboard layer plays even when the backing drum sequencer is stopped");
    setValue(instrument,"synthEnabled",0); instrument.processBlock(audio,midi);
    require(instrument.displayedVoices.load()==0,"Turning synth off releases its voices");
}

void performancePatternRecall()
{
    for(bool instrument:{false,true}) {
        BeatFlipProcessor processor(instrument),restored(instrument);
        processor.loadDrumGroove(2); processor.loadSynthPattern(2); processor.editSynthStep(3,"Note",65);
        const auto original=processor.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(0,3))->load();
        processor.selectPerformancePattern(1,99);
        require(processor.parameters.getRawParameterValue(BeatFlipProcessor::synthStepId(3,"Note"))->load()==-1,"New bank patterns must be blank");
        processor.cycleDrumStep(2,7); processor.editSynthStep(3,"Note",72);
        processor.selectPerformancePattern(0,1);
        require(processor.parameters.getRawParameterValue(BeatFlipProcessor::synthStepId(3,"Note"))->load()==65,"Switching group must preserve synth edits");
        require(processor.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(0,3))->load()==original,"Switching group must preserve drum edits");
        juce::MemoryBlock state; processor.getStateInformation(state); restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        restored.selectPerformancePattern(1,99);
        require(restored.parameters.getRawParameterValue(BeatFlipProcessor::synthStepId(3,"Note"))->load()==72,"Inactive synth bank patterns must survive project recall");
        require(restored.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(2,7))->load()==1,"Inactive drum bank patterns must survive project recall");
        restored.selectPerformancePattern(4,100);
        require(restored.performanceGroup()==1 && restored.performancePattern()==99,"Invalid bank selections must have no effect");
        juce::MemoryBlock second;restored.getStateInformation(second);processor.setStateInformation(second.getData(),static_cast<int>(second.getSize()));
        require(processor.performanceGroup()==1 && processor.performancePattern()==99,"Selected group and pattern must survive recall");
    }
}
void editorSnapshot()
{
    BeatFlipProcessor processor(JucePlugin_IsSynth != 0);
    processor.loadFactoryPreset (4);
    processor.loadSynthPattern(2);
    processor.prepareToPlay (8192.0, 256);
    juce::AudioBuffer<float> audio (2, 256);
    audio.clear();
    juce::MidiBuffer midi;
    for (int i = 0; i < 130; ++i) processor.processBlock (audio, midi);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    require (editor->getWidth() == 1100 && editor->getHeight() == 880, "The performance editor must fit the focused controls");
    for (auto* child : editor->getChildren())
        require (editor->getLocalBounds().contains (child->getBounds()), "Editor controls must stay within its bounds");
    // Every focused workspace must render inside the same compact enclosure.
    for(const auto* view:{"SOUND","PATTERN","FX / FLIP"}) {
        for(auto* child:editor->getChildren()) if(auto* button=dynamic_cast<juce::TextButton*>(child))
            if(button->getButtonText()==view && button->onClick) button->onClick();
        for(auto* child:editor->getChildren()) if(child->isVisible())
            require(editor->getLocalBounds().contains(child->getBounds()),"Every workspace must fit the enclosure");
        const auto workspaceImage=editor->createComponentSnapshot(editor->getLocalBounds());
        const auto filename=juce::String(view)=="SOUND"?"Dustbox-Sound.png":juce::String(view)=="PATTERN"?"Dustbox-Pattern.png":"Dustbox-FX.png";
        auto workspaceStream=juce::File::getCurrentWorkingDirectory().getChildFile(filename).createOutputStream();
        require(workspaceStream!=nullptr && juce::PNGImageFormat().writeImageToStream(workspaceImage,*workspaceStream),"Focused workspace must render");
    }
    for(auto* child:editor->getChildren()) if(auto* button=dynamic_cast<juce::TextButton*>(child))
        if(button->getButtonText()=="DRUMS" && button->onClick) button->onClick();
    const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
    auto stream = juce::File::getCurrentWorkingDirectory().getChildFile ("Beat-Flip-Editor.png").createOutputStream();
    require (stream != nullptr && juce::PNGImageFormat().writeImageToStream (image, *stream), "Editor preview must render successfully");
}
void sequenceRecallAndLiveMonitoring()
{
    for(bool instrument:{false,true}) {
        BeatFlipProcessor processor(instrument),restored(instrument);
        processor.loadSynthPattern(1); processor.editSynthStep(6,"Chord",2); processor.editSynthStep(6,"Gate",.31f);
        juce::MemoryBlock state; processor.getStateInformation(state); restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        for(auto* parameter:processor.getParameters()) {
            const auto id=dynamic_cast<juce::RangedAudioParameter*>(parameter)->getParameterID();
            require(processor.parameters.getRawParameterValue(id)->load()==restored.parameters.getRawParameterValue(id)->load(),"Every synth step must survive project recall in both products");
        }
        restored.prepareToPlay(8000,256); juce::AudioBuffer<float> audio(2,256); juce::MidiBuffer midi;
        allocationGuard::start(); for(int i=0;i<100;++i) { audio.clear(); restored.processBlock(audio,midi); } const auto allocations=allocationGuard::stop();
        require(allocations==0 && restored.displayedSynthStep.load()>=0,"Both native versions must sequence with no realtime allocation");
        setValue(restored,"synthSeqPlay",0); setValue(restored,"enabled",1); setValue(restored,"mix",1); setValue(restored,"amount",1);
        setValue(restored,"source",1); setValue(restored,"drumPlay",0); setValue(restored,"synthRelease",.01f);
        // Warm the FLIP history with silence. A live note must not depend on that history.
        for(int i=0;i<100;++i) { audio.clear(); restored.processBlock(audio,midi); }
        for(int note:{60,64,67}) {
            restored.queueSynthNote(note,true); audio.clear(); restored.processBlock(audio,midi);
            require(audio.getMagnitude(0,256)>.01f,"Live keys must sound mid-bar even with FLIP 100% wet in either version");
            restored.queueSynthNote(note,false); for(int i=0;i<3;++i) { audio.clear(); restored.processBlock(audio,midi); }
        }
        // Oversized host blocks still use preallocated scratch storage.
        juce::AudioBuffer<float> large(2,1024); restored.queueSynthNote(72,true); large.clear();
        allocationGuard::start(); restored.processBlock(large,midi); const auto largeAllocations=allocationGuard::stop();
        require(largeAllocations==0 && large.getMagnitude(0,1024)>.01f,"Live synthesis must handle larger blocks without reallocating");
        restored.panicSynth(); large.clear(); restored.processBlock(large,midi);
        require(large.getMagnitude(0,1024)==0,"Panic must clear live notes and captured sequence audio");
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    const std::pair<const char*, void (*)()> tests[] {
        { "Preset state recall and v0.1 project migration", stateRecallAndLegacyDefaults },
        { "KEEP and allocation-free plugin processing", keepAndRealTimeProcessing },
        { "Drum grid recall and realtime processing", drumStateAndProcessing },
        { "Polyphonic MIDI timing, state and panic", synthMidiAndState },
        { "Synth sequence recall and immediate live monitoring in both products", sequenceRecallAndLiveMonitoring },
        { "A-D pattern banks, inactive pattern recall and selection bounds", performancePatternRecall },
        { "Focused performance editor layout and PNG rendering", editorSnapshot }
    };
    int failures = 0;
    for (const auto& test : tests)
    {
        try { test.second(); std::cout << "PASS  " << test.first << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL  " << test.first << ": " << error.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
