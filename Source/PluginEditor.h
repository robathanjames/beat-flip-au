#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class BeatFlipLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BeatFlipLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

class BeatFlipEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BeatFlipEditor (BeatFlipProcessor&);
    ~BeatFlipEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }
    BeatFlipProcessor& processor;
    BeatFlipLookAndFeel look;
    juce::TooltipWindow tooltips { this, 600 };
    juce::TextButton flipButton { "FLIP" };
    juce::TextButton keepButton { "KEEP THIS PATTERN" };
    juce::ToggleButton enabledButton { "GLITCH ON" };
    juce::ToggleButton protectButton { "PROTECT DOWNBEAT" };
    std::array<juce::ToggleButton, 5> effectButtons;
    juce::Slider amount, mix, output, tempo, swing;
    juce::Label amountLabel, mixLabel, outputLabel, tempoLabel, swingLabel, repeatsLabel, autoFlipLabel;
    juce::ComboBox presets, repeats, autoFlip;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment, mixAttachment, outputAttachment, tempoAttachment, swingAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment> enabledAttachment, protectAttachment;
    std::array<std::unique_ptr<ButtonAttachment>, 5> effectAttachments;
    std::unique_ptr<ComboAttachment> repeatsAttachment, autoFlipAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatFlipEditor)
};
