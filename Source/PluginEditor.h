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
    void timerCallback() override;
    void exportMidi();
    std::unique_ptr<juce::FileChooser> midiChooser;
    juce::TextButton exportMidiButton { "EXPORT MIDI" };
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
    juce::ComboBox presets, repeats, autoFlip, source, grooves;
    juce::ToggleButton drumPlay { "PLAY DRUMS" };
    juce::Slider dust, grooveSwing;
    juce::Label dustLabel, grooveSwingLabel;
    std::array<std::array<juce::TextButton,16>,8> drumGrid;
    std::array<juce::TextButton,8> drumPads;
    std::array<juce::ToggleButton,8> drumMutes;
    std::array<juce::Slider,8> drumLevels;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment, mixAttachment, outputAttachment, tempoAttachment, swingAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment> enabledAttachment, protectAttachment;
    std::array<std::unique_ptr<ButtonAttachment>, 5> effectAttachments;
    std::unique_ptr<ComboAttachment> repeatsAttachment, autoFlipAttachment, sourceAttachment;
    std::unique_ptr<ButtonAttachment> drumPlayAttachment;
    std::unique_ptr<SliderAttachment> dustAttachment, grooveSwingAttachment;
    std::array<std::unique_ptr<ButtonAttachment>,8> drumMuteAttachments;
    std::array<std::unique_ptr<SliderAttachment>,8> drumLevelAttachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatFlipEditor)
};
