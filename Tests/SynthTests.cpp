#include "WavetableSynth.h"
#include "AllocationGuard.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
void require(bool b,const char* m) { if(!b) throw std::runtime_error(m); }
float energy(const std::vector<float>& x) { float total=0; for(float v:x) total+=v*v; return total; }
int main() {
    try {
        beatflip::WavetableSynth synth; synth.prepare(48000);
        beatflip::SynthSettings settings; settings.release=.01f; synth.setSettings(settings);
        std::vector<float> l(4096),r(4096); float* out[]={l.data(),r.data()};
        synth.noteOn(1,60,1); synth.noteOn(1,64,.8f); synth.noteOn(1,67,.6f);
        require(synth.activeVoices()==3,"Chord allocates three independent voices");
        allocationGuard::start(); synth.render(out,2,0,4096); const auto allocations=allocationGuard::stop();
        require(allocations==0 && energy(l)>.01f,"Rendering must sound and allocate no memory");
        synth.sustainPedal(1,true); synth.noteOff(1,60); synth.noteOff(1,64); synth.noteOff(1,67);
        synth.render(out,2,0,4096); require(synth.activeVoices()==3,"Pedal holds released notes");
        synth.sustainPedal(1,false); synth.render(out,2,0,4096); require(synth.activeVoices()==0,"Pedal release ends voices");
        for(int n=0;n<32;++n) synth.noteOn(1,40+n,.8f);
        require(synth.activeVoices()==16,"Polyphony capped at sixteen with voice stealing");
        synth.allSoundOff(); require(synth.activeVoices()==0,"Panic clears every voice");
        synth.noteOn(2,60,1); synth.noteOn(3,64,1); synth.allSoundOff(2);
        require(synth.activeVoices()==1,"Channel panic does not kill another channel");
        synth.allNotesOff(3); synth.render(out,2,0,4096); require(synth.activeVoices()==0,"All notes off releases the channel");
        synth.noteOn(0,60,1); synth.noteOn(1,128,1); require(synth.activeVoices()==0,"Invalid notes ignored");
        synth.noteOn(1,60,1); synth.noteOn(1,60,0); synth.render(out,2,0,4096); require(synth.activeVoices()==0,"Zero velocity is note off");
        beatflip::WavetableSynth a,b; a.prepare(48000); b.prepare(48000);
        settings.bank=2; settings.position=.79f; settings.cutoff=12000;
        a.setSettings(settings); b.setSettings(settings); a.noteOn(1,69,.8f); b.noteOn(1,69,.8f);
        std::vector<float> x(4096),y(4096); float* px[]={x.data()};float* py[]={y.data()};
        a.render(px,1,0,4096); for(int i=0;i<4096;i+=64) b.render(py,1,i,64);
        require(x==y,"Waveform must be independent of host buffer partitioning");
        float previous=-1;
        for(int bank=0;bank<3;++bank) {
            a.reset(); settings.bank=bank; a.setSettings(settings); a.noteOn(1,69,1); x.assign(4096,0); a.render(px,1,0,4096);
            const float e=energy(x); require(e>.01f && std::abs(e-previous)>.001f,"All wavetable banks must be audible and distinct"); previous=e;
        }
        a.reset(); settings.bank=0; settings.position=0; settings.cutoff=20000; settings.attack=.001f; settings.sustain=1;
        a.setSettings(settings); a.pitchWheel(1,8192); a.noteOn(1,69,1);
        std::vector<float> tone(48000); float* pTone[]={tone.data()}; a.render(pTone,1,0,48000);
        int crossings=0;for(int i=12001;i<48000;++i) if(tone[i-1]<=0 && tone[i]>0) ++crossings;
        require(std::abs(crossings/ .75-440)<3,"A4 must have correct pitch");
        a.pitchWheel(1,16383); tone.assign(48000,0); a.render(pTone,1,0,48000);
        crossings=0;for(int i=12001;i<48000;++i) if(tone[i-1]<=0 && tone[i]>0) ++crossings;
        require(std::abs(crossings/.75-493.88)<3,"Pitch wheel reaches two semitones");
        settings.level=std::numeric_limits<float>::quiet_NaN(); settings.cutoff=settings.level;
        a.setSettings(settings); for(int n=112;n<128;++n) a.noteOn(1,n,1);
        tone.assign(48000,0); a.render(pTone,1,0,48000);
        for(float v:tone) require(std::isfinite(v) && std::abs(v)<=1,"High-note chords and invalid controls stay finite and bounded");
        std::cout<<"PASS Polyphony, voice stealing, pedal, panic, wavetable banks, pitch, block invariance, bounds and allocation-free rendering\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
