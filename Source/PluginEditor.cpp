#include "PluginEditor.h"
#include "FactoryPresets.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xffdedcd0 };
const juce::Colour panel { 0xff252c27 };
const juce::Colour muted { 0xff666b62 };
const juce::Colour coral { 0xffdb703c };
const juce::Colour mint { 0xffebe9de };
const juce::Colour amber { 0xffedb577 };

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
    setColour (juce::TextButton::buttonColourId, mint);
    setColour (juce::TextButton::textColourOffId, background);
    setColour (juce::ToggleButton::textColourId, panel);
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
    g.setColour (panel);
    g.fillEllipse (centre.x - body, centre.y - body, body * 2, body * 2);
    g.setColour (muted);
    g.drawEllipse (centre.x - body, centre.y - body, body * 2, body * 2, 2.0f);
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
    g.setColour (face);
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
        if (active) { g.setColour (panel); g.drawRoundedRectangle (bounds.reduced (1), 3, 2); }
    }

}

void BeatFlipLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    if (button.getProperties().contains ("stepValue")) return;
    const auto face = button.findColour (juce::TextButton::buttonColourId);
    g.setColour (face.getBrightness() > .55f ? juce::Colour { 0xff202622 } : mint);
    g.setFont (juce::FontOptions { button.getButtonText() == "FLIP" ? 34.0f : 13.0f, juce::Font::bold });
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

BeatFlipEditor::BeatFlipEditor (BeatFlipProcessor& owner) : AudioProcessorEditor (owner), processor (owner)
{
    setLookAndFeel (&look);
    setSize (1100, 940);
    addAndMakeVisible (flipButton);
    flipButton.setColour (juce::TextButton::buttonColourId, coral);
    addAndMakeVisible (keepButton);
    addAndMakeVisible (enabledButton);
    addAndMakeVisible (protectButton);
    flipButton.setTooltip ("Generate a new pattern. It starts on the next grid step; Glitch switches on automatically.");
    flipButton.onClick = [this] { processor.flip(); };
    enabledButton.setTooltip ("Turn the glitch pattern on or off. The input keeps being captured.");
    keepButton.setColour (juce::TextButton::buttonColourId, mint);
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
    source.addItemList({"Audio input", "Drum machine"},1);
    source.setTooltip("Choose external audio or the built-in drum sequencer.");
    sourceAttachment=std::make_unique<ComboAttachment>(processor.parameters,"source",source);
    addAndMakeVisible(drumPlay);
    drumPlayAttachment=std::make_unique<ButtonAttachment>(processor.parameters,"drumPlay",drumPlay);
    drumPlay.setTooltip("Arm drum playback. In a DAW, press host Play as well. Standalone runs at Free Tempo.");
    addAndMakeVisible(grooves);
    grooves.addItemList({"Dusty Pocket","Warehouse 909","Broken Circuit","Blank"},1);
    grooves.setTextWhenNothingSelected("LOAD DRUM GROOVE");
    grooves.onChange=[this] { if(grooves.getSelectedId()>0) processor.loadDrumGroove(grooves.getSelectedId()-1); grooves.setSelectedId(0,juce::dontSendNotification); };
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
    timerCallback();
    startTimerHz (30);
}

BeatFlipEditor::~BeatFlipEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BeatFlipEditor::timerCallback()
{
    const int active=processor.displayedDrumStep.load();
    for(int tr=0;tr<8;++tr) for(int st=0;st<16;++st) {
        const auto value=static_cast<int>(processor.parameters.getRawParameterValue(BeatFlipProcessor::drumStepId(tr,st))->load());
        auto& cell=drumGrid[tr][st];
        cell.setButtonText(value == 2 ? "ACCENT" : value == 1 ? "HIT" : "OFF");
        cell.setDescription(juce::String(beatflip::drumNames[tr]) + " step " + juce::String(st + 1) + ": " + cell.getButtonText());
        cell.getProperties().set("stepValue", value);
        cell.getProperties().set("activeStep", st == active);
        cell.setColour(juce::TextButton::buttonColourId,value==2?coral:value==1?mint:juce::Colour { 0xffb9bfad });
        cell.repaint();
    }
    repaint();
}
void BeatFlipEditor::resized()
{
    presets.setBounds(450,34,230,32); enabledButton.setBounds(696,37,124,26);
    source.setBounds(44,120,174,34); drumPlay.setBounds(236,120,140,34); grooves.setBounds(398,120,212,34);
    dust.setBounds(784,100,110,80); dustLabel.setBounds(784,181,110,18);
    grooveSwing.setBounds(922,100,138,80); grooveSwingLabel.setBounds(922,181,138,18);
    for(int tr=0;tr<8;++tr) {
        const int y=232+tr*38;
        drumPads[tr].setBounds(30,y,116,32); drumMutes[tr].setBounds(152,y,38,32); drumLevels[tr].setBounds(192,y,78,32);
        for(int st=0;st<16;++st) drumGrid[tr][st].setBounds(286+st*48,y,42,32);
    }
    flipButton.setBounds(28,643,210,77); keepButton.setBounds(28,729,210,30);
    const auto place=[](juce::Slider& slider,juce::Label& label,int x) {slider.setBounds(x,636,144,104);label.setBounds(x,744,144,18);};
    place(amount,amountLabel,256);place(mix,mixLabel,416);place(output,outputLabel,576);place(tempo,tempoLabel,736);place(swing,swingLabel,896);
    repeatsLabel.setBounds(44,786,164,18);repeats.setBounds(44,809,164,30);
    autoFlipLabel.setBounds(230,786,164,18);autoFlip.setBounds(230,809,164,30);protectButton.setBounds(425,810,205,28);
    for(std::size_t i=0;i<effectButtons.size();++i) effectButtons[i].setBounds(44+static_cast<int>(i)*154,874,148,28);
}

void BeatFlipEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
    g.setColour (panel);
    g.setFont (juce::FontOptions { 46.0f, juce::Font::bold });
    g.drawText ("dustbox", 28, 16, 300, 50, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions { 12.0f, juce::Font::bold });
    g.drawText ("RHYTHM COMPOSER", 31, 66, 300, 18, juce::Justification::centredLeft);
    g.setColour (panel);
    g.setFont (juce::FontOptions { 28.0f, juce::Font::bold });
    g.drawText ("DB-09", 880, 22, 190, 36, juce::Justification::centredRight);
    g.setFont (juce::FontOptions { 11.0f });
    g.drawText ("DRUM MACHINE + FLIP ENGINE", 840, 62, 230, 18, juce::Justification::centredRight);
    g.fillRect (28, 90, 1044, 2);
    g.fillRoundedRectangle (28.0f, 104.0f, 1044.0f, 98.0f, 4.0f);
    g.setColour (juce::Colour { 0xffcdd0c2 });
    g.fillRoundedRectangle (28.0f, 224.0f, 1044.0f, 318.0f, 4.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (28.0f, 554.0f, 1044.0f, 76.0f, 4.0f);
    g.fillRect (28, 770, 1044, 1);
    g.setColour (muted);
    g.setFont (juce::FontOptions { 12.0f, juce::Font::bold });
    g.drawText ("SEQUENCER", 30, 205, 160, 18, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions { 11.0f });
    g.drawText ("CLICK: OFF / HIT / ACCENT", 30, 531, 250, 14, juce::Justification::centredLeft);
    for(int i=0;i<16;++i) g.drawText(juce::String(i+1),286+i*48,205,42,18,juce::Justification::centred);
    const auto activeSeed = processor.displayedSeed.load (std::memory_order_relaxed);
    const auto requestedSeed = static_cast<std::uint32_t> (processor.parameters.getRawParameterValue ("seed")->load());
    const auto baseSeed = processor.displayedBaseSeed.load (std::memory_order_relaxed);
    const auto packedPattern = processor.displayedPattern.load (std::memory_order_relaxed);
    const auto step = processor.displayedStep.load (std::memory_order_relaxed);
    const auto enabled = enabledButton.getToggleState();
    const auto playing = processor.displayedPlaying.load (std::memory_order_relaxed);
    g.setFont (juce::FontOptions { 11.0f, juce::Font::bold });
    g.setColour (coral);
    g.drawText ("PATTERN  " + juce::String (activeSeed) + (requestedSeed != baseSeed ? "  /  FLIP QUEUED" : ""),
                44, 556, 1000, 20, juce::Justification::centredLeft);

    for (int i = 0; i < beatflip::stepCount; ++i)
    {
        const auto effect = static_cast<beatflip::Effect> ((packedPattern >> (static_cast<unsigned> (i) * 3u)) & 7u);
        const auto colour = effectColour (effect);
        const auto cell = juce::Rectangle<float> (286.0f + i * 48.0f, 576.0f, 42.0f, 34.0f);
        g.setColour (colour.withAlpha (i == step && playing && enabled ? 0.34f : 0.10f));
        g.fillRoundedRectangle (cell, 3.0f);
        g.setColour (colour.withAlpha (enabled ? 1.0f : 0.4f));
        g.drawRoundedRectangle (cell, 5.0f, i == step && playing && enabled ? 2.0f : 0.6f);
        g.setFont (juce::FontOptions { 10.0f, juce::Font::bold });
        g.drawText (beatflip::effectName (effect), cell.toNearestInt(), juce::Justification::centred);
        g.setColour (mint.withAlpha (.65f));
        g.setFont (juce::FontOptions { 9.0f });
        g.drawText (juce::String (i + 1), static_cast<int> (cell.getX()), 611, 42, 17, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions { 11.0f });
    g.drawText ("EFFECT PALETTE", 44, 849, 1000, 18, juce::Justification::centredLeft);
    const auto sync = processor.displayedHostSync.load (std::memory_order_relaxed) ? "HOST SYNC" : "FREE RUN";
    const auto bpm = processor.displayedBpm.load (std::memory_order_relaxed);
    juce::String status = juce::String (sync) + "  /  " + juce::String (bpm, 1) + " BPM";
    status += ! playing ? "  /  PRESS PLAY" : processor.displayedCapturing.load() ? "  /  CAPTURING A BAR" : "  /  READY";
    g.drawText (status, 28, 923, 1044, 17, juce::Justification::centredLeft);
}
