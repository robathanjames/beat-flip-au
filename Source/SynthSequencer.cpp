#include "SynthSequencer.h"
#include <algorithm>
#include <cmath>

namespace beatflip {
std::array<int,3> synthChord(int root,int type) noexcept {
    if(root<0 || root>127) return { -1,-1,-1 };
    static constexpr int intervals[5][3] {{0,-1,-1},{0,4,7},{0,3,7},{0,2,7},{0,12,-1}};
    std::array<int,3> notes {};
    for(int i=0;i<3;++i) {
        const int interval=intervals[std::clamp(type,0,4)][i];
        notes[static_cast<std::size_t>(i)]=interval<0 || root+interval>127 ? -1 : root+interval;
    }
    return notes;
}
SynthPattern synthPattern(int preset) noexcept {
    SynthPattern pattern {};
    if(preset==1) {
        const int roots[8] {36,36,43,39,36,46,43,39};
        for(int i=0;i<8;++i) pattern[static_cast<std::size_t>(i*2)]={roots[i],0,.82f,.65f};
    } else if(preset==2) {
        pattern[0]={48,2,.75f,1}; pattern[4]={56,1,.7f,1};
        pattern[8]={51,1,.75f,1}; pattern[12]={58,1,.7f,1};
    }
    return pattern;
}
void SynthSequencer::prepare(double sampleRate) {
    rate=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.0,192000.0):48000;
    synth.prepare(rate); reset();
}
void SynthSequencer::reset() noexcept {
    synth.reset(); held.fill(-1); step=-1; freePpq=expectedPpq=remaining=0;
    lastPhase=-1; wasPlaying=false;
}
void SynthSequencer::panic() noexcept { synth.allSoundOff(); held.fill(-1); remaining=0; }
void SynthSequencer::release() noexcept {
    for(int note:held) if(note>=0) synth.noteOff(1,note);
    held.fill(-1); remaining=0;
}
void SynthSequencer::process(float* const* output,int channels,int frames,const SynthSequenceSettings& s,
                            const SynthSettings& sound,const Transport& t) noexcept {
    if(!output || channels<1 || frames<=0) return;
    const bool playing=s.playing && t.playing;
    const double bpm=std::isfinite(t.bpm)?std::clamp(t.bpm,20.0,400.0):120;
    const double delta=bpm/(60*rate), beats=std::clamp(t.numerator,1,32)*4.0/std::clamp(t.denominator,1,32);
    const double anchor=t.hasBarStart&&std::isfinite(t.barStartPpq)?t.barStartPpq:0;
    const bool positioned=t.hasPosition&&std::isfinite(t.ppq);
    double ppq=positioned?t.ppq:freePpq;
    if((playing&&!wasPlaying) || (playing&&positioned&&std::abs(ppq-expectedPpq)>delta*4)) {
        panic(); step=-1; lastPhase=-1;
    }
    if(!playing) { if(wasPlaying) panic(); step=-1; lastPhase=-1; }
    synth.setSettings(sound);
    const double swing=std::isfinite(s.swing)?std::clamp(s.swing,0.0f,.6f)*.48:0;
    int rendered=0;
    for(int i=0;i<frames;++i) {
        if(playing) {
            double phase=std::fmod(ppq-anchor+1.e-10,beats); if(phase<0) phase+=beats;
            const double cell=phase/beats*16;
            int next=std::clamp(static_cast<int>(cell),0,15);
            if(next%2 && cell-next<swing) --next;
            if(next!=step || phase<lastPhase-1.e-7) {
                synth.render(output,channels,rendered,i-rendered); rendered=i;
                release(); step=next;
                const auto& item=s.pattern[static_cast<std::size_t>(step)];
                const double start=step+(step%2?swing:0), width=step%2?1-swing:1+swing;
                const double gate=std::isfinite(item.gate)?std::clamp(item.gate,.1f,1.0f):.65;
                remaining=std::ceil((start+width*gate-cell)*beats/16/delta-1.e-5);
                if(remaining>0) {
                    held=synthChord(item.note,item.chord);
                    for(int note:held) if(note>=0) synth.noteOn(1,note,item.velocity);
                }
            }
            if(remaining<=0 && held[0]>=0) {
                synth.render(output,channels,rendered,i-rendered); rendered=i; release();
            }
            --remaining; lastPhase=phase; ppq+=delta;
        }
    }
    synth.render(output,channels,rendered,frames-rendered);
    freePpq=ppq; expectedPpq=ppq; wasPlaying=playing;
}
}
