import { setupPerformance } from './performance.mjs';
import { TRACKS, EFFECTS, EFFECT_LABELS, groove, blankPattern, makeFlip, renderBar, voice, colorAudio, variationSeed } from './engine.mjs';
import { CHORDS, noteName, synthPattern } from './synth-sequence.mjs';

const $ = id => document.getElementById(id);
const state = {
  pattern: groove('pocket'), tempo: 96, swing: .18, dust: .34, volume: .7,
  amount: .65, mix: .8, repeatSwing: 0, speed: 0, auto: 0, protect: true,
  effects: EFFECTS.slice(1), enabled: false, seed: 909,
  levels: [.95, .7, .55, .5, .45, .65, .6, .4], muted: Array(8).fill(false),
  synth: { enabled: true, bank: 'classic', position: .22, level: .55, tune: 0, cutoff: 7200, attack: .03, decay: .18, sustain: .72, release: .45 },
  synthSequenceEnabled: false, synthPattern: synthPattern('blank')
};
let context, output, limiter, playing = false, starting = false, startGeneration = 0;
let synthBus, midiAccess;
let timer, animation, nextTime = 0, scheduledBars = 0, sequenceAnchor = 0, revision = 0;
let events = [], sources = new Set(), shownEvent = null, previousStep = -1, previousBar = -1;
let previewTimer;
const stepButtons = [], trackElements = [], mapCells = [];
const activeNotes = new Map(), pendingNotes = new Set(), keyboardPointers = new Map(), keyButtons = new Map();
const liveVoices = new Set();
const snapshot = () => ({ ...state, pattern: state.pattern.map(x => [...x]), effects: [...state.effects], levels: [...state.levels], muted: [...state.muted], synth: { ...state.synth }, synthPattern: state.synthPattern.map(step => ({...step})) });
const announce = message => { $('audio-message').textContent = message; };
const synthSteps = [];
let selectedSynthStep = 0;
for (let note = -1; note <= 127; note++) {
  const option = document.createElement('option'); option.value = note; option.textContent = noteName(note); $('seq-note').append(option);
}
CHORDS.forEach((name,chord) => { const option=document.createElement('option'); option.value=chord; option.textContent=name; $('seq-chord').append(option); });
for (let step=0;step<16;step++) {
  const button=document.createElement('button'); button.className='synth-step';
  button.addEventListener('click', () => { selectedSynthStep=step; refreshSynthSequence(); });
  button.addEventListener('keydown', event => {
    if (!['ArrowLeft','ArrowRight','Home','End'].includes(event.key)) return;
    event.preventDefault(); selectedSynthStep=event.key==='Home'?0:event.key==='End'?15:(step+(event.key==='ArrowRight'?1:15))%16;
    refreshSynthSequence(); synthSteps[selectedSynthStep].focus();
  });
  $('synth-steps').append(button); synthSteps.push(button);
}
function refreshSynthSequence() {
  synthSteps.forEach((button,index) => {
    const step=state.synthPattern[index]; button.classList.toggle('selected',index===selectedSynthStep);
    button.classList.toggle('has-note',step.note>=0); button.tabIndex=index===selectedSynthStep?0:-1;
    button.setAttribute('aria-pressed',index===selectedSynthStep);
    button.setAttribute('aria-label',`Synth step ${index+1}: ${noteName(step.note)}${step.note>=0?' '+CHORDS[step.chord]:''}. Select to edit.`);
    button.innerHTML=`<span>${String(index+1).padStart(2,'0')}</span><strong>${noteName(step.note)}</strong><small>${step.note>=0?CHORDS[step.chord].toUpperCase():'—'}</small>`;
  });
  const step=state.synthPattern[selectedSynthStep]; $('seq-selected').textContent=`STEP ${String(selectedSynthStep+1).padStart(2,'0')}`;
  $('seq-note').value=step.note; $('seq-chord').value=step.chord;
  $('seq-velocity').value=Math.round(step.velocity*100); $('seq-gate').value=Math.round(step.gate*100);
  $('seq-velocity-value').textContent=`${Math.round(step.velocity*100)}%`; $('seq-gate-value').textContent=`${Math.round(step.gate*100)}%`;
  $('synth-seq-play').checked=state.synthSequenceEnabled;
}

for (let s = 0; s < 16; s++) {
  const number = document.createElement('span');
  number.textContent = String(s + 1).padStart(2, '0');
  if (s % 4 === 0) number.className = 'quarter';
  $('step-numbers').append(number);
}
TRACKS.forEach((track, tr) => {
  const row = document.createElement('div'); row.className = 'track'; row.style.setProperty('--voice', track.color);
  const info = document.createElement('div'); info.className = 'track-info';
  const pad = document.createElement('button'); pad.className = 'audition'; pad.setAttribute('aria-label', 'Audition ' + track.name);
  const light = document.createElement('span'); light.setAttribute('aria-hidden', 'true'); pad.append(light);
  pad.addEventListener('click', () => audition(tr));
  const name = document.createElement('div'); name.className = 'track-name'; name.textContent = track.name;
  const note = document.createElement('span'); note.className = 'track-note'; note.textContent = track.note; name.append(note);
  const mute = document.createElement('button'); mute.className = 'mute'; mute.textContent = 'M'; mute.setAttribute('aria-label', 'Mute ' + track.name); mute.setAttribute('aria-pressed', 'false');
  mute.addEventListener('click', () => { state.muted[tr] = !state.muted[tr]; refreshTracks(); changed(); });
  info.append(pad, name, mute); row.append(info);
  const steps = document.createElement('div'); steps.className = 'steps';
  stepButtons[tr] = [];
  for (let s = 0; s < 16; s++) {
    const step = document.createElement('button'); step.className = 'step'; step.dataset.track = tr; step.dataset.step = s;
    step.tabIndex = s === 0 ? 0 : -1;
    step.addEventListener('click', () => { state.pattern[tr][s] = (state.pattern[tr][s] + 1) % 3; $('groove').value = 'custom'; refreshStep(tr, s); changed(); });
    step.addEventListener('keydown', e => {
      let t = tr, n = s;
      if (e.key === 'ArrowRight') n = (s + 1) % 16;
      else if (e.key === 'ArrowLeft') n = (s + 15) % 16;
      else if (e.key === 'ArrowDown') t = (tr + 1) % 8;
      else if (e.key === 'ArrowUp') t = (tr + 7) % 8;
      else if (e.key === 'Home') n = 0;
      else if (e.key === 'End') n = 15;
      else return;
      e.preventDefault(); stepButtons[t].forEach(b => b.tabIndex = -1); stepButtons[t][n].tabIndex = 0; stepButtons[t][n].focus();
    });
    step.addEventListener('focus', () => { stepButtons[tr].forEach(b => b.tabIndex = b === step ? 0 : -1); });
    steps.append(step); stepButtons[tr].push(step);
  }
  const level = document.createElement('input'); level.type = 'range'; level.min = 0; level.max = 100; level.value = state.levels[tr] * 100;
  level.className = 'track-level'; level.setAttribute('aria-label', track.name + ' level');
  level.addEventListener('input', () => { state.levels[tr] = Number(level.value) / 100; fillRange(level); changed(); });
  fillRange(level); row.append(steps, level); $('tracks').append(row); trackElements.push({ row, mute, pad, level });
});
for (let s = 0; s < 16; s++) { const cell = document.createElement('span'); cell.className = 'effect-cell'; $('flip-map').append(cell); mapCells.push(cell); }

const blackPitch = new Set([1, 3, 6, 8, 10]);
const whitePitch = [0, 2, 4, 5, 7, 9, 11];
for (let note = 48; note <= 72; note++) {
  const pitch = note % 12, key = document.createElement('button');
  key.className = 'piano-key ' + (blackPitch.has(pitch) ? 'black' : 'white'); key.dataset.note = note;
  key.setAttribute('aria-label', `Play MIDI note ${note}`); key.style.setProperty('--note', note - 48);
  const octave = Math.floor((note - 48) / 12), before = whitePitch.filter(value => value < pitch).length;
  key.style.setProperty('--left', `${(octave * 7 + before) / 15 * 100}%`);
  key.addEventListener('pointerdown', event => { event.preventDefault(); key.setPointerCapture(event.pointerId); keyboardPointers.set(event.pointerId, note); noteOn(note, .82); });
  key.addEventListener('pointerup', event => { noteOff(keyboardPointers.get(event.pointerId)); keyboardPointers.delete(event.pointerId); });
  key.addEventListener('pointercancel', event => { noteOff(keyboardPointers.get(event.pointerId)); keyboardPointers.delete(event.pointerId); });
  $('keyboard').append(key); keyButtons.set(note, key);
}

function refreshStep(tr, s) {
  const value = state.pattern[tr][s], button = stepButtons[tr][s];
  button.dataset.value = value; button.setAttribute('aria-pressed', value ? 'true' : 'false');
  button.setAttribute('aria-label', `${TRACKS[tr].name}, step ${s + 1}: ${['off', 'hit', 'accent'][value]}`);
}
function refreshTracks() {
  TRACKS.forEach((track, tr) => { for (let s = 0; s < 16; s++) refreshStep(tr, s); trackElements[tr].row.classList.toggle('muted', state.muted[tr]); trackElements[tr].mute.setAttribute('aria-pressed', state.muted[tr]); });
}
document.querySelectorAll('.fader').forEach(control => {
  const dial = document.createElement('span'); dial.className = 'dial'; dial.setAttribute('aria-hidden', 'true');
  control.append(dial);
});
function fillRange(input) {
  const fraction = (Number(input.value) - Number(input.min)) / (Number(input.max) - Number(input.min));
  input.style.setProperty('--fill', `${fraction * 100}%`);
  input.closest('.fader')?.style.setProperty('--rotation', `${-135 + fraction * 270}deg`);
}
function refreshModes() {
  $('original').setAttribute('aria-pressed', !state.enabled); $('flipped').setAttribute('aria-pressed', state.enabled);
  $('keep').disabled = !state.enabled;
}
function changed(resetSequence = false) {
  revision++;
  if (resetSequence) sequenceAnchor = scheduledBars;
  if (playing) { $('sequence-status').textContent = 'CHANGES QUEUED'; $('flip-status').textContent = 'Changes land at a bar boundary.'; }
  else { clearTimeout(previewTimer); previewTimer = setTimeout(preview, 35); }
}
function preview() {
  const rendered = renderBar(state.pattern, snapshot(), state.seed);
  showMonitor(rendered, state.enabled);
  $('flip-status').textContent = state.enabled ? 'Ready. Press PLAY to hear your flip.' : 'Your source beat stays intact.';
}
function showMonitor(rendered, enabled) {
  rendered.flip.forEach((f, i) => { const effect = enabled ? f.effect : 0; mapCells[i].dataset.effect = effect; mapCells[i].textContent = EFFECT_LABELS[effect]; mapCells[i].setAttribute('title', `Step ${i + 1}: ${EFFECTS[effect]}`); });
  $('monitor-label').textContent = enabled ? 'FLIPPED BAR' : 'ORIGINAL BAR';
  $('seed-label').textContent = 'SEED ' + String(rendered.seed).padStart(4, '0');
  drawWaveform(rendered.audio, enabled);
}
function drawWaveform(data, enabled) {
  const canvas = $('waveform'), g = canvas.getContext('2d');
  if (!g) return;
  const w = canvas.width, h = canvas.height; g.clearRect(0, 0, w, h);
  g.strokeStyle = '#42483d'; g.lineWidth = 1; g.beginPath(); g.moveTo(0, h / 2); g.lineTo(w, h / 2); g.stroke();
  g.strokeStyle = enabled ? '#e7a763' : '#aab2a1'; g.lineWidth = 1.5; g.beginPath();
  for (let x = 0; x < w; x += 2) {
    const from = Math.floor(x / w * data.length), to = Math.min(data.length, Math.floor((x + 2) / w * data.length));
    let low = 0, high = 0;
    for (let i = from; i < to; i++) { low = Math.min(low, data[i]); high = Math.max(high, data[i]); }
    g.moveTo(x, h / 2 - high * h * .49); g.lineTo(x, h / 2 - low * h * .49);
  }
  g.stroke();
}

async function enableAudio() {
  if (!context) {
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    if (!AudioContext) throw new Error('This browser does not support audio playback.');
    context = new AudioContext({ latencyHint: 'interactive' });
    output = context.createGain(); output.gain.value = state.volume;
    limiter = context.createDynamicsCompressor(); limiter.threshold.value = -4; limiter.knee.value = 4;
    limiter.ratio.value = 12; limiter.attack.value = .001; limiter.release.value = .06;
    synthBus = context.createGain(); synthBus.gain.value = 1; synthBus.connect(limiter);
    limiter.connect(output); output.connect(context.destination);
    context.addEventListener('statechange', () => { if (playing && context.state !== 'running') { stop(); announce('Audio paused. Press PLAY to continue.'); } });
  }
  await context.resume();
  if (context.state !== 'running') throw new Error('Sound is paused. Press PLAY to try again.');
  $('power-led').parentElement.classList.add('active'); $('power-label').textContent = 'AUDIO READY';
}

const frameHarmonics = {
  classic: [[1], [1,0,-1/9,0,1/25,0,-1/49], [1,-.5,.333,-.25,.2,-.167,.143,-.125], [1,0,.333,0,.2,0,.143]],
  warm: [[1,.16,.05], [1,.28,.08,.03], [1,.35,.18,.08,.03], [1,0,.28,0,.16,0,.08]],
  spectral: [[1,0,0,0,0,0,.42], [1,0,0,0,.38,0,0,0,0,0,.24], [1,0,0,0,0,0,0,.44,0,0,0,0,.25], [1,0,0,0,0,0,0,0,.48,0,0,0,0,0,0,.28]]
};
const waveCache = new Map();
function periodicWave(bank, frame) {
  const id = bank + frame; if (waveCache.has(id)) return waveCache.get(id);
  const imag = new Float32Array([0, ...(frameHarmonics[bank] || frameHarmonics.classic)[frame]]), real = new Float32Array(imag.length);
  const wave = context.createPeriodicWave(real, imag, { disableNormalization: false }); waveCache.set(id, wave); return wave;
}
function frameWeights(position) {
  const p = Math.max(0, Math.min(1, position)) * 3, base = Math.min(2, Math.floor(p)), mix = p - base;
  return [0,1,2,3].map(i => i === base ? 1 - mix : i === base + 1 ? mix : 0);
}
function refreshVoiceDisplay() {
  const p = state.synth.position * 3, frame = Math.min(2, Math.floor(p));
  $('synth-display').textContent = `${state.synth.bank.toUpperCase()} · ${frame + 1}→${frame + 2}`;
  $('voice-count').textContent = `${activeNotes.size} / 16 VOICES`;
}
async function noteOn(note, velocity = .82) {
  if (!Number.isFinite(note) || activeNotes.has(note) || pendingNotes.has(note) || !state.synth.enabled) return;
  pendingNotes.add(note);
  try { if (!context || context.state !== 'running') await enableAudio(); } catch (error) { pendingNotes.delete(note); announce(error.message || 'Sound could not start.'); return; }
  if (!pendingNotes.delete(note) || !state.synth.enabled) return;
  if (activeNotes.size >= 16) noteOff(activeNotes.keys().next().value, true);
  const now = context.currentTime, envelope = context.createGain(), filter = context.createBiquadFilter();
  envelope.gain.setValueAtTime(0, now); envelope.gain.linearRampToValueAtTime(velocity * state.synth.level * .2, now + state.synth.attack);
  envelope.gain.linearRampToValueAtTime(velocity * state.synth.level * .2 * state.synth.sustain, now + state.synth.attack + state.synth.decay);
  filter.type = 'lowpass'; filter.Q.value = .65; filter.frequency.value = state.synth.cutoff; filter.connect(envelope); envelope.connect(synthBus);
  const gains = [], oscillators = [], weights = frameWeights(state.synth.position);
  for (let frame = 0; frame < 4; frame++) {
    const osc = context.createOscillator(), gain = context.createGain(); osc.setPeriodicWave(periodicWave(state.synth.bank, frame));
    osc.frequency.value = 440 * 2 ** ((note - 69) / 12); osc.detune.value = state.synth.tune * 100; gain.gain.value = weights[frame];
    osc.connect(gain); gain.connect(filter); osc.start(now); oscillators.push(osc); gains.push(gain);
  }
  const entry={ note, velocity, envelope, filter, oscillators, gains };
  activeNotes.set(note, entry); liveVoices.add(entry); keyButtons.get(note)?.classList.add('active');
  refreshVoiceDisplay();
}
function noteOff(note, immediate = false) {
  pendingNotes.delete(note);
  const entry = activeNotes.get(note); if (!entry || !context) return;
  const now = context.currentTime, release = immediate ? .012 : state.synth.release;
  entry.envelope.gain.cancelScheduledValues(now); entry.envelope.gain.setTargetAtTime(0, now, Math.max(.003, release / 5));
  entry.oscillators.forEach(osc => { try { osc.stop(now + release + .05); } catch {} });
  setTimeout(() => { entry.oscillators.forEach(osc => osc.disconnect()); entry.gains.forEach(g => g.disconnect()); entry.filter.disconnect(); entry.envelope.disconnect(); liveVoices.delete(entry); }, (release + .1) * 1000);
  activeNotes.delete(note); keyButtons.get(note)?.classList.remove('active'); refreshVoiceDisplay();
}
function panic() {
  pendingNotes.clear(); keyboardPointers.clear();
  [...activeNotes.keys()].forEach(note => noteOff(note, true));
  if(context) liveVoices.forEach(entry => {
    entry.envelope.gain.cancelScheduledValues(context.currentTime);
    entry.envelope.gain.setTargetAtTime(0,context.currentTime,.002);
    entry.oscillators.forEach(osc => { try { osc.stop(context.currentTime+.012); } catch {} });
  });
  announce('Synth voices cleared.');
}
function refreshLiveSynth() {
  if (!context) return;
  const now = context.currentTime, weights = frameWeights(state.synth.position);
  activeNotes.forEach(entry => {
    entry.envelope.gain.setTargetAtTime(entry.velocity * state.synth.level * .2 * state.synth.sustain, now, .015);
    entry.filter.frequency.setTargetAtTime(state.synth.cutoff, now, .015);
    entry.oscillators.forEach((osc, i) => { osc.detune.setTargetAtTime(state.synth.tune * 100, now, .015); osc.setPeriodicWave(periodicWave(state.synth.bank, i)); entry.gains[i].gain.setTargetAtTime(weights[i], now, .015); });
  });
}
function sourceFor(data, start, duration) {
  const buffer = context.createBuffer(1, data.length, context.sampleRate); buffer.copyToChannel(data, 0);
  const source = context.createBufferSource(); source.buffer = buffer;
  const envelope = context.createGain(); source.connect(envelope); envelope.connect(limiter);
  envelope.gain.setValueAtTime(0, start); envelope.gain.linearRampToValueAtTime(1, start + .0015);
  envelope.gain.setValueAtTime(1, Math.max(start + .0015, start + duration - .0015)); envelope.gain.linearRampToValueAtTime(0, start + duration);
  sources.add(source); source.addEventListener('ended', () => { sources.delete(source); source.disconnect(); envelope.disconnect(); });
  source.start(start); return source;
}
async function audition(tr,velocity=1) {
  try {
    await enableAudio(); output.gain.setTargetAtTime(state.volume, context.currentTime, .01);
    const sample = colorAudio(voice(TRACKS[tr].id, context.sampleRate), state.dust, context.sampleRate);
    for (let i = 0; i < sample.length; i++) sample[i] *= state.levels[tr]*velocity;
    sourceFor(sample, context.currentTime + .005, sample.length / context.sampleRate);
    flashTrack(tr);
  } catch (error) { announce(error.message || 'Sound could not start. Press PLAY again.'); }
}
function flashTrack(tr) { performance?.flash(tr); trackElements[tr].pad.classList.add('fired'); setTimeout(() => trackElements[tr].pad.classList.remove('fired'), 85); }
function schedule() {
  if (!playing) return;
  if (nextTime < context.currentTime - .05) { nextTime = context.currentTime + .06; sequenceAnchor = scheduledBars; }
  while (nextTime < context.currentTime + .14) {
    const settings = snapshot();
    const variation = settings.enabled && settings.auto ? Math.floor((scheduledBars - sequenceAnchor) / settings.auto) : 0;
    const seed = variationSeed(settings.seed, variation);
    const rendered = renderBar(settings.pattern, settings, seed, context.sampleRate);
    sourceFor(rendered.audio, nextTime, rendered.duration);
    events.push({ start: nextTime, end: nextTime + rendered.duration, bar: scheduledBars, revision, settings, rendered });
    nextTime += rendered.duration; scheduledBars++;
  }
  events = events.filter(e => e.end >= context.currentTime - 1);
}
async function play() {
  if (playing) { stop(); return; }
  if (starting) return;
  starting = true; const generation = ++startGeneration; $('play').disabled = true;
  try {
    await enableAudio();
    if (generation !== startGeneration) return;
    output.gain.cancelScheduledValues(context.currentTime); output.gain.setValueAtTime(state.volume, context.currentTime);
    playing = true; scheduledBars = 0; sequenceAnchor = 0; events = []; shownEvent = null; previousStep = -1; previousBar = -1;
    // Live keys stay audible throughout playback; only sequenced notes enter FLIP.
    synthBus.gain.setValueAtTime(1, context.currentTime);
    nextTime = context.currentTime + .07; schedule(); timer = setInterval(schedule, 25);
    $('play').classList.add('playing'); $('play').querySelector('span').textContent = 'PAUSE'; $('play').setAttribute('aria-label', 'Pause beat');
    $('sequence-status').textContent = 'PLAYING'; announce('Playing. Edits land at bar boundaries.'); tick();
  } catch (error) { announce(error.message || 'Sound could not start. Press PLAY again.'); }
  finally { starting = false; $('play').disabled = false; }
}
function stop() {
  ++startGeneration; playing = false; clearInterval(timer); cancelAnimationFrame(animation);
  if (context && output) {
    const now = context.currentTime;
    sources.forEach(source => { try { source.stop(now); } catch {} });
    output.gain.cancelScheduledValues(now); output.gain.setTargetAtTime(state.volume, now, .012);
    synthBus.gain.setTargetAtTime(1, now, .015);
  }
  events = []; shownEvent = null; previousStep = -1; previousBar = -1;
  stepButtons.flat().forEach(b => b.classList.remove('current')); mapCells.forEach(c => c.classList.remove('current'));
  synthSteps.forEach(b => b.classList.remove('current'));
  $('play').classList.remove('playing'); $('play').querySelector('span').textContent = 'PLAY'; $('play').setAttribute('aria-label', 'Play beat');
  $('position').innerHTML = '01 <small>/</small> 01'; $('sequence-status').textContent = 'SOURCE PATTERN';
  announce('Stopped. Your pattern is ready.'); performance?.refresh(); preview();
}
function currentEvent() { return context ? events.find(e => e.start <= context.currentTime && e.end > context.currentTime) : null; }
function tick() {
  if (!playing) return;
  performance?.refresh();
  const event = currentEvent();
  if (event) {
    if (shownEvent !== event) { shownEvent = event; showMonitor(event.rendered, event.settings.enabled); }
    const step = Math.min(15, Math.floor((context.currentTime - event.start) / event.rendered.duration * 16));
    if (step !== previousStep || event.bar !== previousBar) {
      stepButtons.forEach((row, tr) => { row.forEach((button, s) => button.classList.toggle('current', s === step)); if (event.settings.pattern[tr][step] && !event.settings.muted[tr]) flashTrack(tr); });
      mapCells.forEach((cell, s) => cell.classList.toggle('current', s === step));
      synthSteps.forEach((button,s) => button.classList.toggle('current',s===step && event.settings.synthSequenceEnabled && event.settings.synth.enabled));
      $('position').innerHTML = `${String(event.bar + 1).padStart(2, '0')} <small>/</small> ${String(step + 1).padStart(2, '0')}`;
      previousStep = step; previousBar = event.bar;
    }
    const pending = event.revision !== revision;
    $('sequence-status').textContent = pending ? 'CHANGES QUEUED' : 'PLAYING';
    $('flip-status').textContent = pending ? 'Changes land at a bar boundary.' : event.settings.enabled ? state.auto ? 'Auto Flip is evolving the pocket.' : 'This flip is playing.' : 'Your source beat stays intact.';
  }
  animation = requestAnimationFrame(tick);
}
function setEnabled(enabled) { state.enabled = enabled; refreshModes(); changed(true); }
function flip() {
  const seedBytes = new Uint32Array(1);
  if (window.crypto?.getRandomValues) window.crypto.getRandomValues(seedBytes); else seedBytes[0] = Math.floor(Math.random() * 4294967296);
  state.seed = seedBytes[0] % 1048575 + 1; state.enabled = true; refreshModes(); changed(true);
  if (!playing) preview();
  announce(playing ? 'New flip queued for a bar boundary.' : 'New flip ready. Press PLAY to hear it.');
}
function keep() {
  if (!state.enabled) return;
  const active = currentEvent();
  state.seed = active?.settings.enabled ? active.rendered.seed : state.seed;
  state.auto = 0; $('auto').value = '0'; changed(true);
  announce('Flip kept. Automatic changes are off.');
  if (!playing) preview();
}
$('play').addEventListener('click', play); $('stop').addEventListener('click', stop);
$('flip').addEventListener('click', flip); $('keep').addEventListener('click', keep);
$('original').addEventListener('click', () => setEnabled(false)); $('flipped').addEventListener('click', () => setEnabled(true));
$('groove').addEventListener('change', () => { state.pattern = groove($('groove').value); refreshTracks(); changed(); });
$('clear').addEventListener('click', () => { state.pattern = blankPattern(); $('groove').value = 'blank'; refreshTracks(); changed(); announce('Pattern cleared. Click steps to build a beat.'); });
$('tempo').addEventListener('change', () => { const value = Number($('tempo').value); state.tempo = Math.max(60, Math.min(180, Number.isFinite(value) && value > 0 ? value : 96)); $('tempo').value = state.tempo; performance?.refresh(); changed(); });
for (const id of ['swing', 'dust', 'volume', 'amount', 'mix', 'repeatSwing']) {
  const input = $(id); fillRange(input);
  input.addEventListener('input', () => { state[id] = Number(input.value) / 100; $(id + '-value').textContent = input.value + '%'; fillRange(input); if (id === 'volume') { if (output) output.gain.setTargetAtTime(state.volume, context.currentTime, .012); } else changed(); });
}
$('speed').addEventListener('change', () => { state.speed = Number($('speed').value); changed(); });
$('auto').addEventListener('change', () => { state.auto = Number($('auto').value); if (state.auto) state.enabled = true; refreshModes(); changed(true); });
$('protect').addEventListener('change', () => { state.protect = $('protect').checked; changed(); });
document.querySelectorAll('[data-effect]').forEach(button => button.addEventListener('click', () => {
  const effect = button.dataset.effect, included = state.effects.includes(effect);
  state.effects = included ? state.effects.filter(x => x !== effect) : [...state.effects, effect];
  button.setAttribute('aria-pressed', !included); changed();
}));

$('synth-power').addEventListener('click', () => {
  state.synth.enabled = !state.synth.enabled; $('synth-power').setAttribute('aria-pressed', state.synth.enabled);
  $('synth-power').lastChild.textContent = state.synth.enabled ? 'SYNTH ON' : 'SYNTH OFF';
  if (!state.synth.enabled) panic(); changed();
});
$('panic').addEventListener('click', () => { panic(); state.synthSequenceEnabled=false; refreshSynthSequence(); changed(); if (playing) { stop(); play(); } });
$('synth-seq-play').addEventListener('change', () => { state.synthSequenceEnabled=$('synth-seq-play').checked; changed(); });
$('synth-seq-preset').addEventListener('change', () => { state.synthPattern=synthPattern($('synth-seq-preset').value); state.synthSequenceEnabled=$('synth-seq-preset').value!=='blank'; refreshSynthSequence(); changed(); });
$('synth-seq-clear').addEventListener('click', () => { state.synthPattern=synthPattern('blank'); $('synth-seq-preset').value='blank'; refreshSynthSequence(); changed(); });
for (const [id,field,scale] of [['seq-note','note',1],['seq-chord','chord',1],['seq-velocity','velocity',.01],['seq-gate','gate',.01]]) {
  $(id).addEventListener('input', () => { state.synthPattern[selectedSynthStep][field]=Number($(id).value)*scale; $('synth-seq-preset').value='custom'; refreshSynthSequence(); changed(); });
}
$('synth-bank').addEventListener('change', () => { state.synth.bank = $('synth-bank').value; refreshLiveSynth(); refreshVoiceDisplay(); changed(); });
const synthRanges = {
  'wave-position': { key: 'position', read: v => v / 100, label: v => `${v}%` },
  'synth-level': { key: 'level', read: v => v / 100, label: v => `${v}%` },
  'synth-tune': { key: 'tune', read: v => v, label: v => `${v > 0 ? '+' : ''}${v} st` },
  'synth-cutoff': { key: 'cutoff', read: v => v, label: v => v >= 1000 ? `${(v / 1000).toFixed(1)}k` : `${v}Hz` },
  'synth-attack': { key: 'attack', read: v => v / 1000, label: v => `${v}ms` },
  'synth-decay': { key: 'decay', read: v => v / 1000, label: v => `${v}ms` },
  'synth-sustain': { key: 'sustain', read: v => v / 100, label: v => `${v}%` },
  'synth-release': { key: 'release', read: v => v / 1000, label: v => `${v}ms` }
};
Object.entries(synthRanges).forEach(([id, config]) => {
  const input = $(id); fillRange(input);
  input.addEventListener('input', () => { const value = Number(input.value); state.synth[config.key] = config.read(value); $(id + '-value').textContent = config.label(value); fillRange(input); refreshLiveSynth(); refreshVoiceDisplay(); changed(); });
});
$('midi').addEventListener('click', async () => {
  if (!navigator.requestMIDIAccess) { announce('Web MIDI is not available in this browser.'); return; }
  try {
    midiAccess = await navigator.requestMIDIAccess();
    const attach = () => midiAccess.inputs.forEach(input => { input.onmidimessage = event => { const [status, note, velocity] = event.data, command = status & 0xf0; if (command === 0x90 && velocity) noteOn(note, velocity / 127); else if (command === 0x80 || (command === 0x90 && !velocity)) noteOff(note); }; });
    attach(); midiAccess.onstatechange = attach; $('midi').classList.add('connected'); $('midi').textContent = 'MIDI READY'; announce(`${midiAccess.inputs.size} MIDI input${midiAccess.inputs.size === 1 ? '' : 's'} connected.`);
  } catch { announce('MIDI access was not enabled. The on-screen and computer keyboard still work.'); }
});
const typingKeys = new Map(Object.entries({ KeyA:48, KeyW:49, KeyS:50, KeyE:51, KeyD:52, KeyF:53, KeyT:54, KeyG:55, KeyY:56, KeyH:57, KeyU:58, KeyJ:59, KeyK:60 }));
document.addEventListener('keydown', e => {
  if (e.repeat || e.ctrlKey || e.metaKey || e.altKey || e.target.closest('input,select,textarea,[contenteditable]') || (e.target.closest('button') && !e.target.closest('.piano-key'))) return;
  if (typingKeys.has(e.code)) { e.preventDefault(); noteOn(typingKeys.get(e.code), .82); return; }
  if (e.code === 'Space') { e.preventDefault(); play(); }
});
document.addEventListener('keyup', e => { if (typingKeys.has(e.code)) { e.preventDefault(); noteOff(typingKeys.get(e.code)); } });
document.addEventListener('visibilitychange', () => { if (document.hidden) { panic(); if (playing) { stop(); announce('Playback paused while the tab is hidden. Press PLAY to continue.'); } } });
window.addEventListener('blur', panic);
window.addEventListener('pagehide', () => { if (playing) stop(); panic(); });
let performance=null;
performance=setupPerformance({state,tracks:TRACKS,noteOn,noteOff,audition,refreshTracks,refreshSynthSequence,changed,announce,isPlaying:()=>playing});
refreshTracks(); refreshModes(); refreshVoiceDisplay(); refreshSynthSequence(); preview();

// Tools share the same sequencer state and actions as the visible controls.
if (document.modelContext?.registerTool) {
  const lifecycle = new AbortController();
  const tools = [
    {
      name: 'read_drum_pattern', title: 'Read drum pattern', description: 'Read the eight drum rows, accents, playback settings, and current flip seed.',
      inputSchema: { type: 'object', properties: {}, additionalProperties: false }, annotations: { readOnlyHint: true },
      execute() { return { tracks: TRACKS.map((t, i) => ({ id: t.id, steps: [...state.pattern[i]], muted: state.muted[i] })), tempo: state.tempo, playing, flipped: state.enabled, seed: currentEvent()?.rendered.seed || state.seed }; }
    },
    {
      name: 'set_drum_steps', title: 'Set drum steps', description: 'Set hits for one drum voice. The 16 step values are 0 for off, 1 for a hit, and 2 for an accent. Playback changes at a bar boundary.',
      inputSchema: { type: 'object', properties: { track: { type: 'string', enum: TRACKS.map(t => t.id) }, steps: { type: 'array', items: { type: 'integer', minimum: 0, maximum: 2 }, minItems: 16, maxItems: 16 } }, required: ['track', 'steps'], additionalProperties: false },
      execute(input) { const tr = TRACKS.findIndex(t => t.id === input?.track); if (tr < 0 || !Array.isArray(input.steps) || input.steps.length !== 16 || input.steps.some(x => !Number.isInteger(x) || x < 0 || x > 2)) throw new Error('Choose a drum voice and exactly 16 step values from 0 to 2.'); state.pattern[tr] = [...input.steps]; $('groove').value = 'custom'; refreshTracks(); changed(); return { track: input.track, steps: [...state.pattern[tr]], queued: playing }; }
    },
    {
      name: 'generate_beat_flip', title: 'Flip programmed beat', description: 'Generate and enable a new tempo-synced glitch variation of the programmed drum bar. Does not start playback or change the source steps.',
      inputSchema: { type: 'object', properties: {}, additionalProperties: false },
      execute() { flip(); return { seed: state.seed, queued: playing, playing }; }
    }
  ];
  tools.forEach(tool => { try { Promise.resolve(document.modelContext.registerTool(tool, { signal: lifecycle.signal })).catch(() => {}); } catch {} });
  window.addEventListener('pagehide', () => lifecycle.abort(), { once: true });
}
