#pragma once

#include "PluginProcessor.h"
#include "SynthKeyboard.h"
#include <juce_gui_basics/juce_gui_basics.h>

class BeatFlipLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    BeatFlipLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

// A pad owns its note until release, even if a view/bank changes during the gesture.
class PerformancePad final : public juce::TextButton
{
public:
    bool keyboardHeld = false;
    bool keyPressed(const juce::KeyPress& key) override { if(key.getKeyCode()!=juce::KeyPress::spaceKey && key.getKeyCode()!=juce::KeyPress::returnKey) return false; if(!keyboardHeld) { keyboardHeld=true; if(pressed) pressed(.8f); } return true; }
    bool keyStateChanged(bool) override { if(keyboardHeld && !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey) && !juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::returnKey)) { keyboardHeld=false; if(released) released(); return true; } return false; }
    void focusLost(FocusChangeType) override { if(keyboardHeld) { keyboardHeld=false; if(released) released(); } }
    std::function<void(float)> pressed;
    std::function<void()> released;
    void mouseDown(const juce::MouseEvent& e) override { juce::TextButton::mouseDown(e); if(pressed) pressed(juce::jlimit(.15f,1.0f,1.0f-e.position.y/static_cast<float>(getHeight())*.85f)); }
    void mouseUp(const juce::MouseEvent& e) override { juce::TextButton::mouseUp(e); if(released) released(); }
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
    void updatePerformancePads();
    void setView(int);
    std::array<PerformancePad,12> performancePads;
    std::array<juce::TextButton,4> groupButtons, viewButtons;
    std::array<int,12> heldPadNotes;
    std::array<juce::uint32,12> padFlashUntil {};
    juce::ComboBox padBank, patternNumber, faderAssignment;
    juce::Slider masterFader;
    juce::Label faderValue;
    std::vector<juce::Component*> drumView, soundView, sequenceView, fxView;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderAttachment;
    int currentView = 0;

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
    SynthKeyboard synthKeyboard;
    juce::ToggleButton synthOn { "SYNTH ON" };
    juce::ComboBox synthBank;
    juce::TextButton synthPanic { "PANIC" };
    juce::Label synthStatus;
    std::array<juce::Slider,8> synthControls;
    std::array<juce::Label,8> synthLabels;
    juce::ToggleButton synthSequencePlay { "SEQUENCE ON" };
    juce::ComboBox synthSequencePreset, synthStepNote, synthStepChord;
    juce::TextButton synthSequenceClear { "CLEAR" };
    std::array<juce::TextButton,16> synthSequenceGrid;
    juce::Slider synthStepVelocity, synthStepGate;
    juce::Label synthSelectedLabel, synthNoteLabel, synthChordLabel, synthVelocityLabel, synthGateLabel;
    int selectedSynthStep = 0;
    void refreshSynthSequence();
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> synthSequenceAttachment;
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
    std::unique_ptr<ButtonAttachment> synthOnAttachment;
    std::unique_ptr<ComboAttachment> synthBankAttachment;
    std::array<std::unique_ptr<SliderAttachment>,8> synthAttachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatFlipEditor)
};
