#include "WavetableSynth.h"
#include <algorithm>
#include <cmath>

namespace beatflip {
namespace {
constexpr double pi = 3.14159265358979323846;
float safe(float v, float lo, float hi, float fallback) noexcept { return std::isfinite(v) ? std::clamp(v,lo,hi) : fallback; }
}
void WavetableSynth::prepare(double sampleRate) {
    rate = std::isfinite(sampleRate) ? std::clamp(sampleRate,8000.0,384000.0) : 48000;
    smoothing = static_cast<float>(1 - std::exp(-1/(rate*.01)));
    for(std::size_t bank=0;bank<tables.size();++bank) for(std::size_t frame=0;frame<frames;++frame) for(std::size_t band=0;band<bands;++band) {
        auto& table=tables[bank][frame][band]; double peak=0;
        for(std::size_t i=0;i<tableSize;++i) {
            double sample=0;
            for(std::size_t h=1;h<=(std::size_t{1}<<band);++h) {
                const double harmonic=static_cast<double>(h); double amplitude=0;
                if(bank==0) {
                    if(frame==0) amplitude=h==1?1:0;
                    if(frame==1 && h%2) amplitude=((h%4)==1?1:-1)/(harmonic*harmonic);
                    if(frame==2) amplitude=1/harmonic;
                    if(frame==3 && h%2) amplitude=1/harmonic;
                } else if(bank==1) {
                    amplitude=h==1?1:std::exp(-harmonic/(1.5+static_cast<double>(frame)*2)) / std::sqrt(harmonic);
                } else {
                    const double centre=2+static_cast<double>(frame)*4;
                    amplitude=(h==1?.7:0)+std::exp(-std::pow((harmonic-centre)/2.5,2))*.7;
                }
                sample+=std::sin(2*pi*harmonic*static_cast<double>(i)/static_cast<double>(tableSize))*amplitude;
            }
            table[i]=static_cast<float>(sample); peak=std::max(peak,std::abs(sample));
        }
        if(peak>0) for(auto& x:table) x=static_cast<float>(x/peak);
        table[tableSize]=table[0];
    }
    reset(); morph=settings.position; gain=settings.level; cutoff=settings.cutoff; tune=settings.tune;
}
void WavetableSynth::reset() noexcept { voices={}; pedal.fill(false); bend.fill(0); clock=0; }
void WavetableSynth::setSettings(const SynthSettings& s) noexcept {
    settings.bank=std::clamp(s.bank,0,2);
    settings.position=safe(s.position,0,1,.35f); settings.level=safe(s.level,0,1,.65f); settings.tune=safe(s.tune,-24,24,0);
    settings.attack=safe(s.attack,.001f,4,.01f); settings.decay=safe(s.decay,.005f,4,.25f);
    settings.sustain=safe(s.sustain,0,1,.65f); settings.release=safe(s.release,.01f,8,.5f);
    settings.cutoff=safe(s.cutoff,40,20000,9000);
}
void WavetableSynth::noteOn(int channel,int note,float velocity) noexcept {
    if(channel<1 || channel>16 || note<0 || note>127) return;
    velocity=safe(velocity,0,1,0); if(velocity<=0) { noteOff(channel,note); return; }
    Voice* selected=nullptr;
    for(auto& v:voices) if(v.stage!=Stage::off && v.channel==channel && v.note==note) { selected=&v; break; }
    if(!selected) for(auto& v:voices) if(v.stage==Stage::off) { selected=&v; break; }
    if(!selected) {
        selected=&voices[0];
        for(auto& v:voices) if((v.stage==Stage::release && selected->stage!=Stage::release)
            || ((v.stage==Stage::release)==(selected->stage==Stage::release) && v.age<selected->age)) selected=&v;
    }
    const float tail=selected->last;
    *selected={}; selected->stage=Stage::attack; selected->held=true;
    selected->channel=channel; selected->note=note; selected->velocity=velocity; selected->age=++clock;
    selected->tail=tail; selected->tailSamples=64;
}
void WavetableSynth::noteOff(int channel,int note) noexcept {
    if(channel<1 || channel>16 || note<0 || note>127) return;
    for(auto& v:voices) if(v.channel==channel && v.note==note && v.stage!=Stage::off) {
        v.held=false; if(!pedal[static_cast<std::size_t>(channel-1)]) v.stage=Stage::release;
    }
}
void WavetableSynth::sustainPedal(int channel,bool down) noexcept {
    if(channel<1 || channel>16) return;
    pedal[static_cast<std::size_t>(channel-1)]=down;
    if(!down) for(auto& v:voices) if(v.channel==channel && !v.held && v.stage!=Stage::off) v.stage=Stage::release;
}
void WavetableSynth::pitchWheel(int channel,int value) noexcept {
    if(channel>=1 && channel<=16) bend[static_cast<std::size_t>(channel-1)]=static_cast<float>(std::clamp(value,0,16383)-8192)/8192.0f*2;
}
void WavetableSynth::allNotesOff(int channel) noexcept {
    if(channel<1 || channel>16) return;
    for(auto& v:voices) if(v.channel==channel) { v.held=false; if(!pedal[static_cast<std::size_t>(channel-1)] && v.stage!=Stage::off) v.stage=Stage::release; }
}
void WavetableSynth::allSoundOff(int channel) noexcept {
    for(auto& v:voices) if(channel==0 || v.channel==channel) v={};
    if(channel==0) pedal.fill(false); else if(channel>=1 && channel<=16) pedal[static_cast<std::size_t>(channel-1)]=false;
}
int WavetableSynth::activeVoices() const noexcept { int count=0; for(const auto& v:voices) if(v.stage!=Stage::off) ++count; return count; }
float WavetableSynth::oscillator(Voice& voice,double frequency) noexcept {
    const double available=rate*.45/frequency; std::size_t band=0;
    while(band+1<bands && static_cast<double>(std::size_t{1}<<(band+1))<=available) ++band;
    const float framePosition=morph*3; const auto a=static_cast<std::size_t>(framePosition); const auto b=std::min(a+1,frames-1);
    const float fraction=framePosition-static_cast<float>(a);
    const double position=voice.phase*static_cast<double>(tableSize); const auto i=static_cast<std::size_t>(position);
    const float t=static_cast<float>(position-static_cast<double>(i));
    const auto& first=tables[static_cast<std::size_t>(settings.bank)][a][band];
    const auto& second=tables[static_cast<std::size_t>(settings.bank)][b][band];
    const float x=first[i]+(first[i+1]-first[i])*t, y=second[i]+(second[i+1]-second[i])*t;
    voice.phase+=frequency/rate; voice.phase-=std::floor(voice.phase);
    return x+(y-x)*fraction;
}
void WavetableSynth::render(float* const* output,int channels,int start,int count) noexcept {
    if(!output || channels<1 || start<0 || count<=0) return;
    const float attackStep=static_cast<float>(1/(settings.attack*rate));
    const float decayCoefficient=static_cast<float>(std::exp(-6/(settings.decay*rate)));
    const float releaseCoefficient=static_cast<float>(std::exp(-9/(settings.release*rate)));
    for(int i=start;i<start+count;++i) {
        morph+=(settings.position-morph)*smoothing; gain+=(settings.level-gain)*smoothing;
        cutoff+=(settings.cutoff-cutoff)*smoothing; tune+=(settings.tune-tune)*smoothing;
        const float filter=static_cast<float>(1-std::exp(-2*pi*std::min(static_cast<double>(cutoff),rate*.45)/rate));
        float left=0,right=0;
        for(std::size_t slot=0;slot<voices.size();++slot) {
            auto& v=voices[slot]; if(v.stage==Stage::off) continue;
            if(v.stage==Stage::attack) { v.envelope=std::min(1.0f,v.envelope+attackStep); if(v.envelope>=1) v.stage=Stage::decay; }
            else if(v.stage==Stage::decay) { v.envelope=settings.sustain+(v.envelope-settings.sustain)*decayCoefficient; if(std::abs(v.envelope-settings.sustain)<.0001f) v.stage=Stage::sustain; }
            else if(v.stage==Stage::sustain) v.envelope+=(settings.sustain-v.envelope)*smoothing;
            else if(v.stage==Stage::release) { v.envelope*=releaseCoefficient; if(v.envelope<.00001f) { v={}; continue; } }
            const double frequency=std::min(rate*.45,440*std::pow(2.0,(v.note-69+tune+bend[static_cast<std::size_t>(v.channel-1)])/12.0));
            v.filtered+=(oscillator(v,frequency)-v.filtered)*filter;
            float sample=v.filtered*v.envelope*v.velocity;
            if(v.tailSamples>0) { sample+=v.tail*static_cast<float>(v.tailSamples)/64; --v.tailSamples; }
            v.last=sample;
            const float pan=(static_cast<float>(slot%4)-1.5f)*.12f;
            left+=sample*(1-pan); right+=sample*(1+pan);
        }
        // A soft ceiling keeps 16-note chords bounded; the synth is mixed into the source before FLIP.
        const float l=std::tanh(left*.18f)*gain, r=std::tanh(right*.18f)*gain;
        if(channels==1) output[0][i]+=(l+r)*.5f;
        else { output[0][i]+=l; output[1][i]+=r; }
    }
}
}
