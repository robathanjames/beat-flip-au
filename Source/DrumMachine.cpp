#include "DrumMachine.h"
#include <algorithm>
#include <cmath>
namespace beatflip {
namespace { constexpr double pi = 3.14159265358979323846; }
DrumPattern drumGroove (int index) noexcept {
    DrumPattern p {};
    auto set = [&p](int tr, std::initializer_list<int> hits, std::initializer_list<int> accents = {}) {
        for (auto s : hits) p[tr][s] = 1;
        for (auto s : accents) p[tr][s] = 2;
    };
    if (index == 0) {
        set(0,{0,6,8,14},{0,8}); set(1,{4,12},{4,12}); set(2,{12});
        set(3,{0,2,4,6,8,10,12,14},{2,10}); set(4,{7,15}); set(6,{3,11});
    } else if (index == 1) {
        set(0,{0,4,8,12},{0,8}); set(1,{4,12}); set(2,{4,12},{12});
        set(3,{0,2,4,6,8,10,12,14}); set(4,{2,6,10,14}); set(7,{0,8});
    } else if (index == 2) {
        set(0,{0,3,6,10,14},{0,10}); set(1,{4,7,12,15},{4,12});
        set(3,{0,1,2,4,6,7,8,9,10,12,14,15},{2,6,10,14}); set(4,{11}); set(5,{13}); set(6,{5,11});
    }
    return p;
}
void DrumMachine::prepare (double rate) {
    sampleRate = std::isfinite(rate) ? std::clamp(rate, 8000.0, 192000.0) : 48000.0;
    const double durations[] { .65,.38,.32,.095,.52,.55,.085,.85 };
    const double frequencies[] {205,369,522,801,1139,1567};
    for (int tr=0;tr<8;++tr) {
        auto& out=bank[tr]; out.resize(static_cast<std::size_t>(std::ceil(durations[tr]*sampleRate)));
        std::uint32_t rng=static_cast<std::uint32_t>(tr*773+909);
        double phase=0, noiseLow=0, metalLow=0;
        for (std::size_t i=0;i<out.size();++i) {
            const auto t=static_cast<double>(i)/sampleRate;
            rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5;
            const double noise=static_cast<double>(rng)/4294967296.0*2-1;
            noiseLow += .22*(noise-noiseLow); const auto bright=noise-noiseLow;
            double sample=0;
            if (tr==0 || tr==5) {
                phase += 2*pi*(tr==0 ? 49+155*std::exp(-t*55) : 91+81*std::exp(-t*34))/sampleRate;
                sample=std::sin(phase)*std::exp(-t*(tr==0 ? 8 : 10))*(tr==0 ? .94 : .72)
                    +bright*std::exp(-t*(tr==0 ? 190 : 110))*(tr==0 ? .17 : .12);
            } else if(tr==1) sample=(std::sin(2*pi*185*t)*.22+std::sin(2*pi*330*t)*.12)*std::exp(-t*22)+bright*.82*std::exp(-t*16);
            else if(tr==2) {
                double envelope=0; const double delays[]{0,.009,.020,.031};
                for(int j=0;j<4;++j) if(t>=delays[j]) envelope+=std::exp(-(t-delays[j])*(j==3?22:170))*(j==3?.55:.3);
                sample=bright*envelope*.95;
            } else if(tr==6) sample=(std::sin(2*pi*480*t)*.55+std::sin(2*pi*1720*t)*.35+bright*.13)*std::exp(-t*75);
            else {
                double metal=0; for(auto f:frequencies) metal += std::sin(2*pi*f*(tr==7?1.57:1)*t)>=0?1:-1;
                metal/=6; metalLow+=.18*(metal-metalLow);
                sample=((metal-metalLow)*.48+bright*.19)*std::exp(-t*(tr==3?65:tr==4?10:6));
                if(tr==7) sample+=std::sin(2*pi*2437*t)*std::exp(-t*11)*.16;
            }
            out[i]=static_cast<float>(std::tanh(sample)*std::min(1.0,t*1400)*std::min(1.0,(durations[tr]-t)*350));
        }
    }
    reset();
}
void DrumMachine::reset() noexcept {
    cursors.fill(-1); gains.fill(0); step=-1; freePpq=expectedPpq=0; low=held=0; holdCounter=0; wasPlaying=false;
}
void DrumMachine::process(float* const* output,int channels,int frames,const DrumSettings& s,const Transport& t,unsigned audition,const std::array<std::atomic<float>,8>* velocity) noexcept {
    const bool playing=s.playing && t.playing;
    const double bpm=std::isfinite(t.bpm)?std::clamp(t.bpm,20.0,400.0):120;
    const double delta=bpm/(60*sampleRate);
    const double beats=std::clamp(t.numerator,1,32)*4.0/std::clamp(t.denominator,1,32);
    const double start=t.hasBarStart&&std::isfinite(t.barStartPpq)?t.barStartPpq:0;
    const bool positioned=t.hasPosition&&std::isfinite(t.ppq);
    double ppq=positioned?t.ppq:freePpq;
    if ((playing&&!wasPlaying) || (playing&&positioned&&std::abs(ppq-expectedPpq)>delta*4)) {
        cursors.fill(-1); step=-1; low=held=0;
    }
    if(!playing&&wasPlaying) { cursors.fill(-1); step=-1; low=held=0; }
    for(int tr=0;tr<8;++tr) if(audition & (1u<<tr)) { cursors[tr]=0; gains[tr]=s.levels[tr]*(velocity?std::clamp((*velocity)[tr].load(),.01f,1.0f):1.0f);
        if(tr==3) cursors[4]=-1; }
    const auto dust=std::clamp(s.dust,0.0f,1.0f);
    const auto alpha=static_cast<float>(1-std::exp(-2*pi*(14000-dust*10500)/sampleRate));
    const auto quant=std::pow(2.0f,15-std::round(dust*7)); const int hold=1+static_cast<int>(dust*3);
    for(int i=0;i<frames;++i) {
        if(playing) {
            double phase=std::fmod(ppq-start,beats); if(phase<0)phase+=beats;
            const double cell=phase/beats*16;
            int next=std::min(15,static_cast<int>(cell));
            if(next%2 && cell-next<std::clamp(s.swing,0.0f,.6f)*.48) --next;
            if(next!=step) {
                step=next;
                if(s.pattern[3][step]&&!s.muted[3]) cursors[4]=-1;
                for(int tr=0;tr<8;++tr) if(s.pattern[tr][step]&&!s.muted[tr]) {
                    cursors[tr]=0; gains[tr]=s.levels[tr]*(s.pattern[tr][step]==2?1.0f:.68f);
                }
            }
            ppq+=delta;
        }
        float sample=0;
        for(int tr=0;tr<8;++tr) if(cursors[tr]>=0) {
            if(cursors[tr]<static_cast<int>(bank[tr].size())) sample+=bank[tr][static_cast<std::size_t>(cursors[tr]++)]*gains[tr]*(s.muted[tr]?0.0f:1.0f);
            else cursors[tr]=-1;
        }
        low+=alpha*(sample-low);
        if(holdCounter==0) held=std::round(std::tanh(low*(1+dust*1.6f))*quant)/quant;
        holdCounter=(holdCounter+1)%hold;
        for(int ch=0;ch<channels;++ch) output[ch][i]=held*.8f;
    }
    if(!playing) step=-1;
    freePpq=ppq; expectedPpq=positioned?t.ppq+(playing?frames*delta:0):ppq; wasPlaying=playing;
}
}
