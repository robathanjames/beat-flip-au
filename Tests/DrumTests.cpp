#include "DrumMachine.h"
#include "AllocationGuard.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <algorithm>
void require(bool c,const char* m){if(!c)throw std::runtime_error(m);}
int main(){try {
    for(double rate:{8000.,44100.,48000.,96000.}) {
        beatflip::DrumMachine d;d.prepare(rate);
        beatflip::DrumSettings s;s.playing=true;s.dust=.35f;
        beatflip::Transport t;t.bpm=96;
        const int n=static_cast<int>(rate*2.5);
        std::vector<float> a(n),b(n);float* out[]{a.data(),b.data()};
        for(int tr=0;tr<8;++tr) {
            s.pattern={};s.pattern[tr][0]=2;d.reset();d.process(out,2,n,s,t);
            double energy=0;for(int i=0;i<n;++i){require(std::isfinite(a[i])&&std::abs(a[i])<=.81f,"Voice must be finite and bounded");require(a[i]==b[i],"Stereo channels must match");energy+=a[i]*a[i];}
            require(energy>.001,"All eight voices must make sound");
            s.muted[tr]=true;d.reset();d.process(out,2,n,s,t);require(std::all_of(a.begin(),a.end(),[](float x){return x==0;}),"Muted voice must be silent");s.muted[tr]=false;
        }
        s.pattern={};s.pattern[0][0]=1;s.dust=0;d.reset();d.process(out,2,n,s,t);
        double regular=0;for(auto x:a)regular+=x*x;
        s.pattern[0][0]=2;d.reset();d.process(out,2,n,s,t);double accent=0;for(auto x:a)accent+=x*x;
        require(accent>regular,"Accent must be louder than a normal hit");
        s.levels[0]=0;d.reset();d.process(out,2,n,s,t);require(std::all_of(a.begin(),a.end(),[](float x){return x==0;}),"Zero level must silence a track");s.levels[0]=.8f;s.dust=.35f;
        s.pattern=beatflip::drumGroove(0);d.reset();d.process(out,2,n,s,t);const auto original=a;
        d.reset();int at=0;while(at<n){int count=std::min(137,n-at);float* segment[]{a.data()+at,b.data()+at};d.process(segment,2,count,s,t);at+=count;}
        require(a==original,"Sequencer must be independent of audio block size");
        s.playing=false;d.process(out,2,n,s,t);require(std::all_of(a.begin(),a.end(),[](float x){return x==0;}),"Stop must immediately silence drums");
        d.process(out,2,n,s,t,1);require(std::any_of(a.begin(),a.end(),[](float x){return x!=0;}),"Audition must work while stopped");
        std::array<std::atomic<float>,8> velocity {}; velocity[0].store(.2f);
        d.reset();d.process(out,2,n,s,t,1,&velocity); double soft=0;for(auto x:a)soft+=x*x;
        velocity[0].store(1); d.reset();d.process(out,2,n,s,t,1,&velocity);double hard=0;for(auto x:a)hard+=x*x;
        require(hard>soft*2,"Audition velocity must produce a meaningful dynamic range");
        s.playing=true;d.reset();allocationGuard::start();d.process(out,2,n,s,t);const auto allocations=allocationGuard::stop();require(allocations==0,"Drum callback must not allocate");
        s.pattern={};s.pattern[0][1]=2;s.swing=.6f;s.dust=0;d.reset();d.process(out,2,n,s,t);
        auto first=std::find_if(a.begin(),a.end(),[](float x){return x!=0;});
        require(std::distance(a.begin(),first)>n/16,"Swing must delay odd drum steps");
        t.hasPosition=true;t.ppq=0;d.reset();s.pattern=beatflip::drumGroove(1);d.process(out,2,n,s,t);const auto positioned=a;d.process(out,2,n,s,t);require(a==positioned,"A host seek must restart drums deterministically");
        t.playing=false;d.process(out,2,n,s,t);require(std::all_of(a.begin(),a.end(),[](float x){return x==0;}),"Host stop must silence drums");
    }
    require(beatflip::drumGroove(3)==beatflip::DrumPattern{},"Blank groove must be empty");
    std::cout<<"PASS eight voices, mutes, accents, block-size invariance, stop, audition, swing, host seeks and allocation guard at four sample rates\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
