#include "SynthSequencer.h"
#include "AllocationGuard.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
std::vector<float> render(int block,bool positioned,float swing=0) {
    beatflip::SynthSequencer seq; seq.prepare(8000);
    beatflip::SynthSequenceSettings settings; settings.pattern=beatflip::synthPattern(1); settings.playing=true; settings.swing=swing;
    beatflip::SynthSettings sound; sound.attack=.001f; sound.release=.01f;
    beatflip::Transport transport; transport.bpm=120; transport.hasPosition=positioned;
    std::vector<float> audio(32000,0);
    for(int offset=0;offset<32000;offset+=block) {
        transport.ppq=offset/4000.0; const int count=std::min(block,32000-offset);
        float* output[]{audio.data()+offset}; seq.process(output,1,count,settings,sound,transport);
    }
    return audio;
}
void tests() {
    require(beatflip::synthChord(60,1)==std::array<int,3>{60,64,67},"Major chord intervals");
    require(beatflip::synthChord(60,2)==std::array<int,3>{60,63,67},"Minor chord intervals");
    require(beatflip::synthChord(-1,1)==std::array<int,3>{-1,-1,-1},"Rest must emit no notes");
    require(beatflip::synthChord(127,1)==std::array<int,3>{127,-1,-1},"Chord notes stay in MIDI range");
    for(float swing:{0.0f,.45f}) {
        const auto expected=render(1,true,swing);
        require(expected==render(257,true,swing),"Sequence must be block invariant with host PPQ");
        require(expected==render(1024,false,swing),"Free and host clocks must agree");
    }
    beatflip::SynthSequencer seq; seq.prepare(8000);
    beatflip::SynthSequenceSettings settings; settings.playing=true;
    settings.pattern[2]={60,0,1,.3f}; settings.pattern[6]={67,2,1,.3f}; settings.pattern[10]={72,0,1,.3f};
    beatflip::SynthSettings sound; sound.attack=.001f; sound.release=.01f;
    beatflip::Transport transport; transport.bpm=120;
    std::vector<float> audio(16000); float* output[]{audio.data()};
    allocationGuard::start(); seq.process(output,1,16000,settings,sound,transport); const auto allocations=allocationGuard::stop();
    require(allocations==0,"Synth sequence rendering must allocate no memory");
    require(std::all_of(audio.begin(),audio.begin()+2000,[](float x){return x==0;}),"Rest steps stay silent");
    for(int start:{2000,6000,10000}) require(std::any_of(audio.begin()+start,audio.begin()+start+300,[](float x){return std::abs(x)>.02f;}),"Multiple notes/chords must play within the first bar");
    for(int start:{4000,8000,12000}) require(std::all_of(audio.begin()+start,audio.begin()+start+300,[](float x){return x==0;}),"Gate and release end before later rests");
    settings.pattern=beatflip::synthPattern(2); transport.hasPosition=true; transport.ppq=0;
    std::fill(audio.begin(),audio.end(),0); seq.process(output,1,256,settings,sound,transport);
    require(seq.activeVoices()==3,"Chord step must use three voices");
    settings.playing=false; std::fill(audio.begin(),audio.end(),0); seq.process(output,1,256,settings,sound,transport);
    require(seq.currentStep()==-1 && seq.activeVoices()==0 && audio[0]==0,"Sequence stop must clear all notes");
    settings.playing=true; transport.ppq=1; seq.process(output,1,256,settings,sound,transport);
    require(seq.currentStep()==4,"Start/seek must follow the host's current step");
    seq.panic(); require(seq.activeVoices()==0,"Panic clears release tails");
}
}
int main() { try { tests(); std::cout<<"PASS synth pattern timing, chords, gate, stop, seek, block invariance and no allocation\n"; return 0; } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; } }
