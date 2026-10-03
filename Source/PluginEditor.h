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
    juce::ToggleButton enabledButton { "GLITCH ON" };
    juce::Slider amount, mix, output, tempo;
    juce::Label amountLabel, mixLabel, outputLabel, tempoLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment, mixAttachment, outputAttachment, tempoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatFlipEditor)
};
