#include "DrumMidiExport.h"
#include <iostream>
#include <stdexcept>
#include <limits>

void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
struct Event { unsigned tick; unsigned status, note, velocity; };
struct Parsed { std::vector<Event> notes; unsigned end = 0, tempo = 0; };
Parsed parse(const std::vector<std::uint8_t>& bytes) {
    require(bytes.size() >= 22 && std::string(bytes.begin(),bytes.begin()+4)=="MThd", "MIDI header");
    require(bytes[9]==0 && bytes[11]==1 && bytes[12]==3 && bytes[13]==192, "Format 0, single track, 960 PPQ");
    unsigned length=0; for (int i=18;i<22;++i) length=(length<<8)|bytes[i];
    require(length==bytes.size()-22, "Track chunk length");
    std::size_t pos=22; unsigned tick=0; Parsed result;
    auto read = [&]() { return bytes.at(pos++); };
    while(pos<bytes.size()) {
        unsigned delta=0, b; do { b=read(); delta=(delta<<7)|(b&127); } while(b&128); tick+=delta;
        const auto status=read();
        if(status==255) {
            auto type=read(), size=read();
            if(type==81) { require(size==3,"Tempo length"); for(int i=0;i<3;++i) result.tempo=(result.tempo<<8)|read(); }
            else { if(type==47) result.end=tick; pos+=size; }
        } else { require(status==0x99 || status==0x89,"Notes use channel 10"); auto note=read(), velocity=read(); result.notes.push_back({tick,status,note,velocity}); }
    }
    return result;
}
int main() {
    try {
        beatflip::DrumSettings settings;
        settings.levels.fill(1); settings.swing=.5f;
        for(int i=0;i<8;++i) settings.pattern[i][i]=i%2?2:1;
        auto parsed=parse(beatflip::exportDrumMidi(settings,120));
        require(parsed.tempo==500000 && parsed.end==3840,"Tempo and full-bar duration");
        require(parsed.notes.size()==16,"One note-on and off per hit");
        const int mapping[]={36,38,39,42,46,41,37,51};
        for(int i=0;i<8;++i) {
            const auto on=parsed.notes[i*2], off=parsed.notes[i*2+1];
            require(on.note==static_cast<unsigned>(mapping[i]) && off.note==on.note,"All eight drum mappings");
            require(on.tick==static_cast<unsigned>(i*240+(i%2?58:0)),"Swung ticks match drum scheduling");
            require(on.velocity==static_cast<unsigned>(i%2?127:86),"Accent velocities");
            require(off.tick==on.tick+60 && off.status==0x89,"Balanced note-offs");
        }
        settings.muted[0]=true; settings.levels[1]=0; settings.levels[2]=.5f;
        parsed=parse(beatflip::exportDrumMidi(settings,96));
        require(parsed.notes.size()==12 && parsed.notes[0].note==39 && parsed.notes[0].velocity==43,"Mutes, zero levels and scaled velocities");
        require(parsed.tempo==625000,"96 BPM tempo");
        settings.pattern={}; parsed=parse(beatflip::exportDrumMidi(settings,120));
        require(parsed.notes.empty() && parsed.end==3840,"Empty grid keeps a complete silent bar");
        for(auto& row:settings.pattern) row.fill(2);
        settings.muted.fill(false); settings.levels.fill(1); settings.swing=.6f;
        parsed=parse(beatflip::exportDrumMidi(settings,120));
        require(parsed.notes.size()==256 && parsed.notes.back().tick<parsed.end,"Dense grid completes before end of track");
        settings.swing=std::numeric_limits<float>::quiet_NaN();
        parsed=parse(beatflip::exportDrumMidi(settings,std::numeric_limits<double>::quiet_NaN()));
        require(parsed.tempo==500000,"Invalid tempo fallback");
        std::cout<<"PASS MIDI format, timing, mapping, velocities, mutes, blank and dense grids\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
