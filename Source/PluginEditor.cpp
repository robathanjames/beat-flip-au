#include "DrumMidiExport.h"
#include "PluginEditor.h"
#include "FactoryPresets.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xffdedcd0 };
const juce::Colour panel { 0xff242a29 };
const juce::Colour muted { 0xff62665e };
const juce::Colour coral { 0xffdb703c };
const juce::Colour mint { 0xff282b28 };
const juce::Colour amber { 0xffedb577 };
const juce::Colour blue { 0xff4c7786 };

juce::Colour effectColour (beatflip::Effect effect)
{
    return effect == beatflip::Effect::clean ? juce::Colour { 0xffaab2a1 } : amber;
}
}

BeatFlipLookAndFeel::BeatFlipLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, amber);
    setColour (juce::Slider::textBoxBackgroundColourId, panel);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId, juce::Colour(0xffeceae1));
    setColour (juce::TextButton::textColourOffId, mint);
    setColour (juce::ToggleButton::textColourId, mint);
    setColour (juce::ToggleButton::tickColourId, coral);
    setColour (juce::Slider::thumbColourId, mint);
    setColour (juce::Slider::trackColourId, panel);
    setColour (juce::Slider::backgroundColourId, muted);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, amber);
    setColour (juce::ComboBox::outlineColourId, muted.withAlpha (0.3f));
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, amber);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, mint.darker (0.6f));
}

void BeatFlipLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float position, float start, float end, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                               static_cast<float> (w), static_cast<float> (h)).reduced (4.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    for (int i = 0; i < 11; ++i)
    {
        const auto tick = start + (end - start) * i / 10.0f;
        g.setColour (muted);
        g.drawLine (centre.x + std::sin(tick) * radius, centre.y - std::cos(tick) * radius,
                    centre.x + std::sin(tick) * (radius - 3), centre.y - std::cos(tick) * (radius - 3), 1.0f);
    }
    const auto body = radius * .78f;
    g.setColour (juce::Colours::black.withAlpha (.55f));
    g.fillEllipse (centre.x - body + 2, centre.y - body + 4, body * 2, body * 2);
    juce::Path grip;
    for (int i = 0; i < 48; ++i) {
        const auto a = juce::MathConstants<float>::twoPi * i / 48.0f;
        const auto r = body * (i % 4 == 0 ? .91f : 1.0f);
        const auto px = centre.x + std::sin(a) * r, py = centre.y - std::cos(a) * r;
        if (i == 0) grip.startNewSubPath(px, py); else grip.lineTo(px, py);
    }
    grip.closeSubPath();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff383a3b),centre.x-body,centre.y-body,juce::Colour(0xff080909),centre.x+body,centre.y+body,false));
    g.fillPath(grip);
    const auto cap = body * .54f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe4e6df),centre.x-cap,centre.y-cap,juce::Colour(0xff737974),centre.x+cap,centre.y+cap,false));
    g.fillEllipse(centre.x-cap,centre.y-cap,cap*2,cap*2);
    g.setColour(juce::Colour(0xffb7bbb2));
    g.drawEllipse(centre.x-cap,centre.y-cap,cap*2,cap*2,1);
    const auto angle = start + position * (end - start);
    g.setColour (mint);
    g.drawLine (centre.x + std::sin (angle) * (radius * .38f), centre.y - std::cos (angle) * (radius * .38f),
                centre.x + std::sin (angle) * (radius * .67f), centre.y - std::cos (angle) * (radius * .67f), 3.0f);
}

void BeatFlipLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& colour, bool over, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto face = down ? colour.darker (.22f) : over ? colour.brighter (.08f) : colour;
    g.setColour (muted.withAlpha (.35f));
    g.fillRoundedRectangle (bounds.translated (0, 2), 3.0f);
    g.setGradientFill(juce::ColourGradient(face.brighter(.16f),bounds.getX(),bounds.getY(),face.darker(.28f),bounds.getRight(),bounds.getBottom(),false));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (muted.withAlpha (.5f));
    g.drawRoundedRectangle (bounds.reduced (.5f), 3.0f, 1.0f);
    if (button.getProperties().contains ("stepValue"))
    {
        const int value = static_cast<int> (button.getProperties()["stepValue"]);
        const bool active = static_cast<bool> (button.getProperties()["activeStep"]);
        g.setColour (value == 2 ? panel : value == 1 ? coral : muted.withAlpha (.35f));
        g.fillRoundedRectangle (bounds.getCentreX() - 5, bounds.getY() + 6, 10, 3, 1.0f);
        if (value == 2) g.fillRoundedRectangle (bounds.getCentreX() - 5, bounds.getY() + 12, 10, 3, 1.0f);
        if (active) { g.setColour (mint); g.drawRoundedRectangle (bounds.reduced (1), 3, 2); }
    }

}

void BeatFlipLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    if (button.getProperties().contains ("stepValue")) return;
    if(button.getProperties().contains("performancePad")) {
        g.setColour(juce::Colour(0xff292d29)); g.setFont(juce::FontOptions{22.0f,juce::Font::bold});
        g.drawText(juce::String(static_cast<int>(button.getProperties()["performancePad"])+1).paddedLeft('0',2),12,6,44,26,juce::Justification::centredLeft);
        g.setFont(juce::FontOptions{11.0f,juce::Font::bold}); g.drawText(button.getButtonText(),12,34,button.getWidth()-24,18,juce::Justification::centredLeft);
        g.setColour(static_cast<bool>(button.getProperties()["activeStep"])?coral:juce::Colour(0xff96998e)); g.fillEllipse(static_cast<float>(button.getWidth()-23),12.0f,6.0f,6.0f); return;
    }
    if(button.getProperties().contains("synthStep")) {
        const auto bounds=button.getLocalBounds();
        const auto selected=static_cast<bool>(button.getProperties()["selectedStep"]);
        const auto active=static_cast<bool>(button.getProperties()["activeStep"]);
        g.setColour(mint); g.setFont(juce::FontOptions{12.0f,juce::Font::bold});
        g.drawText(juce::String(static_cast<int>(button.getProperties()["synthStep"])+1),bounds.withHeight(17),juce::Justification::centred);
        g.drawText(button.getButtonText(),bounds.withTrimmedTop(17).withTrimmedBottom(4),juce::Justification::centred);
        if(selected) { g.setColour(coral); g.drawRoundedRectangle(bounds.toFloat().reduced(1),3,2); }
        if(active) { g.setColour(amber); g.fillRect(4,bounds.getHeight()-5,bounds.getWidth()-8,3); }
        return;
    }
    const auto face = button.findColour (juce::TextButton::buttonColourId);
    g.setColour (face.getBrightness() > .55f ? juce::Colour { 0xff202622 } : mint);
    g.setFont (juce::FontOptions { button.getButtonText() == "FLIP" ? 34.0f : 13.0f, juce::Font::bold });
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

BeatFlipEditor::BeatFlipEditor (BeatFlipProcessor& owner) : AudioProcessorEditor (owner), processor (owner)
{
    setLookAndFeel (&look);
    setSize (1100, 880);
    addAndMakeVisible (flipButton);
    flipButton.setColour (juce::TextButton::buttonColourId, coral);
    addAndMakeVisible (keepButton);
    addAndMakeVisible (enabledButton);
    addAndMakeVisible (protectButton);
    flipButton.setTooltip ("Generate a new pattern. It starts on the next grid step; Glitch switches on automatically.");
    flipButton.onClick = [this] { processor.flip(); };
    enabledButton.setTooltip ("Turn the glitch pattern on or off. The input keeps being captured.");
    keepButton.setColour (juce::TextButton::buttonColourId, juce::Colour(0xffeceae1));
    keepButton.setTooltip ("Keep the current variation and turn Auto Flip off. The seed is saved with your project.");
    keepButton.onClick = [this] { processor.keepPattern(); };
    protectButton.setTooltip ("Leave the first cell live to anchor the groove. Turn off to allow glitches on the downbeat.");

    const auto configure = [this] (juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 94, 22);
        addAndMakeVisible (slider);
        slider.setColour (juce::Slider::textBoxTextColourId, amber);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, panel);
        slider.setColour (juce::Slider::textBoxOutlineColourId, muted.withAlpha (.4f));
        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, muted);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions { 13.0f, juce::Font::bold });
        addAndMakeVisible (label);
    };
    configure (amount, amountLabel, "AMOUNT");
    configure (mix, mixLabel, "MIX");
    configure (output, outputLabel, "OUTPUT");
    configure (tempo, tempoLabel, "FREE TEMPO");
    configure (swing, swingLabel, "REPEAT SWING");
    amount.setTooltip ("How many steps get glitched. At zero the beat stays unchanged.");
    mix.setTooltip ("Blend the live drum track with the glitch pattern.");
    output.setTooltip ("Output level after the dry/wet blend.");
    tempo.setTooltip ("Tempo used when the host supplies no tempo. Logic's tempo takes precedence.");
    swing.setTooltip ("Delay alternating stutter and gate pulses. The live input and 16-cell grid keep their timing.");
    for (auto* slider : { &amount, &mix, &swing })
    {
        slider->textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; };
        slider->valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() * 0.01; };
    }
    output.setTextValueSuffix (" dB");
    tempo.setTextValueSuffix (" BPM");
    amountAttachment = std::make_unique<SliderAttachment> (processor.parameters, "amount", amount);
    mixAttachment = std::make_unique<SliderAttachment> (processor.parameters, "mix", mix);
    outputAttachment = std::make_unique<SliderAttachment> (processor.parameters, "output", output);
    tempoAttachment = std::make_unique<SliderAttachment> (processor.parameters, "tempo", tempo);
    swingAttachment = std::make_unique<SliderAttachment> (processor.parameters, "swing", swing);
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "enabled", enabledButton);
    protectAttachment = std::make_unique<ButtonAttachment> (processor.parameters, "protect", protectButton);

    const auto configureChoice = [this] (juce::ComboBox& box, juce::Label& label, const char* title)
    {
        addAndMakeVisible (box);
        label.setText (title, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, muted);
        label.setFont (juce::FontOptions { 11.0f, juce::Font::bold });
        addAndMakeVisible (label);
    };
    configureChoice (repeats, repeatsLabel, "STUTTER / GATE SPEED");
    configureChoice (autoFlip, autoFlipLabel, "AUTO FLIP");
    repeats.addItemList ({ "Random", "2 per cell", "4 per cell", "8 per cell", "16 per cell" }, 1);
    autoFlip.addItemList ({ "Off", "Every bar", "Every 2 bars", "Every 4 bars", "Every 8 bars" }, 1);
    repeats.setTooltip ("Choose repeats per grid cell. Random uses 2, 4 or 8. Also controls the rhythmic gate.");
    autoFlip.setTooltip ("Generate variations at bar boundaries. The sequence restarts on playback restart or a seek. KEEP saves the current one.");
    repeatsAttachment = std::make_unique<ComboAttachment> (processor.parameters, "repeats", repeats);
    autoFlipAttachment = std::make_unique<ComboAttachment> (processor.parameters, "autoFlip", autoFlip);
    const std::array<const char*, 5> effectNames { "STUTTER", "REVERSE", "SHUFFLE", "GATE", "HALF SPEED" };
    for (std::size_t i = 0; i < effectButtons.size(); ++i)
    {
        auto& button = effectButtons[i];
        button.setButtonText (effectNames[i]);
        button.setTooltip ("Include this effect in generated patterns. Changes apply at the next grid cell.");
        addAndMakeVisible (button);
        effectAttachments[i] = std::make_unique<ButtonAttachment> (processor.parameters, BeatFlipProcessor::effectParameterIds[i], button);
    }
    addAndMakeVisible (presets);
    presets.setTextWhenNothingSelected ("Load a factory preset...");
    const auto& factory = beatflip::factoryPresets();
    for (std::size_t i = 0; i < factory.size(); ++i) presets.addItem (factory[i].name, static_cast<int> (i + 1));
    presets.setTooltip ("Load a musical starting point. Presets keep your Free Tempo setting.");
    presets.onChange = [this]
    {
        if (presets.getSelectedId() > 0) processor.loadFactoryPreset (presets.getSelectedId() - 1);
        presets.setSelectedId (0, juce::dontSendNotification);
    };
    addAndMakeVisible(source);
    source.addItemList(processor.isInstrument() ? juce::StringArray{"Synth only", "Drum machine"} : juce::StringArray{"Audio input", "Drum machine"},1);
    source.setTooltip("Choose external audio or the built-in drum sequencer.");
    sourceAttachment=std::make_unique<ComboAttachment>(processor.parameters,"source",source);
    addAndMakeVisible(drumPlay);
    drumPlayAttachment=std::make_unique<ButtonAttachment>(processor.parameters,"drumPlay",drumPlay);
    drumPlay.setTooltip("Arm drum playback. In a DAW, press host Play as well. Standalone runs at Free Tempo.");
    addAndMakeVisible(grooves);
    grooves.addItemList({"Dusty Pocket","Warehouse 909","Broken Circuit","Blank"},1);
    grooves.setTextWhenNothingSelected("LOAD DRUM GROOVE");
    grooves.onChange=[this] { if(grooves.getSelectedId()>0) processor.loadDrumGroove(grooves.getSelectedId()-1); grooves.setSelectedId(0,juce::dontSendNotification); };
    addAndMakeVisible(exportMidiButton);
    exportMidiButton.setTooltip("Save one 4/4 bar of source drum notes, with swing, accents, levels and mutes. Import into a Logic drum instrument track. FLIP audio effects are not MIDI notes.");
    exportMidiButton.onClick = [this] { exportMidi(); };
    configure(dust,dustLabel,"DUST"); configure(grooveSwing,grooveSwingLabel,"GROOVE SWING");
    dustLabel.setColour(juce::Label::textColourId,mint);
    grooveSwingLabel.setColour(juce::Label::textColourId,mint);
    drumPlay.setColour(juce::ToggleButton::textColourId,mint);
    for(auto* slider:{&dust,&grooveSwing}) {
        slider->textFromValueFunction=[](double v){return juce::String(juce::roundToInt(v*100))+"%";};
        slider->valueFromTextFunction=[](const juce::String& t){return t.getDoubleValue()*.01;};
    }
    dustAttachment=std::make_unique<SliderAttachment>(processor.parameters,"dust",dust);
    grooveSwingAttachment=std::make_unique<SliderAttachment>(processor.parameters,"grooveSwing",grooveSwing);
    dust.setTooltip("Low-pass, saturation, reduced bit depth and sample hold for lo-fi drum color.");
    grooveSwing.setTooltip("Delay alternate drum steps. Repeat Swing separately shapes FLIP pulses.");
    for(int tr=0;tr<8;++tr) {
        auto& pad=drumPads[tr]; pad.setButtonText(beatflip::drumNames[tr]); addAndMakeVisible(pad);
        pad.onClick=[this,tr]{processor.auditionDrum(tr);}; pad.setTooltip("Audition this voice in Drum machine mode.");
        auto& mute=drumMutes[tr]; mute.setButtonText("M"); addAndMakeVisible(mute);
        drumMuteAttachments[tr]=std::make_unique<ButtonAttachment>(processor.parameters,"drumMute"+juce::String(tr),mute);
        auto& level=drumLevels[tr]; level.setSliderStyle(juce::Slider::LinearHorizontal); level.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); addAndMakeVisible(level);
        level.setTooltip(juce::String(beatflip::drumNames[tr])+" level");
        drumLevelAttachments[tr]=std::make_unique<SliderAttachment>(processor.parameters,"drumLevel"+juce::String(tr),level);
        for(int st=0;st<16;++st) {
            auto& cell=drumGrid[tr][st]; addAndMakeVisible(cell);
            cell.setTitle(juce::String(beatflip::drumNames[tr])+" step "+juce::String(st+1));
            cell.setTooltip("Click: off, hit, accent. Space/Return activates the focused step.");
            cell.onClick=[this,tr,st]{processor.cycleDrumStep(tr,st);};
        }
    }
    // Attachments install their own conversion callbacks. Apply display units afterwards.
    for (auto* slider : { &amount, &mix, &swing, &dust, &grooveSwing }) {
        slider->textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt(value * 100.0)) + "%"; };
        slider->valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() * .01; };
        slider->updateText();
    }
    addAndMakeVisible(synthOn); addAndMakeVisible(synthBank); addAndMakeVisible(synthPanic); addAndMakeVisible(synthStatus); addAndMakeVisible(synthKeyboard);
    synthBank.addItemList({"CLASSIC", "WARM", "SPECTRAL"},1);
    synthOnAttachment=std::make_unique<ButtonAttachment>(processor.parameters,"synthEnabled",synthOn);
    synthBankAttachment=std::make_unique<ComboAttachment>(processor.parameters,"synthBank",synthBank);
    synthPanic.onClick=[this] { synthKeyboard.releaseAll(); synthSequencePlay.setToggleState(false,juce::sendNotification); processor.panicSynth(); };
    synthPanic.setTooltip("Silence all synth voices and release the sustain pedal.");
    synthKeyboard.noteChanged=[this](int note,bool down) {
        if(down) if(auto* p=processor.parameters.getParameter("synthEnabled")) p->setValueNotifyingHost(1);
        processor.queueSynthNote(note,down);
    };
    const std::array<const char*,8> ids {"synthPosition","synthLevel","synthTune","synthCutoff","synthAttack","synthDecay","synthSustain","synthRelease"};
    const std::array<const char*,8> names {"WAVE POSITION","SYNTH LEVEL","TUNE","CUTOFF","ATTACK","DECAY","SUSTAIN","RELEASE"};
    for(std::size_t i=0;i<ids.size();++i) {
        configure(synthControls[i],synthLabels[i],names[i]);
        synthAttachments[i]=std::make_unique<SliderAttachment>(processor.parameters,ids[i],synthControls[i]);
        auto& slider=synthControls[i];
        slider.textFromValueFunction=[i](double value) {
            if(i==0 || i==1 || i==6) return juce::String(juce::roundToInt(value*100))+"%";
            if(i==2) return juce::String(value,1)+" st";
            if(i==3) return value>=1000 ? juce::String(value/1000,1)+" kHz" : juce::String(juce::roundToInt(value))+" Hz";
            return value<1 ? juce::String(juce::roundToInt(value*1000))+" ms" : juce::String(value,2)+" s";
        };
        slider.valueFromTextFunction=[i](const juce::String& text) {
            auto value=text.getDoubleValue();
            if(i==0 || i==1 || i==6) value*=.01;
            else if(i==3 && text.containsIgnoreCase("k")) value*=1000;
            else if(i>=4 && i!=6 && text.containsIgnoreCase("ms")) value*=.001;
            return value;
        };
        slider.updateText();
    }
    synthControls[0].setTooltip("Morph continuously across four frames in the selected wavetable bank.");
    synthControls[2].setTooltip("Transpose the synth by up to two octaves. MIDI pitch bend adds +/- 2 semitones.");
    for(auto* component:std::initializer_list<juce::Component*>{&synthSequencePlay,&synthSequencePreset,&synthSequenceClear,&synthStepNote,&synthStepChord,&synthStepVelocity,&synthStepGate,&synthSelectedLabel,&synthNoteLabel,&synthChordLabel,&synthVelocityLabel,&synthGateLabel}) addAndMakeVisible(component);
    synthSequenceAttachment=std::make_unique<ButtonAttachment>(processor.parameters,"synthSeqPlay",synthSequencePlay);
    synthSequencePlay.setTooltip("Run the synth pattern with host transport or the standalone clock. Live keys always play immediately.");
    synthSequencePreset.addItemList({"Blank","Bassline","Chord stabs"},1);
    synthSequencePreset.setTextWhenNothingSelected("LOAD SYNTH PATTERN");
    synthSequencePreset.onChange=[this] { processor.loadSynthPattern(synthSequencePreset.getSelectedId()-1); synthSequencePreset.setSelectedId(0,juce::dontSendNotification); refreshSynthSequence(); };
    synthSequenceClear.onClick=[this] { processor.loadSynthPattern(0); refreshSynthSequence(); };
    synthStepNote.addItem("REST",1);
    for(int note=0;note<128;++note) synthStepNote.addItem(juce::MidiMessage::getMidiNoteName(note,true,true,4),note+2);
    synthStepChord.addItemList({"Single","Major","Minor","Sus2","Octave"},1);
    synthStepNote.onChange=[this] { processor.editSynthStep(selectedSynthStep,"Note",static_cast<float>(synthStepNote.getSelectedId()-2)); };
    synthStepChord.onChange=[this] { processor.editSynthStep(selectedSynthStep,"Chord",static_cast<float>(synthStepChord.getSelectedId()-1)); };
    for(auto* slider:{&synthStepVelocity,&synthStepGate}) {
        slider->setSliderStyle(juce::Slider::LinearHorizontal); slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,54,24);
        slider->setRange(slider==&synthStepGate?.1:.01,1,.01);
        slider->textFromValueFunction=[](double value) { return juce::String(juce::roundToInt(value*100))+"%"; };
        slider->valueFromTextFunction=[](const juce::String& text) { return text.getDoubleValue()*.01; };
    }
    synthStepVelocity.onValueChange=[this] { processor.editSynthStep(selectedSynthStep,"Velocity",static_cast<float>(synthStepVelocity.getValue())); };
    synthStepGate.onValueChange=[this] { processor.editSynthStep(selectedSynthStep,"Gate",static_cast<float>(synthStepGate.getValue())); };
    for(const auto& pair:std::initializer_list<std::pair<juce::Label*,const char*>>{{&synthNoteLabel,"NOTE / REST"},{&synthChordLabel,"CHORD"},{&synthVelocityLabel,"VELOCITY"},{&synthGateLabel,"GATE"}}) {
        pair.first->setText(pair.second,juce::dontSendNotification); pair.first->setColour(juce::Label::textColourId,muted);
        pair.first->setFont(juce::FontOptions{12.0f,juce::Font::bold});
    }
    synthSelectedLabel.setColour(juce::Label::textColourId,amber);
    for(std::size_t step=0;step<16;++step) {
        auto& cell=synthSequenceGrid[step]; addAndMakeVisible(cell);
        cell.getProperties().set("synthStep",static_cast<int>(step));
        cell.setTooltip("Select this synth step, then edit its note/rest, chord, velocity and gate.");
        cell.onClick=[this,step] { selectedSynthStep=static_cast<int>(step); refreshSynthSequence(); };
    }
    if(processor.isInstrument()) source.setTooltip("Choose silence or the drum machine as the synth's backing source. This instrument has no audio input.");
    // The performance face stays visible while the editor below changes view.
    drumView={&grooves,&exportMidiButton,&dust,&dustLabel,&grooveSwing,&grooveSwingLabel};
    for(int tr=0;tr<8;++tr) { drumView.push_back(&drumPads[tr]); drumView.push_back(&drumMutes[tr]); drumView.push_back(&drumLevels[tr]); for(auto& cell:drumGrid[tr]) drumView.push_back(&cell); }
    soundView={&synthBank,&synthStatus,&synthPanic,&synthKeyboard};
    for(std::size_t i=0;i<synthControls.size();++i) { soundView.push_back(&synthControls[i]); soundView.push_back(&synthLabels[i]); }
    sequenceView={&synthSequencePlay,&synthSequencePreset,&synthSequenceClear,&synthStepNote,&synthStepChord,&synthStepVelocity,&synthStepGate,&synthSelectedLabel,&synthNoteLabel,&synthChordLabel,&synthVelocityLabel,&synthGateLabel};
    for(auto& cell:synthSequenceGrid) sequenceView.push_back(&cell);
    fxView={&mix,&mixLabel,&swing,&swingLabel,&repeats,&repeatsLabel,&autoFlip,&autoFlipLabel,&protectButton};
    for(auto& button:effectButtons) fxView.push_back(&button);
    heldPadNotes.fill(-1);
    const char* views[]{"DRUMS","SOUND","PATTERN","FX / FLIP"};
    for(int i=0;i<4;++i) {
        auto& button=viewButtons[i]; button.setButtonText(views[i]); addAndMakeVisible(button); button.onClick=[this,i]{setView(i);};
        auto& group=groupButtons[i]; group.setButtonText(juce::String::charToString(static_cast<juce::juce_wchar>('A'+i))); addAndMakeVisible(group);
        group.setTooltip("Recall this group's source pattern. Other groups remain intact.");
        group.onClick=[this,i]{processor.selectPerformancePattern(i,patternNumber.getSelectedId());timerCallback();};
    }
    for(auto* control:std::initializer_list<juce::Component*>{&padBank,&patternNumber,&faderAssignment,&masterFader,&faderValue}) addAndMakeVisible(control);
    padBank.addItemList({"DRUM PADS","CHROMATIC KEYS"},1); padBank.setSelectedId(1);
    padBank.onChange=[this]{for(auto& note:heldPadNotes) { if(note>=0) processor.queueSynthNote(note,false); note=-1; } updatePerformancePads();};
    for(int i=1;i<=99;++i) patternNumber.addItem("PATTERN "+juce::String(i).paddedLeft('0',2),i);
    patternNumber.setSelectedId(processor.performancePattern(),juce::dontSendNotification);
    patternNumber.onChange=[this]{processor.selectPerformancePattern(processor.performanceGroup(),patternNumber.getSelectedId());timerCallback();};
    padBank.setName("Pad sound bank"); patternNumber.setName("Pattern number"); faderAssignment.setName("Fader assignment"); masterFader.setName("Master performance fader");
    masterFader.setSliderStyle(juce::Slider::LinearVertical); masterFader.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    masterFader.setColour(juce::Slider::trackColourId,coral); faderValue.setColour(juce::Label::textColourId,mint); faderValue.setJustificationType(juce::Justification::centred);
    faderAssignment.addItemList({"PITCH","FILTER","SYNTH LEVEL","DUST","FLIP MIX","WAVE MORPH"},1);
    faderAssignment.onChange=[this]{
        // End the old gesture before rebinding. Attachments preserve host automation.
        faderAttachment.reset();
        const char* ids[]{"synthTune","synthCutoff","synthLevel","dust","mix","synthPosition"};
        faderAttachment=std::make_unique<SliderAttachment>(processor.parameters,ids[juce::jlimit(0,5,faderAssignment.getSelectedId()-1)],masterFader);
    };
    faderAssignment.setSelectedId(4);
    for(int i=0;i<12;++i) {
        auto& pad=performancePads[i]; addAndMakeVisible(pad); pad.getProperties().set("performancePad",i);
        pad.pressed=[this,i](float velocity){
            padFlashUntil[i]=juce::Time::getMillisecondCounter()+120;
            if(padBank.getSelectedId()==1) {
                if(auto* parameter=processor.parameters.getParameter("source")) parameter->setValueNotifyingHost(1);
                processor.auditionDrum(i<8?i:i-8,velocity*(i<8?1.0f:.55f));
            } else {
                if(auto* parameter=processor.parameters.getParameter("synthEnabled")) parameter->setValueNotifyingHost(1);
                heldPadNotes[i]=48+i; processor.queueSynthNote(48+i,true,velocity);
            }
        };
        pad.released=[this,i]{if(heldPadNotes[i]>=0) processor.queueSynthNote(heldPadNotes[i],false);heldPadNotes[i]=-1;};
        pad.onClick=[this,i]{if(!performancePads[i].isMouseOver()) {performancePads[i].pressed(.8f);
            juce::Timer::callAfterDelay(100,[safe=juce::Component::SafePointer<BeatFlipEditor>(this),i]{if(safe!=nullptr) safe->performancePads[i].released();});}};
    }
    updatePerformancePads(); setView(0);
    timerCallback();
    startTimerHz (30);
}

BeatFlipEditor::~BeatFlipEditor()
{
    synthKeyboard.releaseAll();
    for(auto note:heldPadNotes) if(note>=0) processor.queueSynthNote(note,false);
    synthKeyboard.noteChanged=nullptr;
    stopTimer();
    setLookAndFeel (nullptr);
}

void BeatFlipEditor::exportMidi()
{
    if (midiChooser) return;
    beatflip::DrumSettings settings;
    for (int track = 0; track < 8; ++track) {
        settings.levels[track] = processor.parameters.getRawParameterValue("drumLevel" + juce::String(track))->load();
        settings.muted[track] = processor.parameters.getRawParameterValue("drumMute" + juce::String(track))->load() > .5f;
        for (int step = 0; step < 16; ++step)
            settings.pattern[track][step] = static_cast<int>(processor.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(track, step))->load());
    }
    settings.swing = processor.parameters.getRawParameterValue("grooveSwing")->load();
    const double bpm = processor.displayedHostSync.load() ? processor.displayedBpm.load()
        : processor.parameters.getRawParameterValue("tempo")->load();
    const auto bytes = beatflip::exportDrumMidi(settings, bpm);
    midiChooser = std::make_unique<juce::FileChooser>("Export source drum sequence as MIDI",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Dustbox-Drums.mid"), "*.mid");
    exportMidiButton.setEnabled(false);
    midiChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::warnAboutOverwriting,
        [safe = juce::Component::SafePointer<BeatFlipEditor>(this), bytes](const juce::FileChooser& chooser) {
            if (safe == nullptr) return;
            const auto destination = chooser.getResult();
            if (destination != juce::File()) {
                // TemporaryFile replaces an existing file only after all bytes are written.
                juce::TemporaryFile temporary(destination);
                if (!temporary.getFile().replaceWithData(bytes.data(), bytes.size()) || !temporary.overwriteTargetFileWithTemporary())
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "MIDI export failed", "The MIDI file could not be saved. Choose a writable location and try again.");
            }
            safe->exportMidiButton.setEnabled(true);
            safe->midiChooser.reset();
        });
}

void BeatFlipEditor::timerCallback()
{
    refreshSynthSequence();
    patternNumber.setSelectedId(processor.performancePattern(),juce::dontSendNotification);
    for(int i=0;i<4;++i) groupButtons[i].setColour(juce::TextButton::buttonColourId,i==processor.performanceGroup()?coral:juce::Colour(0xffeceae1));
    faderValue.setText(juce::String(masterFader.getValue(),faderAssignment.getSelectedId()==1?1:0),juce::dontSendNotification);
    for(int i=0;i<12;++i) {
        const int drumStep=processor.displayedDrumStep.load();
        const bool sequenceHit=padBank.getSelectedId()==1 && drumStep>=0 && processor.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(i<8?i:i-8,drumStep))->load()>0;
        const bool lit=sequenceHit || heldPadNotes[i]>=0 || static_cast<juce::int32>(padFlashUntil[i]-juce::Time::getMillisecondCounter())>0;
        performancePads[i].getProperties().set("activeStep",lit);
        performancePads[i].repaint();
    }
    synthStatus.setText(juce::String(processor.displayedVoices.load())+" / 16 VOICES",juce::dontSendNotification);
    const int active=processor.displayedDrumStep.load();
    for(int tr=0;tr<8;++tr) for(int st=0;st<16;++st) {
        const auto value=static_cast<int>(processor.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(tr,st))->load());
        auto& cell=drumGrid[tr][st];
        cell.setButtonText(value == 2 ? "ACCENT" : value == 1 ? "HIT" : "OFF");
        cell.setDescription(juce::String(beatflip::drumNames[tr]) + " step " + juce::String(st + 1) + ": " + cell.getButtonText());
        cell.getProperties().set("stepValue", value);
        cell.getProperties().set("activeStep", st == active);
        cell.setColour(juce::TextButton::buttonColourId,value==2?coral:value==1?blue:juce::Colour { 0xff303435 });
        cell.repaint();
    }
    repaint();
}
void BeatFlipEditor::refreshSynthSequence()
{
    for(int step=0;step<16;++step) {
        const int note=static_cast<int>(processor.parameters.getRawParameterValue(BeatFlipProcessor::synthStepId(step,"Note"))->load());
        const int chord=static_cast<int>(processor.parameters.getRawParameterValue(BeatFlipProcessor::synthStepId(step,"Chord"))->load());
        const char* names[]{""," maj"," min"," sus"," oct"};
        auto& cell=synthSequenceGrid[static_cast<std::size_t>(step)];
        const auto name=note<0?juce::String("REST"):juce::MidiMessage::getMidiNoteName(note,true,true,4)+names[juce::jlimit(0,4,chord)];
        cell.setButtonText(name); cell.setDescription("Synth step "+juce::String(step+1)+": "+name);
        cell.setColour(juce::TextButton::buttonColourId,note<0?juce::Colour{0xff303435}:blue);
        cell.getProperties().set("selectedStep",step==selectedSynthStep); cell.getProperties().set("activeStep",step==processor.displayedSynthStep.load()); cell.repaint();
    }
    const auto read=[this](const char* field) { return processor.parameters.getRawParameterValue(BeatFlipProcessor::synthStepId(selectedSynthStep,field))->load(); };
    synthSelectedLabel.setText("STEP "+juce::String(selectedSynthStep+1).paddedLeft('0',2),juce::dontSendNotification);
    synthStepNote.setSelectedId(static_cast<int>(read("Note"))+2,juce::dontSendNotification);
    synthStepChord.setSelectedId(static_cast<int>(read("Chord"))+1,juce::dontSendNotification);
    if(!synthStepVelocity.isMouseButtonDown()) synthStepVelocity.setValue(read("Velocity"),juce::dontSendNotification);
    if(!synthStepGate.isMouseButtonDown()) synthStepGate.setValue(read("Gate"),juce::dontSendNotification);
}
void BeatFlipEditor::setView(int view)
{
    currentView=juce::jlimit(0,3,view);
    const std::array<std::vector<juce::Component*>*,4> lists{&drumView,&soundView,&sequenceView,&fxView};
    for(int i=0;i<4;++i) { for(auto* component:*lists[i]) component->setVisible(i==currentView); viewButtons[i].setColour(juce::TextButton::buttonColourId,i==currentView?coral:juce::Colour(0xffeceae1)); }
    resized(); repaint();
}
void BeatFlipEditor::updatePerformancePads()
{
    for(int i=0;i<12;++i) {
        auto& pad=performancePads[i];
        pad.setButtonText(padBank.getSelectedId()==1 ? juce::String(beatflip::drumNames[i<8?i:i-8])+(i>=8?" SOFT":"") : juce::MidiMessage::getMidiNoteName(48+i,true,true,4));
        pad.setTitle("Performance pad "+juce::String(i+1)+": "+pad.getButtonText());
        pad.setTooltip("Press near the top for louder velocity; near the bottom for softer. Keys sustain until release.");
        pad.setColour(juce::TextButton::buttonColourId,i==11?coral:juce::Colour(0xffeceae1));
    }
}
void BeatFlipEditor::resized()
{
    presets.setBounds(674,24,210,30);
    for(int i=0;i<4;++i) { viewButtons[i].setBounds(36+i*143,203,132,34); groupButtons[i].setBounds(36+i*54,263,46,34); }
    patternNumber.setBounds(272,263,172,34); padBank.setBounds(620,203,432,34);
    source.setBounds(36,310,160,32); drumPlay.setBounds(216,310,146,32);
    flipButton.setBounds(36,361,160,76); keepButton.setBounds(36,451,160,32);
    enabledButton.setBounds(216,450,114,28); synthOn.setBounds(338,450,114,28);
    amount.setBounds(216,350,110,78); amountLabel.setBounds(216,429,110,18);
    tempo.setBounds(344,350,110,78); tempoLabel.setBounds(344,429,110,18);
    output.setSliderStyle(juce::Slider::LinearHorizontal); output.setTextBoxStyle(juce::Slider::TextBoxRight,false,50,22);
    output.setBounds(344,477,110,30); outputLabel.setBounds(216,483,110,18);
    faderAssignment.setBounds(466,250,136,32); masterFader.setBounds(498,291,76,188); faderValue.setBounds(466,480,136,26);
    for(int i=0;i<12;++i) performancePads[i].setBounds(620+(i%3)*146,250+(i/3)*65,140,59);
    grooves.setBounds(170,550,190,30); exportMidiButton.setBounds(374,550,128,30);
    dust.setSliderStyle(juce::Slider::LinearHorizontal); dust.setTextBoxStyle(juce::Slider::TextBoxRight,false,54,24);
    grooveSwing.setSliderStyle(juce::Slider::LinearHorizontal); grooveSwing.setTextBoxStyle(juce::Slider::TextBoxRight,false,54,24);
    dustLabel.setBounds(537,545,100,16); dust.setBounds(537,563,227,28); grooveSwingLabel.setBounds(799,545,240,16); grooveSwing.setBounds(799,563,240,28);
    for(int tr=0;tr<8;++tr) {
        const int y=602+tr*28; drumPads[tr].setBounds(36,y,110,24); drumMutes[tr].setBounds(151,y,30,24); drumLevels[tr].setBounds(186,y,80,24);
        for(int st=0;st<16;++st) drumGrid[tr][st].setBounds(284+st*48,y,42,24);
    }
    synthBank.setBounds(260,550,180,30); synthStatus.setBounds(470,550,200,30); synthPanic.setBounds(947,550,104,30);
    for(std::size_t i=0;i<synthControls.size();++i) { const int x=36+static_cast<int>(i)*130; synthControls[i].setBounds(x,603,122,108); synthLabels[i].setBounds(x,714,122,18); }
    synthKeyboard.setBounds(36,758,1016,76);
    synthSequencePlay.setBounds(264,550,142,30); synthSequencePreset.setBounds(620,550,260,30); synthSequenceClear.setBounds(948,550,104,30);
    for(int st=0;st<16;++st) synthSequenceGrid[static_cast<std::size_t>(st)].setBounds(36+st*64,614,58,61);
    synthSelectedLabel.setBounds(36,719,95,28);
    synthNoteLabel.setBounds(146,705,155,18); synthStepNote.setBounds(146,728,155,30);
    synthChordLabel.setBounds(323,705,155,18); synthStepChord.setBounds(323,728,155,30);
    synthVelocityLabel.setBounds(509,705,235,18); synthStepVelocity.setBounds(509,728,235,30);
    synthGateLabel.setBounds(785,705,264,18); synthStepGate.setBounds(785,728,264,30);
    mix.setBounds(36,597,144,104); mixLabel.setBounds(36,705,144,18); swing.setBounds(216,597,144,104); swingLabel.setBounds(216,705,144,18);
    repeatsLabel.setBounds(420,605,180,18); repeats.setBounds(420,630,180,30); autoFlipLabel.setBounds(646,605,180,18); autoFlip.setBounds(646,630,180,30); protectButton.setBounds(855,630,200,28);
    for(std::size_t i=0;i<effectButtons.size();++i) effectButtons[i].setBounds(36+static_cast<int>(i)*204,782,194,28);
}
void BeatFlipEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(juce::Colour(0xffb0b1a4)); g.drawRect(getLocalBounds().reduced(12),1);
    for(int x:{24,1076}) for(int y:{24,856}) { g.setColour(juce::Colour(0xff85897f)); g.fillEllipse(static_cast<float>(x-3),static_cast<float>(y-3),6,6); }
    g.setColour(mint); g.setFont(juce::FontOptions{34.0f,juce::Font::bold}); g.drawText("dustbox",36,20,300,40,juce::Justification::centredLeft);
    g.setFont(juce::FontOptions{11.0f,juce::Font::bold}); g.drawText("RHYTHM / WAVE / FLIP",264,33,260,20,juce::Justification::centredLeft);
    g.setFont(juce::FontOptions{24.0f,juce::Font::bold}); g.drawText("DB-12",926,22,126,35,juce::Justification::centredRight);
    g.setColour(panel); g.fillRoundedRectangle(36,76,1016,110,5); g.setColour(juce::Colour(0xff111817)); g.fillRoundedRectangle(43,83,1002,96,3);
    const auto lcd=juce::Colour(0xffffad68); g.setColour(lcd); g.setFont(juce::FontOptions{42.0f,juce::Font::bold}.withName(juce::Font::getDefaultMonospacedFontName()));
    const auto group=juce::String::charToString(static_cast<juce::juce_wchar>('A'+processor.performanceGroup()));
    g.drawText(group+"."+juce::String(processor.performancePattern()).paddedLeft('0',2),62,93,230,50,juce::Justification::centredLeft);
    g.drawText(juce::String(processor.displayedBpm.load(),1),310,93,240,50,juce::Justification::centredLeft);
    g.setFont(juce::FontOptions{12.0f,juce::Font::bold}); g.drawText("PATTERN BANK",62,148,200,20,juce::Justification::centredLeft);
    g.drawText(processor.displayedHostSync.load()?"BPM / HOST SYNC":"BPM / FREE RUN",310,148,240,20,juce::Justification::centredLeft);
    const auto packed=processor.displayedPattern.load();
    for(int i=0;i<16;++i) { const auto effect=static_cast<beatflip::Effect>((packed>>(static_cast<unsigned>(i)*3u))&7u); g.setColour(i==processor.displayedStep.load()?lcd:lcd.withAlpha(.25f)); g.fillRect(592+i*27,106,20,12+static_cast<int>(effect)*5); }
    g.setColour(lcd); g.drawText(processor.displayedPlaying.load()?"PLAY  /  LIVE KEYS READY":"STOP  /  LIVE KEYS READY",590,148,430,20,juce::Justification::centredLeft);
    g.setColour(muted); g.setFont(juce::FontOptions{11.0f,juce::Font::bold}); g.drawText("GROUP",36,243,200,16,juce::Justification::centredLeft);
    g.drawText("VELOCITY: TOP LOUD / BOTTOM SOFT",620,512,432,16,juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffc8c9be)); g.fillRoundedRectangle(28,537,1044,304,4);
    g.setColour(mint); g.setFont(juce::FontOptions{14.0f,juce::Font::bold});
    const char* titles[]{"DRUM GRID","WAVETABLE SOUND","NOTE + CHORD PATTERN","FLIP ENGINE"}; g.drawText(titles[currentView],36,548,220,32,juce::Justification::centredLeft);
    if(currentView==2) { g.setFont(juce::FontOptions{12.0f}); g.drawText("Select a step, then edit note, chord, velocity and gate. Live pads always play immediately.",36,795,1000,24,juce::Justification::centredLeft); }
    g.setColour(muted); g.setFont(juce::FontOptions{11.0f}); g.drawText("8 DRUMS + 16-VOICE WAVETABLE + 16-STEP SEQUENCERS + FLIP    /    A–D: SOURCE PATTERN BANKS",36,850,1016,18,juce::Justification::centredLeft);
}
