#include "PluginEditor.h"
#include "FactoryPresets.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xff11151c };
const juce::Colour panel { 0xff1b212c };
const juce::Colour muted { 0xff909bb0 };
const juce::Colour coral { 0xffff6d5e };
const juce::Colour mint { 0xff81e4ce };

juce::Colour effectColour (beatflip::Effect effect)
{
    switch (effect)
    {
        case beatflip::Effect::repeat: return coral;
        case beatflip::Effect::reverse: return juce::Colour { 0xffb89cff };
        case beatflip::Effect::shuffle: return mint;
        case beatflip::Effect::gate: return juce::Colour { 0xfff0ca77 };
        case beatflip::Effect::halfSpeed: return juce::Colour { 0xff77b7f0 };
        case beatflip::Effect::clean: return muted;
    }
    return muted;
}
}

BeatFlipLookAndFeel::BeatFlipLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId, coral);
    setColour (juce::TextButton::textColourOffId, background);
    setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    setColour (juce::ToggleButton::tickColourId, mint);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, muted.withAlpha (0.3f));
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
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
    g.setColour (panel.brighter (0.1f));
    g.strokePath (track, juce::PathStrokeType (4.0f));
    g.setColour (mint);
    g.strokePath (value, juce::PathStrokeType (4.0f));
    g.setColour (panel);
    g.fillEllipse (centre.x - radius + 13.0f, centre.y - radius + 13.0f, 2.0f * (radius - 13.0f), 2.0f * (radius - 13.0f));
    const auto angle = start + position * (end - start);
    g.setColour (juce::Colours::white);
    g.drawLine (centre.x + std::sin (angle) * (radius * 0.27f), centre.y - std::cos (angle) * (radius * 0.27f),
                centre.x + std::sin (angle) * (radius * 0.62f), centre.y - std::cos (angle) * (radius * 0.62f), 2.5f);
}

void BeatFlipLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& colour, bool over, bool down)
{
    g.setColour (down ? colour.darker (0.2f) : over ? colour.brighter (0.1f) : colour);
    g.fillRoundedRectangle (button.getLocalBounds().toFloat().reduced (1.0f), 12.0f);
}

void BeatFlipLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setColour (background);
    g.setFont (juce::FontOptions { button.getButtonText() == "FLIP" ? 34.0f : 13.0f, juce::Font::bold });
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

BeatFlipEditor::BeatFlipEditor (BeatFlipProcessor& owner) : AudioProcessorEditor (owner), processor (owner)
{
    setLookAndFeel (&look);
    setSize (860, 580);
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
    startTimerHz (30);
}

BeatFlipEditor::~BeatFlipEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BeatFlipEditor::resized()
{
    presets.setBounds (470, 32, 220, 30);
    enabledButton.setBounds (710, 34, 124, 26);
    flipButton.setBounds (28, 271, 210, 77);
    keepButton.setBounds (28, 357, 210, 30);
    const auto place = [] (juce::Slider& slider, juce::Label& label, int x)
    {
        slider.setBounds (x, 264, 104, 104);
        label.setBounds (x, 372, 104, 18);
    };
    place (amount, amountLabel, 256);
    place (mix, mixLabel, 372);
    place (output, outputLabel, 488);
    place (tempo, tempoLabel, 604);
    place (swing, swingLabel, 720);
    repeatsLabel.setBounds (44, 414, 164, 18);
    repeats.setBounds (44, 437, 164, 30);
    autoFlipLabel.setBounds (230, 414, 164, 18);
    autoFlip.setBounds (230, 437, 164, 30);
    protectButton.setBounds (425, 438, 205, 28);
    for (std::size_t i = 0; i < effectButtons.size(); ++i)
        effectButtons[i].setBounds (44 + static_cast<int> (i) * 154, 502, 148, 28);
}

void BeatFlipEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions { 32.0f, juce::Font::bold });
    g.drawText ("BEAT FLIP", 28, 22, 320, 40, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (juce::FontOptions { 13.0f });
    g.drawText ("ONE BUTTON. A DIFFERENT POCKET.", 30, 66, 430, 20, juce::Justification::centredLeft);
    g.setColour (panel);
    g.fillRoundedRectangle (28.0f, 110.0f, 804.0f, 141.0f, 12.0f);
    g.fillRoundedRectangle (28.0f, 401.0f, 804.0f, 142.0f, 12.0f);

    const auto activeSeed = processor.displayedSeed.load (std::memory_order_relaxed);
    const auto requestedSeed = static_cast<std::uint32_t> (processor.parameters.getRawParameterValue ("seed")->load());
    const auto baseSeed = processor.displayedBaseSeed.load (std::memory_order_relaxed);
    const auto packedPattern = processor.displayedPattern.load (std::memory_order_relaxed);
    const auto step = processor.displayedStep.load (std::memory_order_relaxed);
    const auto enabled = enabledButton.getToggleState();
    const auto playing = processor.displayedPlaying.load (std::memory_order_relaxed);
    g.setFont (juce::FontOptions { 11.0f, juce::Font::bold });
    g.setColour (muted);
    g.drawText ("PATTERN  " + juce::String (activeSeed) + (requestedSeed != baseSeed ? "  /  FLIP QUEUED" : ""),
                44, 119, 760, 20, juce::Justification::centredLeft);

    for (int i = 0; i < beatflip::stepCount; ++i)
    {
        const auto effect = static_cast<beatflip::Effect> ((packedPattern >> (static_cast<unsigned> (i) * 3u)) & 7u);
        const auto colour = effectColour (effect);
        const auto cell = juce::Rectangle<float> (44.0f + i * 48.0f, 151.0f, 42.0f, 55.0f);
        g.setColour (colour.withAlpha (i == step && playing && enabled ? 0.34f : 0.10f));
        g.fillRoundedRectangle (cell, 5.0f);
        g.setColour (colour.withAlpha (enabled ? 1.0f : 0.4f));
        g.drawRoundedRectangle (cell, 5.0f, i == step && playing && enabled ? 2.0f : 0.6f);
        g.setFont (juce::FontOptions { 9.0f, juce::Font::bold });
        g.drawText (beatflip::effectName (effect), cell.toNearestInt(), juce::Justification::centred);
        g.setColour (muted);
        g.setFont (juce::FontOptions { 9.0f });
        g.drawText (juce::String (i + 1), static_cast<int> (cell.getX()), 209, 42, 17, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions { 11.0f });
    g.drawText ("EFFECT PALETTE", 44, 477, 760, 18, juce::Justification::centredLeft);
    const auto sync = processor.displayedHostSync.load (std::memory_order_relaxed) ? "HOST SYNC" : "FREE RUN";
    const auto bpm = processor.displayedBpm.load (std::memory_order_relaxed);
    juce::String status = juce::String (sync) + "  /  " + juce::String (bpm, 1) + " BPM";
    status += ! playing ? "  /  PRESS PLAY" : processor.displayedCapturing.load() ? "  /  CAPTURING A BAR" : "  /  READY";
    g.drawText (status, 28, 555, 804, 17, juce::Justification::centredLeft);
}
