#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class SynthKeyboard final : public juce::Component {
public:
    std::function<void(int,bool)> noteChanged;
    SynthKeyboard() { setWantsKeyboardFocus(true); setTitle("Synth keyboard C3 to C5"); setDescription("Click and drag to play. Computer keys A W S E D F T G Y H U J K play C3 to C4."); }
    ~SynthKeyboard() override { releaseAll(); }
    void releaseAll() { changeMouse(-1); for(std::size_t i=0;i<computerHeld.size();++i) if(computerHeld[i]) { computerHeld[i]=false; if(noteChanged) noteChanged(48+static_cast<int>(i),false); } }
    void paint(juce::Graphics& g) override {
        const float width=static_cast<float>(getWidth())/15;
        int white=0;
        for(int n=0;n<25;++n) if(!black(n)) {
            const auto key=juce::Rectangle<float>(white++*width,0,width-2,static_cast<float>(getHeight()));
            const auto face=held(n)?juce::Colour(0xffd77432):juce::Colour(0xffe5e5dc);
            g.setGradientFill(juce::ColourGradient(face.darker(.2f),key.getX(),0,face,key.getX()+width*.5f,key.getBottom(),false)); g.fillRoundedRectangle(key,2);
            g.setColour(juce::Colours::black.withAlpha(.18f)); g.fillRect(key.withHeight(7));
            if(n%12==0) { g.setColour(juce::Colour(0xff252c27)); g.setFont(11); g.drawText("C"+juce::String(3+n/12),key.toNearestInt().removeFromBottom(18),juce::Justification::centred); }
        }
        white=0;
        for(int n=0;n<25;++n) {
            if(!black(n)) { ++white; continue; }
            const auto key=juce::Rectangle<float>((white-.32f)*width,0,width*.62f,getHeight()*.62f);
            const auto face=held(n)?juce::Colour(0xffd77432):juce::Colour(0xff161819);
            g.setGradientFill(juce::ColourGradient(face.brighter(.25f),key.getX(),0,face,key.getRight(),key.getBottom(),false));
            g.fillRoundedRectangle(key,2);
            g.setColour(face.brighter(.18f)); g.fillRect(key.withTrimmedTop(key.getHeight()-7));
        }
    }
    void mouseDown(const juce::MouseEvent& e) override { grabKeyboardFocus(); changeMouse(noteAt(e.position)); }
    void mouseDrag(const juce::MouseEvent& e) override { changeMouse(noteAt(e.position)); }
    void mouseUp(const juce::MouseEvent&) override { changeMouse(-1); }
    void focusLost(FocusChangeType) override { releaseAll(); }
    bool keyStateChanged(bool) override {
        const char* keys="AWSEDFTGYHUJK"; bool changed=false;
        for(std::size_t i=0;i<computerHeld.size();++i) {
            const bool down=juce::KeyPress::isKeyCurrentlyDown(keys[i]);
            if(down!=computerHeld[i]) { computerHeld[i]=down; changed=true; if(noteChanged && mouseNote!=48+static_cast<int>(i)) noteChanged(48+static_cast<int>(i),down); }
        }
        if(changed) repaint();
        return changed;
    }
private:
    int mouseNote=-1;
    std::array<bool,13> computerHeld {};
    static bool black(int n) { const int p=n%12; return p==1 || p==3 || p==6 || p==8 || p==10; }
    bool held(int n) const { return mouseNote==48+n || (n<13 && computerHeld[static_cast<std::size_t>(n)]); }
    int noteAt(juce::Point<float> position) const {
        if(!getLocalBounds().toFloat().contains(position)) return -1;
        const float width=static_cast<float>(getWidth())/15; int white=0;
        if(position.y<getHeight()*.62f) for(int n=0;n<25;++n) { if(!black(n)) ++white; else if(position.x>=(white-.32f)*width && position.x<(white+.30f)*width) return 48+n; }
        const int index=static_cast<int>(position.x/width); white=0;
        for(int n=0;n<25;++n) if(!black(n) && white++==index) return 48+n;
        return -1;
    }
    bool computerHolding(int note) const { return note>=48 && note<61 && computerHeld[static_cast<std::size_t>(note-48)]; }
    void changeMouse(int note) { if(note==mouseNote) return; const auto old=mouseNote; mouseNote=note; if(noteChanged) { if(old>=0 && !computerHolding(old)) noteChanged(old,false); if(note>=0 && !computerHolding(note)) noteChanged(note,true); } repaint(); }
};
