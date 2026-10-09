#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"
#include <cmath>

BeatFlipProcessor::BeatFlipProcessor(bool instrument)
    : AudioProcessor ((instrument ? BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)
                      : BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true))),
      parameters (*this, nullptr, "BeatFlipState", makeParameters(instrument)), instrumentMode(instrument)
{
    seedParameter = parameters.getRawParameterValue ("seed");
    amountParameter = parameters.getRawParameterValue ("amount");
    mixParameter = parameters.getRawParameterValue ("mix");
    outputParameter = parameters.getRawParameterValue ("output");
    enabledParameter = parameters.getRawParameterValue ("enabled");
    tempoParameter = parameters.getRawParameterValue ("tempo");
    swingParameter = parameters.getRawParameterValue ("swing");
    repeatsParameter = parameters.getRawParameterValue ("repeats");
    autoFlipParameter = parameters.getRawParameterValue ("autoFlip");
    protectParameter = parameters.getRawParameterValue ("protect");
    for (std::size_t i = 0; i < effectParameters.size(); ++i)
        effectParameters[i] = parameters.getRawParameterValue (effectParameterIds[i]);
    sourceParameter = parameters.getRawParameterValue ("source");
    drumPlayParameter = parameters.getRawParameterValue ("drumPlay");
    grooveSwingParameter = parameters.getRawParameterValue ("grooveSwing");
    dustParameter = parameters.getRawParameterValue ("dust");
    for (int tr=0; tr<8; ++tr) {
        drumLevels[tr] = parameters.getRawParameterValue ("drumLevel" + juce::String(tr));
        drumMutes[tr] = parameters.getRawParameterValue ("drumMute" + juce::String(tr));
        for (int st=0; st<16; ++st) drumSteps[tr][st] = parameters.getRawParameterValue (drumStepId(tr,st));
    }
    const std::array<const char*,10> synthIds { "synthEnabled","synthBank","synthPosition","synthLevel","synthTune","synthAttack","synthDecay","synthSustain","synthRelease","synthCutoff" };
    for(std::size_t i=0;i<synthIds.size();++i) synthParameters[i]=parameters.getRawParameterValue(synthIds[i]);
    synthSequencePlay=parameters.getRawParameterValue("synthSeqPlay");
    for(int step=0;step<16;++step) {
        const char* fields[] {"Note","Chord","Velocity","Gate"};
        for(int field=0;field<4;++field) synthSteps[static_cast<std::size_t>(step)][static_cast<std::size_t>(field)]=parameters.getRawParameterValue(synthStepId(step,fields[field]));
    }
    publishPattern();
}

juce::AudioProcessorValueTreeState::ParameterLayout BeatFlipProcessor::makeParameters(bool instrument)
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
    // Existing IDs/version hints stay unchanged so Logic automation remains stable.
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "swing", 2 }, "Repeat Swing",
                juce::NormalisableRange<float> { 0.0f, 0.75f, 0.001f }, 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "repeats", 2 }, "Stutter Speed",
                juce::StringArray { "Random", "2 per cell", "4 per cell", "8 per cell", "16 per cell" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "autoFlip", 2 }, "Auto Flip",
                juce::StringArray { "Off", "Every bar", "Every 2 bars", "Every 4 bars", "Every 8 bars" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "protect", 2 }, "Protect Downbeat", true));
    const std::array<const char*, 5> names { "Use Stutter", "Use Reverse", "Use Shuffle", "Use Gate", "Use Half Speed" };
    for (std::size_t i = 0; i < effectParameterIds.size(); ++i)
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { effectParameterIds[i], 2 }, names[i], true));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "source", 3 }, "Audio Source", juce::StringArray { "Audio input", "Drum machine" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "drumPlay", 3 }, "Drum Play", false));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "grooveSwing", 3 }, "Groove Swing", juce::NormalisableRange<float> {0.0f,.6f,.001f}, .12f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "dust", 3 }, "Drum Dust", juce::NormalisableRange<float> {0.0f,1.0f,.001f}, .35f));
    const auto groove = beatflip::drumGroove(0);
    for (int tr=0; tr<8; ++tr) {
        const auto name=juce::String(beatflip::drumNames[tr]);
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "drumLevel" + juce::String(tr), 3 }, name + " Level", juce::NormalisableRange<float> {0.0f,1.0f,.001f}, .8f));
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "drumMute" + juce::String(tr), 3 }, name + " Mute", false));
        for (int st=0;st<16;++st)
            layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID {drumStepId(tr,st),3}, name + " Step " + juce::String(st+1), 0, 2, groove[tr][st]));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"synthEnabled",4}, "Synth On", instrument));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"synthBank",4}, "Wavetable Bank", juce::StringArray{"Classic","Warm","Spectral"},0));
    const auto addSynth = [&layout](const char* id,const char* name,float lo,float hi,float initial,float skew=1) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,4},name,juce::NormalisableRange<float>{lo,hi,.001f,skew},initial));
    };
    addSynth("synthPosition","Wave Position",0,1,.35f);
    addSynth("synthLevel","Synth Level",0,1,.65f);
    addSynth("synthTune","Synth Tune",-24,24,0);
    addSynth("synthAttack","Synth Attack",.001f,4,.01f,.3f);
    addSynth("synthDecay","Synth Decay",.005f,4,.25f,.3f);
    addSynth("synthSustain","Synth Sustain",0,1,.65f);
    addSynth("synthRelease","Synth Release",.01f,8,.5f,.3f);
    addSynth("synthCutoff","Synth Cutoff",40,20000,9000,.25f);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"synthSeqPlay",5},"Play Synth Sequence",false));
    for(int step=0;step<16;++step) {
        const auto name="Synth Step "+juce::String(step+1);
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{synthStepId(step,"Note"),5},name+" Note (-1 Rest)",-1,127,-1));
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{synthStepId(step,"Chord"),5},name+" Chord",juce::StringArray{"Single","Major","Minor","Sus2","Octave"},0));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{synthStepId(step,"Velocity"),5},name+" Velocity",juce::NormalisableRange<float>{.01f,1.0f,.001f},.8f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{synthStepId(step,"Gate"),5},name+" Gate",juce::NormalisableRange<float>{.1f,1.0f,.001f},.65f));
    }
    return layout;
}

void BeatFlipProcessor::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    processorRate=std::isfinite(sampleRate)?juce::jlimit(8000.0,192000.0,sampleRate):48000.0;
    engine.prepare (sampleRate);
    drums.prepare (sampleRate);
    synth.prepare(sampleRate);
    synthSequence.prepare(sampleRate);
    liveSynthAudio.setSize(2,juce::jmax(1,maximumBlockSize));
    previousSynthEnabled=false;
    previousSequencePlaying=false; freeTransportPpq=0;
    guiRead.store(guiWrite.load());
    previousDrumSource = previousDrumPlaying = false;
    setLatencySamples (0);
}

void BeatFlipProcessor::reset()
{
    engine.reset();
    drums.reset();
    synth.reset();
    synthSequence.reset();
    freeTransportPpq=0; previousSequencePlaying=false;
    guiRead.store(guiWrite.load());
}

bool BeatFlipProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && (instrumentMode ? layouts.getMainInputChannelSet().isDisabled() : layouts.getMainInputChannelSet() == output);
}

void BeatFlipProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    beatflip::Settings settings;
    settings.seed = static_cast<std::uint32_t> (seedParameter->load (std::memory_order_relaxed));
    settings.amount = amountParameter->load (std::memory_order_relaxed);
    settings.mix = mixParameter->load (std::memory_order_relaxed);
    settings.outputGain = juce::Decibels::decibelsToGain (outputParameter->load (std::memory_order_relaxed));
    settings.enabled = enabledParameter->load (std::memory_order_relaxed) >= 0.5f;
    settings.swing = swingParameter->load (std::memory_order_relaxed);
    settings.pattern.protectDownbeat = protectParameter->load (std::memory_order_relaxed) >= 0.5f;
    settings.pattern.repeats = repeatChoices[static_cast<std::size_t> (juce::jlimit (0, 4,
        juce::roundToInt (repeatsParameter->load (std::memory_order_relaxed))))];
    settings.autoFlipBars = autoFlipChoices[static_cast<std::size_t> (juce::jlimit (0, 4,
        juce::roundToInt (autoFlipParameter->load (std::memory_order_relaxed))))];
    settings.pattern.effectMask = 0;
    for (std::size_t i = 0; i < effectParameters.size(); ++i)
        if (effectParameters[i]->load (std::memory_order_relaxed) >= 0.5f)
            settings.pattern.effectMask |= static_cast<std::uint8_t> (1u << i);

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

    const bool actualHostSync=transport.hasPosition;
    if(!transport.hasPosition) {
        // One clock for drums, synth pattern and FLIP in both standalone apps.
        transport.ppq=freeTransportPpq; transport.hasPosition=true;
        const double bpm=std::isfinite(transport.bpm)?juce::jlimit(20.0,400.0,transport.bpm):120.0;
        if(transport.playing) freeTransportPpq+=buffer.getNumSamples()*bpm/(60*processorRate);
    }
    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    const bool hostPlaying=transport.playing;
    const bool drumSource = sourceParameter->load (std::memory_order_relaxed) >= .5f;
    const bool drumPlaying = drumPlayParameter->load (std::memory_order_relaxed) >= .5f && transport.playing;
    if (drumSource != previousDrumSource || (drumSource && drumPlaying != previousDrumPlaying)) {
        engine.reset(); drums.reset();
    }
    previousDrumSource = drumSource; previousDrumPlaying = drumPlaying;
    if (drumSource) {
        beatflip::DrumSettings drumSettings;
        drumSettings.playing = drumPlaying;
        drumSettings.swing = grooveSwingParameter->load (std::memory_order_relaxed);
        drumSettings.dust = dustParameter->load (std::memory_order_relaxed);
        for (int tr=0;tr<8;++tr) {
            drumSettings.levels[tr]=drumLevels[tr]->load (std::memory_order_relaxed);
            drumSettings.muted[tr]=drumMutes[tr]->load (std::memory_order_relaxed)>=.5f;
            for (int st=0;st<16;++st) drumSettings.pattern[tr][st]=static_cast<int>(drumSteps[tr][st]->load (std::memory_order_relaxed));
        }
        drums.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), drumSettings, transport, auditionMask.exchange(0), &auditionVelocities);
        displayedDrumStep.store(drums.currentStep(), std::memory_order_relaxed);
        transport.playing = drumPlaying;
        if (!drumPlaying) settings.enabled=false;
    } else { auditionMask.store(0); displayedDrumStep.store(-1); }
    const bool synthEnabled=synthParameters[0]->load()>=.5f;
    if(synthEnabled!=previousSynthEnabled) { synth.reset(); synthSequence.reset(); engine.reset(); }
    previousSynthEnabled=synthEnabled;
    if(panicRequested.exchange(false)) { synth.allSoundOff(); synthSequence.panic(); engine.reset(); guiRead.store(guiWrite.load()); }
    beatflip::SynthSettings synthSettings;
    synthSettings.bank=static_cast<int>(synthParameters[1]->load());
    synthSettings.position=synthParameters[2]->load(); synthSettings.level=synthParameters[3]->load();
    synthSettings.tune=synthParameters[4]->load(); synthSettings.attack=synthParameters[5]->load();
    synthSettings.decay=synthParameters[6]->load(); synthSettings.sustain=synthParameters[7]->load();
    synthSettings.release=synthParameters[8]->load(); synthSettings.cutoff=synthParameters[9]->load();
    synth.setSettings(synthSettings);
    beatflip::SynthSequenceSettings sequenceSettings;
    sequenceSettings.playing=synthEnabled && synthSequencePlay->load()>=.5f;
    if(sequenceSettings.playing!=previousSequencePlaying) engine.reset();
    previousSequencePlaying=sequenceSettings.playing;
    sequenceSettings.swing=grooveSwingParameter->load();
    for(std::size_t step=0;step<16;++step) {
        auto& item=sequenceSettings.pattern[step]; const auto& values=synthSteps[step];
        item.note=static_cast<int>(values[0]->load()); item.chord=static_cast<int>(values[1]->load());
        item.velocity=values[2]->load(); item.gate=values[3]->load();
    }
    auto synthTransport=transport; synthTransport.playing=hostPlaying;
    synthSequence.process(buffer.getArrayOfWritePointers(),buffer.getNumChannels(),buffer.getNumSamples(),sequenceSettings,synthSettings,synthTransport);
    displayedSynthStep.store(synthSequence.currentStep());
    if(drumSource && (synthEnabled || sequenceSettings.playing)) {
        transport.playing=hostPlaying; settings.enabled=enabledParameter->load()>=.5f;
    }
    // Only programmed audio enters FLIP. Live MIDI/keys are monitored afterwards,
    // so even a 100% wet flip can never delay or replace a new note.
    engine.process(buffer.getArrayOfWritePointers(),buffer.getNumChannels(),buffer.getNumSamples(),settings,transport);
    const auto renderLive=[&](int start,int count) {
        while(count>0) {
            const int chunk=juce::jmin(count,liveSynthAudio.getNumSamples());
            liveSynthAudio.clear(); synth.render(liveSynthAudio.getArrayOfWritePointers(),buffer.getNumChannels(),0,chunk);
            for(int ch=0;ch<buffer.getNumChannels();++ch) buffer.addFrom(ch,start,liveSynthAudio,ch,0,chunk,settings.outputGain);
            start+=chunk; count-=chunk;
        }
    };
    auto read=guiRead.load(std::memory_order_relaxed);
    const auto write=guiWrite.load(std::memory_order_acquire);
    while(read!=write) {
        const auto event=guiNotes[read];
        if(synthEnabled) { if(event.down) synth.noteOn(1,event.note,event.velocity); else synth.noteOff(1,event.note); }
        read=(read+1)%static_cast<unsigned>(guiNotes.size());
    }
    guiRead.store(read,std::memory_order_release);
    if(synthEnabled) {
        int rendered=0;
        if(instrumentMode) for(const auto metadata:midi) {
            const int position=juce::jlimit(rendered,buffer.getNumSamples(),metadata.samplePosition);
            renderLive(rendered,position-rendered);
            if(metadata.numBytes<=3) handleSynthMidi(metadata.getMessage());
            rendered=position;
        }
        renderLive(rendered,buffer.getNumSamples()-rendered);
    }
    if(instrumentMode) midi.clear();
    displayedVoices.store(synth.activeVoices());
    displayedStep.store (engine.getCurrentStep(), std::memory_order_relaxed);
    displayedSeed.store (engine.getActiveSeed(), std::memory_order_relaxed);
    displayedBaseSeed.store (engine.getBaseSeed(), std::memory_order_relaxed);
    publishPattern (settings.amount);
    displayedCapturing.store (engine.isCapturing(), std::memory_order_relaxed);
    displayedHostSync.store (actualHostSync, std::memory_order_relaxed);
    displayedPlaying.store (transport.playing, std::memory_order_relaxed);
    displayedBpm.store (transport.bpm, std::memory_order_relaxed);
    const auto safeBpm = std::isfinite (transport.bpm) ? juce::jlimit (20.0, 400.0, transport.bpm) : 120.0;
    displayedBarSeconds.store (juce::jmin (16.0, 60.0 / safeBpm * juce::jlimit (1, 32, transport.numerator)
                                         * 4.0 / juce::jlimit (1, 32, transport.denominator)), std::memory_order_relaxed);
}

void BeatFlipProcessor::editSynthStep(int step,const char* field,float value)
{
    if(step>=0 && step<16) setParameterValue(synthStepId(step,field).toRawUTF8(),value);
}
void BeatFlipProcessor::loadSynthPattern(int preset)
{
    const auto pattern=beatflip::synthPattern(preset);
    for(int step=0;step<16;++step) {
        const auto& item=pattern[static_cast<std::size_t>(step)];
        editSynthStep(step,"Note",static_cast<float>(item.note)); editSynthStep(step,"Chord",static_cast<float>(item.chord));
        editSynthStep(step,"Velocity",item.velocity); editSynthStep(step,"Gate",item.gate);
    }
    if(preset>0) { setParameterValue("synthEnabled",1); setParameterValue("synthSeqPlay",1); }
}

void BeatFlipProcessor::queueSynthNote(int note,bool down,float velocity) noexcept
{
    if(note<0 || note>127) return;
    const auto write=guiWrite.load(std::memory_order_relaxed);
    const auto next=(write+1)%static_cast<unsigned>(guiNotes.size());
    if(next==guiRead.load(std::memory_order_acquire)) { panicRequested.store(true); return; }
    guiNotes[write]={note,down,juce::jlimit(.01f,1.0f,velocity)}; guiWrite.store(next,std::memory_order_release);
}
void BeatFlipProcessor::handleSynthMidi(const juce::MidiMessage& message) noexcept
{
    const int channel=message.getChannel();
    if(message.isNoteOn()) synth.noteOn(channel,message.getNoteNumber(),message.getFloatVelocity());
    else if(message.isNoteOff()) synth.noteOff(channel,message.getNoteNumber());
    else if(message.isPitchWheel()) synth.pitchWheel(channel,message.getPitchWheelValue());
    else if(message.isController()) {
        if(message.getControllerNumber()==64) synth.sustainPedal(channel,message.getControllerValue()>=64);
        else if(message.getControllerNumber()==120) synth.allSoundOff(channel);
        else if(message.getControllerNumber()==123) synth.allNotesOff(channel);
        else if(message.getControllerNumber()==121) { synth.sustainPedal(channel,false); synth.pitchWheel(channel,8192); }
    }
}

void BeatFlipProcessor::loadDrumGroove (int index)
{
    const auto groove=beatflip::drumGroove(index);
    for (int tr=0;tr<8;++tr) for(int st=0;st<16;++st)
        setParameterValue(drumStepId(tr,st).toRawUTF8(), static_cast<float>(groove[tr][st]));
    setParameterValue("source",1);
}
void BeatFlipProcessor::cycleDrumStep (int track,int step)
{
    if(track<0 || track>=8 || step<0 || step>=16) return;
    const int current=static_cast<int>(drumSteps[track][step]->load());
    setParameterValue(drumStepId(track,step).toRawUTF8(), static_cast<float>((current+1)%3));
}
void BeatFlipProcessor::auditionDrum (int track, float velocity)
{
    if(track>=0&&track<8) { auditionVelocities[track].store(juce::jlimit(.01f,1.0f,velocity)); auditionMask.fetch_or(1u<<track); }
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

void BeatFlipProcessor::publishPattern (float amount) noexcept
{
    std::uint64_t packed = 0;
    for (std::size_t i = 0; i < engine.getPattern().size(); ++i)
    {
        const auto& step = engine.getPattern()[i];
        const auto effect = amount < step.threshold ? beatflip::Effect::clean : step.effect;
        packed |= static_cast<std::uint64_t> (effect) << (i * 3u);
    }
    displayedPattern.store (packed, std::memory_order_relaxed);
}

void BeatFlipProcessor::setParameterValue (const char* id, float value)
{
    if (auto* parameter = parameters.getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

void BeatFlipProcessor::keepPattern()
{
    setParameterValue ("seed", static_cast<float> (displayedSeed.load (std::memory_order_relaxed)));
    setParameterValue ("autoFlip", 0.0f);
}

void BeatFlipProcessor::loadFactoryPreset (int index)
{
    const auto& presets = beatflip::factoryPresets();
    if (index < 0 || index >= static_cast<int> (presets.size())) return;
    const auto& settings = presets[static_cast<std::size_t> (index)].settings;
    setParameterValue ("seed", static_cast<float> (settings.seed));
    setParameterValue ("amount", settings.amount);
    setParameterValue ("mix", settings.mix);
    setParameterValue ("swing", settings.swing);
    setParameterValue ("output", 0.0f);
    for (std::size_t i = 0; i < repeatChoices.size(); ++i)
        if (repeatChoices[i] == settings.pattern.repeats) setParameterValue ("repeats", static_cast<float> (i));
    for (std::size_t i = 0; i < autoFlipChoices.size(); ++i)
        if (autoFlipChoices[i] == settings.autoFlipBars) setParameterValue ("autoFlip", static_cast<float> (i));
    setParameterValue ("protect", settings.pattern.protectDownbeat ? 1.0f : 0.0f);
    for (std::size_t i = 0; i < effectParameterIds.size(); ++i)
        setParameterValue (effectParameterIds[i], (settings.pattern.effectMask & (1u << i)) != 0 ? 1.0f : 0.0f);
    setParameterValue ("enabled", 1.0f);
}

juce::AudioProcessorEditor* BeatFlipProcessor::createEditor()
{
    return new BeatFlipEditor (*this);
}

// Pattern banks store source sequences only: sound design, FLIP and automation stay global.
// These methods run on the message/state thread. The audio callback reads APVTS atomics.
void BeatFlipProcessor::capturePerformancePattern()
{
    const auto key=juce::String(selectedGroup.load())+"/"+juce::String(selectedPattern.load());
    auto slot=performanceBanks.getChildWithProperty("key",key);
    if(!slot.isValid()) { slot=juce::ValueTree("Pattern"); slot.setProperty("key",key,nullptr); performanceBanks.appendChild(slot,nullptr); }
    for(int tr=0;tr<8;++tr) for(int st=0;st<16;++st) {
        const auto id=drumStepId(tr,st); slot.setProperty(id,parameters.getRawParameterValue(id)->load(),nullptr);
    }
    for(int st=0;st<16;++st) for(const auto* field:{"Note","Chord","Velocity","Gate"}) {
        const auto id=synthStepId(st,field); slot.setProperty(id,parameters.getRawParameterValue(id)->load(),nullptr);
    }
    for(const auto* id:{"drumPlay","synthSeqPlay"}) slot.setProperty(id,parameters.getRawParameterValue(id)->load(),nullptr);
    performanceBanks.setProperty("group",selectedGroup.load(),nullptr);
    performanceBanks.setProperty("pattern",selectedPattern.load(),nullptr);
}
void BeatFlipProcessor::selectPerformancePattern(int group,int pattern)
{
    if(group<0 || group>3 || pattern<1 || pattern>99) return;
    const juce::ScopedLock lock(bankLock);
    if(group==selectedGroup.load() && pattern==selectedPattern.load()) return;
    capturePerformancePattern(); selectedGroup.store(group); selectedPattern.store(pattern);
    const auto slot=performanceBanks.getChildWithProperty("key",juce::String(group)+"/"+juce::String(pattern));
    for(int tr=0;tr<8;++tr) for(int st=0;st<16;++st) {
        const auto id=drumStepId(tr,st); setParameterValue(id.toRawUTF8(),static_cast<float>(slot.getProperty(id,0)));
    }
    for(int st=0;st<16;++st) for(const auto* field:{"Note","Chord","Velocity","Gate"}) {
        const auto id=synthStepId(st,field);
        const float fallback=juce::String(field)=="Note"?-1.0f:juce::String(field)=="Velocity"?.8f:juce::String(field)=="Gate"?.65f:0.0f;
        setParameterValue(id.toRawUTF8(),static_cast<float>(slot.getProperty(id,fallback)));
    }
    setParameterValue("drumPlay",static_cast<float>(slot.getProperty("drumPlay",1)));
    setParameterValue("synthSeqPlay",static_cast<float>(slot.getProperty("synthSeqPlay",0)));
}

void BeatFlipProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    const juce::ScopedLock lock(bankLock);
    capturePerformancePattern();
    auto state = parameters.copyState();
    state.appendChild(performanceBanks.createCopy(), nullptr);
    if (const auto xml = state.createXml()) copyXmlToBinary (*xml, destination);
}

void BeatFlipProcessor::setStateInformation (const void* data, int size)
{
    if (const auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            // Old projects contain no v0.2 parameters. Fill missing values from
            // defaults rather than keeping settings from a previously loaded preset.
            for (auto* parameter : getParameters())
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                    if (! state.getChildWithProperty ("id", ranged->getParameterID()).isValid())
                    {
                        juce::ValueTree child { "PARAM" };
                        child.setProperty ("id", ranged->getParameterID(), nullptr);
                        child.setProperty ("value", ranged->convertFrom0to1 (ranged->getDefaultValue()), nullptr);
                        state.appendChild (child, nullptr);
                    }
            const juce::ScopedLock lock(bankLock);
            auto banks = state.getChildWithName("PerformanceBanks");
            performanceBanks = banks.isValid() ? banks.createCopy() : juce::ValueTree("PerformanceBanks");
            selectedGroup.store(juce::jlimit(0,3,static_cast<int>(performanceBanks.getProperty("group",0))));
            selectedPattern.store(juce::jlimit(1,99,static_cast<int>(performanceBanks.getProperty("pattern",1))));
            if (banks.isValid()) state.removeChild(banks,nullptr);
            parameters.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BeatFlipProcessor(JucePlugin_IsSynth != 0);
}
