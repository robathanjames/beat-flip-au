#include "PluginEditor.h"
#include "FactoryPresets.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xff3b403d };
const juce::Colour panel { 0xff202622 };
const juce::Colour muted { 0xffc8c4ae };
const juce::Colour coral { 0xffed8a3c };
const juce::Colour mint { 0xffe8d7a8 };

juce::Colour effectColour (beatflip::Effect effect)
{
    switch (effect)
    {
        case beatflip::Effect::repeat: return coral;
        case beatflip::Effect::reverse: return juce::Colour { 0xffceaa76 };
        case beatflip::Effect::shuffle: return mint;
        case beatflip::Effect::gate: return juce::Colour { 0xfff0ca77 };
        case beatflip::Effect::halfSpeed: return juce::Colour { 0xff9cb9a7 };
        case beatflip::Effect::clean: return muted;
    }
    return muted;
}
}

BeatFlipLookAndFeel::BeatFlipLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour { 0xfff3ead3 });
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId, coral);
    setColour (juce::TextButton::textColourOffId, background);
    setColour (juce::ToggleButton::textColourId, juce::Colour { 0xfff3ead3 });
    setColour (juce::ToggleButton::tickColourId, mint);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, juce::Colour { 0xfff3ead3 });
    setColour (juce::ComboBox::outlineColourId, muted.withAlpha (0.3f));
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, juce::Colour { 0xfff3ead3 });
    setColour (juce::PopupMenu::highlightedBackgroundColourId, mint.darker (0.6f));
}

void BeatFlipLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float position, float start, float end, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                               static_cast<float> (w), static_cast<float> (h)).reduced (9.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    juce::Path track, value;
    track.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, start, end, true);
    value.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, start, start + position * (end - start), true);
    for (int i = 0; i < 11; ++i)
    {
        const auto tick = start + (end - start) * i / 10.0f;
        g.setColour (muted.withAlpha (.65f));
        g.drawLine (centre.x + std::sin(tick) * radius, centre.y - std::cos(tick) * radius,
                    centre.x + std::sin(tick) * (radius - 3), centre.y - std::cos(tick) * (radius - 3), 1.0f);
    }
    g.setColour (panel.brighter (0.1f));
    g.strokePath (track, juce::PathStrokeType (4.0f));
    g.setColour (mint);
    g.strokePath (value, juce::PathStrokeType (4.0f));
    g.setColour (juce::Colour { 0xff101713 });
    g.fillEllipse (centre.x - radius + 13.0f, centre.y - radius + 13.0f, 2.0f * (radius - 13.0f), 2.0f * (radius - 13.0f));
    for (int i = 0; i < 24; ++i)
    {
        const auto rib = juce::MathConstants<float>::twoPi * i / 24.0f;
        g.setColour (muted.withAlpha (.2f));
        g.drawLine (centre.x + std::sin(rib) * (radius - 14), centre.y - std::cos(rib) * (radius - 14),
                    centre.x + std::sin(rib) * (radius - 17), centre.y - std::cos(rib) * (radius - 17), 1.0f);
    }
    const auto angle = start + position * (end - start);
    g.setColour (juce::Colour { 0xfff3ead3 });
    g.drawLine (centre.x + std::sin (angle) * (radius * 0.27f), centre.y - std::cos (angle) * (radius * 0.27f),
                centre.x + std::sin (angle) * (radius * 0.62f), centre.y - std::cos (angle) * (radius * 0.62f), 2.5f);
}

void BeatFlipLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& colour, bool over, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto face = down ? colour.darker (.22f) : over ? colour.brighter (.08f) : colour;
    g.setColour (juce::Colour { 0xff0f1512 });
    g.fillRoundedRectangle (bounds.translated (0, 2), 3.0f);
    g.setGradientFill (juce::ColourGradient (face.brighter (.12f), bounds.getTopLeft(),
                                          face.darker (.15f), bounds.getBottomLeft(), false));
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (muted.withAlpha (.3f));
    g.drawRoundedRectangle (bounds.reduced (.5f), 3.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? .08f : .25f));
    g.drawLine (bounds.getX() + 4, bounds.getY() + 2, bounds.getRight() - 4, bounds.getY() + 2);

}

void BeatFlipLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
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
        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, muted);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions { 12.0f, juce::Font::bold });
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
        cell.setButtonText(value==2?"!":value==1?"+":"");
        cell.setColour(juce::TextButton::buttonColourId,value==2?coral:value==1?mint.darker(.25f):panel.brighter(st==active?.35f:.05f));
    }
    repaint();
}
void BeatFlipEditor::resized()
{
    presets.setBounds(470,32,220,30); enabledButton.setBounds(710,34,124,26);
    source.setBounds(30,100,190,30); drumPlay.setBounds(236,100,140,30); grooves.setBounds(390,100,220,30);
    dust.setBounds(820,76,100,80); dustLabel.setBounds(820,158,100,18);
    grooveSwing.setBounds(948,76,120,80); grooveSwingLabel.setBounds(948,158,120,18);
    for(int tr=0;tr<8;++tr) {
        const int y=204+tr*40;
        drumPads[tr].setBounds(30,y,116,32); drumMutes[tr].setBounds(152,y,38,32); drumLevels[tr].setBounds(192,y,78,32);
        for(int st=0;st<16;++st) drumGrid[tr][st].setBounds(286+st*48,y,42,32);
    }
    flipButton.setBounds(28,643,210,77); keepButton.setBounds(28,729,210,30);
    const auto place=[](juce::Slider& slider,juce::Label& label,int x) {slider.setBounds(x,636,104,104);label.setBounds(x,744,104,18);};
    place(amount,amountLabel,256);place(mix,mixLabel,372);place(output,outputLabel,488);place(tempo,tempoLabel,604);place(swing,swingLabel,720);
    repeatsLabel.setBounds(44,786,164,18);repeats.setBounds(44,809,164,30);
    autoFlipLabel.setBounds(230,786,164,18);autoFlip.setBounds(230,809,164,30);protectButton.setBounds(425,810,205,28);
    for(std::size_t i=0;i<effectButtons.size();++i) effectButtons[i].setBounds(44+static_cast<int>(i)*154,874,148,28);
}

void BeatFlipEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
    // Painted metal chassis with walnut side cheeks and recessed screw heads.
    for (int y = 1; y < getHeight(); y += 3) {
        g.setColour (juce::Colours::white.withAlpha (.015f));
        g.drawHorizontalLine (y, 17.0f, static_cast<float>(getWidth() - 17));
    }
    for (const int x : { 0, getWidth() - 17 }) {
        g.setGradientFill (juce::ColourGradient (juce::Colour { 0xff75462c }, static_cast<float>(x), 0,
                                              juce::Colour { 0xff342218 }, static_cast<float>(x + 17), 0, false));
        g.fillRect (x, 0, 17, getHeight());
        for (int grain = 2; grain < 17; grain += 4) {
            g.setColour (juce::Colours::black.withAlpha (.16f));
            g.drawVerticalLine (x + grain, 0, static_cast<float>(getHeight()));
        }
    }
    for (const int x : { 9, getWidth() - 9 }) for (const int y : { 16, 530, 922 }) {
        g.setColour (juce::Colour { 0xff161b18 }); g.fillEllipse (x - 4.0f, y - 4.0f, 8, 8);
        g.setColour (muted.withAlpha (.6f)); g.drawLine (x - 2.0f, y - 2.0f, x + 2.0f, y + 2.0f);
    }
    g.setColour (coral);
    g.fillRect (28, 88, 1044, 3);
    g.setColour (panel);
    g.fillRoundedRectangle (278.0f, 199.0f, 780.0f, 326.0f, 4.0f);

    g.setColour (juce::Colour { 0xfff3ead3 });
    g.setFont (juce::FontOptions { 32.0f, juce::Font::bold });
    g.drawText ("DUSTBOX", 28, 22, 320, 40, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions { 13.0f });
    g.drawText ("DB-09 / RHYTHM COMPOSER & BEAT MANGLER", 30, 66, 430, 20, juce::Justification::centredLeft);
    g.setColour (panel);
    g.fillRoundedRectangle (28.0f, 548.0f, 1044.0f, 82.0f, 4.0f);
    g.fillRoundedRectangle (28.0f, 773.0f, 1044.0f, 142.0f, 4.0f);

    g.setColour(muted); g.setFont(juce::FontOptions{12.0f});
    g.drawText("OFF > HIT > ACCENT / ORIGINAL: SWITCH GLITCH OFF",30,174,770,20,juce::Justification::centredLeft);
    for(int i=0;i<16;++i) g.drawText(juce::String(i+1),286+i*48,185,42,18,juce::Justification::centred);
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
                44, 550, 1000, 20, juce::Justification::centredLeft);

    for (int i = 0; i < beatflip::stepCount; ++i)
    {
        const auto effect = static_cast<beatflip::Effect> ((packedPattern >> (static_cast<unsigned> (i) * 3u)) & 7u);
        const auto colour = effectColour (effect);
        const auto cell = juce::Rectangle<float> (286.0f + i * 48.0f, 576.0f, 42.0f, 34.0f);
        g.setColour (colour.withAlpha (i == step && playing && enabled ? 0.34f : 0.10f));
        g.fillRoundedRectangle (cell, 5.0f);
        g.setColour (colour.withAlpha (enabled ? 1.0f : 0.4f));
        g.drawRoundedRectangle (cell, 5.0f, i == step && playing && enabled ? 2.0f : 0.6f);
        g.setFont (juce::FontOptions { 9.0f, juce::Font::bold });
        g.drawText (beatflip::effectName (effect), cell.toNearestInt(), juce::Justification::centred);
        g.setColour (muted);
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
