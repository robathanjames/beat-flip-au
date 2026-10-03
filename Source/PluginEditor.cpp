#include "PluginEditor.h"
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
    g.setFont (juce::FontOptions { 34.0f, juce::Font::bold });
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

BeatFlipEditor::BeatFlipEditor (BeatFlipProcessor& owner) : AudioProcessorEditor (owner), processor (owner)
{
    setLookAndFeel (&look);
    setSize (680, 420);
    addAndMakeVisible (flipButton);
    addAndMakeVisible (enabledButton);
    flipButton.setTooltip ("Generate a new pattern. It starts on the next grid step; Glitch switches on automatically.");
    flipButton.onClick = [this] { processor.flip(); };
    enabledButton.setTooltip ("Turn the glitch pattern on or off. The input keeps being captured.");

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
    amount.setTooltip ("How many steps get glitched. At zero the beat stays unchanged.");
    mix.setTooltip ("Blend the live drum track with the glitch pattern.");
    output.setTooltip ("Output level after the dry/wet blend.");
    tempo.setTooltip ("Tempo used when the host supplies no tempo. Logic's tempo takes precedence.");
    for (auto* slider : { &amount, &mix })
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
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "enabled", enabledButton);
    startTimerHz (30);
}

BeatFlipEditor::~BeatFlipEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BeatFlipEditor::resized()
{
    enabledButton.setBounds (514, 31, 140, 26);
    flipButton.setBounds (28, 276, 202, 93);
    const auto place = [] (juce::Slider& slider, juce::Label& label, int x)
    {
        slider.setBounds (x, 267, 99, 104);
        label.setBounds (x, 372, 99, 18);
    };
    place (amount, amountLabel, 242);
    place (mix, mixLabel, 344);
    place (output, outputLabel, 446);
    place (tempo, tempoLabel, 548);
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
    g.fillRoundedRectangle (28.0f, 110.0f, 624.0f, 141.0f, 12.0f);

    const auto activeSeed = processor.displayedSeed.load (std::memory_order_relaxed);
    const auto requestedSeed = static_cast<std::uint32_t> (processor.parameters.getRawParameterValue ("seed")->load());
    const auto pattern = beatflip::makePattern (activeSeed);
    const auto density = processor.parameters.getRawParameterValue ("amount")->load();
    const auto step = processor.displayedStep.load (std::memory_order_relaxed);
    const auto enabled = enabledButton.getToggleState();
    const auto playing = processor.displayedPlaying.load (std::memory_order_relaxed);
    g.setFont (juce::FontOptions { 11.0f, juce::Font::bold });
    g.setColour (muted);
    g.drawText ("PATTERN  " + juce::String (activeSeed) + (requestedSeed != activeSeed ? "  /  FLIP QUEUED" : ""),
                44, 119, 590, 20, juce::Justification::centredLeft);

    for (int i = 0; i < beatflip::stepCount; ++i)
    {
        auto effect = pattern[static_cast<std::size_t> (i)].effect;
        if (density < pattern[static_cast<std::size_t> (i)].threshold) effect = beatflip::Effect::clean;
        const auto colour = effectColour (effect);
        const auto cell = juce::Rectangle<float> (44.0f + i * 37.0f, 151.0f, 31.0f, 55.0f);
        g.setColour (colour.withAlpha (i == step && playing && enabled ? 0.34f : 0.10f));
        g.fillRoundedRectangle (cell, 5.0f);
        g.setColour (colour.withAlpha (enabled ? 1.0f : 0.4f));
        g.drawRoundedRectangle (cell, 5.0f, i == step && playing && enabled ? 2.0f : 0.6f);
        g.setFont (juce::FontOptions { 9.0f, juce::Font::bold });
        g.drawText (beatflip::effectName (effect), cell.toNearestInt(), juce::Justification::centred);
        g.setColour (muted);
        g.setFont (juce::FontOptions { 9.0f });
        g.drawText (juce::String (i + 1), static_cast<int> (cell.getX()), 209, 31, 17, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (juce::FontOptions { 11.0f });
    const auto sync = processor.displayedHostSync.load (std::memory_order_relaxed) ? "HOST SYNC" : "FREE RUN";
    const auto bpm = processor.displayedBpm.load (std::memory_order_relaxed);
    juce::String status = juce::String (sync) + "  /  " + juce::String (bpm, 1) + " BPM";
    status += ! playing ? "  /  PRESS PLAY" : processor.displayedCapturing.load() ? "  /  CAPTURING A BAR" : "  /  READY";
    g.drawText (status, 28, 399, 624, 17, juce::Justification::centredLeft);
}
