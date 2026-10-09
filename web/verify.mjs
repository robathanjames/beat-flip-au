import assert from 'node:assert/strict';
import { readFileSync, existsSync } from 'node:fs';
import { performance } from 'node:perf_hooks';
import { TRACKS, EFFECTS, SYNTH_BANKS, groove, blankPattern, clonePattern, voice, renderDry, renderFlip, renderBar, renderSynthBar, wavetableSample, makeFlip, variationSeed } from './engine.mjs';
import { chordNotes, synthPattern, synthSequenceEvents } from './synth-sequence.mjs';

const settings = { tempo: 96, swing: .18, dust: .34, amount: .65, mix: .8, repeatSwing: 0, speed: 0, protect: true, effects: EFFECTS.slice(1), enabled: true, levels: [.95,.7,.55,.5,.45,.65,.6,.4], muted: Array(8).fill(false) };
const finite = data => { for (const x of data) assert.ok(Number.isFinite(x) && Math.abs(x) <= 1, 'Audio must remain finite and bounded'); };
const equalAudio = (a,b) => assert.deepEqual(a,b);
let groups = 0;
const test = (name, run) => { run(); groups++; console.log('PASS', name); };

test('Eight audible synthesized voices', () => {
  TRACKS.forEach(t => { const data = voice(t.id); finite(data); assert.ok(data.some(x => Math.abs(x) > .03), t.id + ' must produce audio'); });
});
test('Grooves, accents, mutes, and an empty sequence', () => {
  for (const name of ['pocket','warehouse','broken']) { const p = groove(name); assert.equal(p.length,8); assert.ok(p.flat().some(x=>x===2)); const audio = renderDry(p, settings); finite(audio); assert.ok(audio.some(x=>Math.abs(x)>.1)); }
  assert.ok(renderDry(blankPattern(), settings).every(x=>x===0));
  assert.ok(renderDry(groove('pocket'), {...settings,muted:Array(8).fill(true)}).every(x=>x===0));
  const p=blankPattern();p[0][0]=1;const normal=renderDry(p,settings);p[0][0]=2;const accent=renderDry(p,settings);assert.ok(accent.reduce((s,x)=>s+x*x,0)>normal.reduce((s,x)=>s+x*x,0));
});
test('Seeded flips preserve the source and repeat exactly', () => {
  const p=groove('broken'), before=clonePattern(p);const a=renderBar(p,settings,8123),b=renderBar(p,settings,8123),c=renderBar(p,settings,9191);
  equalAudio(a.audio,b.audio); assert.notDeepEqual(a.audio,c.audio); assert.deepEqual(p,before);finite(a.audio);assert.equal(a.flip[0].effect,0);
});
test('Original mode, zero Amount/Mix, and empty palette are transparent', () => {
  const dry=renderDry(groove('pocket'),settings), flip=makeFlip(909,settings);
  equalAudio(renderFlip(dry,flip,{...settings,enabled:false}),dry);
  equalAudio(renderFlip(dry,flip,{...settings,mix:0}),dry);
  equalAudio(renderFlip(dry,flip,{...settings,amount:0}),dry);
  equalAudio(renderFlip(dry,makeFlip(909,{...settings,effects:[]}),settings),dry);
});
test('Every effect changes samples with fixed subdivisions and swung pulses', () => {
  const sr=1600, dry=new Float32Array(3200); for(let i=0;i<dry.length;i++)dry[i]=Math.sin(i*.07)*.5 + i/dry.length*.1;
  for(const effect of EFFECTS.slice(1)){
    const s={...settings,amount:1,mix:1,protect:false,effects:[effect],speed:4,repeatSwing:.4};const f=makeFlip(10,s);
    assert.ok(f.every(cell=>EFFECTS[cell.effect]===effect && cell.repeats===4));const audio=renderFlip(dry,f,s,sr);finite(audio);assert.notDeepEqual(audio,dry);
  }
  const s={...settings,amount:1,mix:1,protect:false,effects:['stutter'],speed:4};const f=makeFlip(909,s);
  assert.notDeepEqual(renderFlip(dry,f,s,sr),renderFlip(dry,f,{...s,repeatSwing:.5},sr));
});
test('Tempo and sample-rate boundaries stay musical and bounded', () => {
  for(const tempo of [60,96,180])for(const sr of [44100,48000]){
    const a=renderBar(groove('warehouse'),{...settings,tempo,swing:.6,dust:1,repeatSwing:.75,speed:16},12345,sr);
    assert.ok(Math.abs(a.duration-240/tempo)<1/sr);finite(a.audio);
  }
  assert.equal(variationSeed(909,0),909);assert.notEqual(variationSeed(909,1),909);assert.equal(variationSeed(909,1048575),909);
});
test('Sixteen-voice wavetable synth morphs three banks before FLIP', () => {
  for (const bank of SYNTH_BANKS) {
    const values = [0,.25,.5,.75,1].map(position => wavetableSample(bank, position, .137));
    assert.ok(values.every(Number.isFinite)); assert.ok(new Set(values.map(x => x.toFixed(6))).size > 1, bank + ' must morph');
    const audio = renderSynthBar(Array.from({length:16}, (_,i) => ({note:48+i,velocity:.7})), {enabled:true,bank,position:.41,level:.55,tune:0,cutoff:7200,attack:.03,decay:.18,sustain:.72,release:.45}, 44100, 44100);
    finite(audio); assert.ok(audio.some(x => Math.abs(x) > .05), bank + ' must produce audio');
  }
  const synth = {enabled:true,bank:'warm',position:.6,level:.5,tune:0,cutoff:6000,attack:.02,decay:.1,sustain:.7,release:.2};
  const withSynth = renderBar(blankPattern(), {...settings,synth,synthSequenceEnabled:true,synthPattern:synthPattern('chords')}, 321, 8000);
  const withoutSynth = renderBar(blankPattern(), settings, 321, 8000);
  assert.notDeepEqual(withSynth.dry, withoutSynth.dry); assert.notDeepEqual(withSynth.audio, withSynth.dry);
});
test('Synth sequence plays multiple notes/chords in one bar with gate, rests and swing', () => {
  assert.deepEqual(chordNotes(60,1),[60,64,67]); assert.deepEqual(chordNotes(60,2),[60,63,67]);
  assert.deepEqual(chordNotes(-1,1),[]); assert.deepEqual(chordNotes(127,1),[127]);
  const pattern=synthPattern('blank'); pattern[2]={note:60,chord:0,velocity:1,gate:.3}; pattern[6]={note:67,chord:2,velocity:1,gate:.3}; pattern[10]={note:72,chord:0,velocity:1,gate:.3};
  const synth={enabled:true,bank:'classic',position:0,level:.7,tune:0,cutoff:6000,attack:.002,decay:.02,sustain:.8,release:.01};
  const audio=renderBar(blankPattern(),{...settings,tempo:120,swing:0,enabled:false,synth,synthSequenceEnabled:true,synthPattern:pattern},1,8000).audio;
  for(const start of [2000,6000,10000]) assert.ok(audio.slice(start,start+300).some(x=>Math.abs(x)>.02),'A note/chord must start at each programmed step');
  for(const start of [0,4000,8000,12000]) assert.ok(audio.slice(start,start+300).every(x=>x===0),'Rest and note-off timing must be silent');
  pattern[1]={note:62,chord:0,velocity:.6,gate:.5};
  const straight=synthSequenceEvents(pattern,120,0),swung=synthSequenceEvents(pattern,120,.6);
  assert.ok(swung[0].start>straight[0].start); assert.ok(swung[0].duration<straight[0].duration);
  assert.deepEqual(renderBar(blankPattern(),{...settings,synth,synthSequenceEnabled:false,synthPattern:pattern},1,8000).audio,new Float32Array(20000));
  const app=readFileSync('app.mjs','utf8');
  assert.ok(!app.includes('synthNotes:') && !/synthBus\.gain\.setTargetAtTime\(0/.test(app),'Live notes must never be snapshotted once per bar or muted by PLAY');
});
test('Static entrypoint has all controls and assets', () => {
  const html=readFileSync('index.html','utf8'),app=readFileSync('app.mjs','utf8');
  const ids=new Set([...html.matchAll(/\bid="([^"]+)"/g)].map(x=>x[1]));
  for(const [,id] of app.matchAll(/\$\('([^']+)'\)/g))assert.ok(ids.has(id),'Missing control: '+id);
  for(const [,asset] of html.matchAll(/(?:src|href)="([^"#]+)"/g))if(!asset.includes(':'))assert.ok(existsSync(asset),'Missing asset: '+asset);
  assert.equal(ids.size,[...html.matchAll(/\bid="([^"]+)"/g)].length,'IDs must be unique');
});
const start=performance.now();for(let i=0;i<5;i++)renderBar(groove('pocket'),settings,909+i,48000);
console.log(`${groups} checks passed. Mean rendered bar: ${((performance.now()-start)/5).toFixed(1)} ms.`);

// Switching patterns must not alias stored source sequences or overwrite sound design.
const {PatternBanks,padVelocity}=await import('./performance.mjs');
const banks=new PatternBanks();
const bankState={pattern:Array.from({length:8},()=>Array(16).fill(0)),synthPattern:Array.from({length:16},()=>({note:-1,chord:0,velocity:.8,gate:.65})),synthSequenceEnabled:true,volume:.7,synth:{cutoff:7200}};
bankState.pattern[2][7]=2;bankState.synthPattern[3].note=65;
banks.select(1,99,bankState);assert.equal(bankState.pattern[2][7],0);assert.equal(bankState.synthPattern[3].note,-1);
bankState.pattern[2][7]=1;bankState.synthPattern[3].note=72;
banks.select(0,1,bankState);assert.equal(bankState.pattern[2][7],2);assert.equal(bankState.synthPattern[3].note,65);
bankState.pattern[2][7]=0;banks.select(1,99,bankState);assert.equal(bankState.pattern[2][7],1);assert.equal(bankState.synthPattern[3].note,72);
assert.equal(bankState.volume,.7);assert.equal(bankState.synth.cutoff,7200);assert.equal(banks.select(4,100,bankState),false);
assert.equal(padVelocity(0,70),1);assert.ok(padVelocity(69,70)<.17);assert.equal(padVelocity(-30,70),1);
console.log('PASS independent A-D / 99 source patterns, preserved sound design and pad velocity bounds');
