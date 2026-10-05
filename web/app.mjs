import { TRACKS, EFFECTS, EFFECT_LABELS, groove, blankPattern, makeFlip, renderBar, voice, colorAudio, variationSeed } from './engine.mjs';

const $ = id => document.getElementById(id);
const state = {
  pattern: groove('pocket'), tempo: 96, swing: .18, dust: .34, volume: .7,
  amount: .65, mix: .8, repeatSwing: 0, speed: 0, auto: 0, protect: true,
  effects: EFFECTS.slice(1), enabled: false, seed: 909,
  levels: [.95, .7, .55, .5, .45, .65, .6, .4], muted: Array(8).fill(false)
};
let context, output, limiter, playing = false, starting = false, startGeneration = 0;
let timer, animation, nextTime = 0, scheduledBars = 0, sequenceAnchor = 0, revision = 0;
let events = [], sources = new Set(), shownEvent = null, previousStep = -1, previousBar = -1;
let previewTimer;
const stepButtons = [], trackElements = [], mapCells = [];
const snapshot = () => ({ ...state, pattern: state.pattern.map(x => [...x]), effects: [...state.effects], levels: [...state.levels], muted: [...state.muted] });
const announce = message => { $('audio-message').textContent = message; };

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

function refreshStep(tr, s) {
  const value = state.pattern[tr][s], button = stepButtons[tr][s];
  button.dataset.value = value; button.setAttribute('aria-pressed', value ? 'true' : 'false');
  button.setAttribute('aria-label', `${TRACKS[tr].name}, step ${s + 1}: ${['off', 'hit', 'accent'][value]}`);
}
function refreshTracks() {
  TRACKS.forEach((track, tr) => { for (let s = 0; s < 16; s++) refreshStep(tr, s); trackElements[tr].row.classList.toggle('muted', state.muted[tr]); trackElements[tr].mute.setAttribute('aria-pressed', state.muted[tr]); });
}
function fillRange(input) { input.style.setProperty('--fill', `${(Number(input.value) - Number(input.min)) / (Number(input.max) - Number(input.min)) * 100}%`); }
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
  g.strokeStyle = '#39412e'; g.lineWidth = 1; g.beginPath(); g.moveTo(0, h / 2); g.lineTo(w, h / 2); g.stroke();
  g.strokeStyle = enabled ? '#ccff78' : '#879c6e'; g.lineWidth = 1.5; g.beginPath();
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
    limiter.connect(output); output.connect(context.destination);
    context.addEventListener('statechange', () => { if (playing && context.state !== 'running') { stop(); announce('Audio paused. Press PLAY to continue.'); } });
  }
  await context.resume();
  if (context.state !== 'running') throw new Error('Sound is paused. Press PLAY to try again.');
  $('power-led').parentElement.classList.add('active'); $('power-label').textContent = 'AUDIO READY';
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
async function audition(tr) {
  try {
    await enableAudio(); output.gain.setTargetAtTime(state.volume, context.currentTime, .01);
    const sample = colorAudio(voice(TRACKS[tr].id, context.sampleRate), state.dust, context.sampleRate);
    for (let i = 0; i < sample.length; i++) sample[i] *= state.levels[tr];
    sourceFor(sample, context.currentTime + .005, sample.length / context.sampleRate);
    flashTrack(tr);
  } catch (error) { announce(error.message || 'Sound could not start. Press PLAY again.'); }
}
function flashTrack(tr) { trackElements[tr].pad.classList.add('fired'); setTimeout(() => trackElements[tr].pad.classList.remove('fired'), 85); }
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
    nextTime = context.currentTime + .07; schedule(); timer = setInterval(schedule, 25);
    $('play').classList.add('playing'); $('play').querySelector('span').textContent = 'PAUSE'; $('play').setAttribute('aria-label', 'Pause beat');
    $('sequence-status').textContent = 'PLAYING'; announce('Playing. Edits land at bar boundaries.'); tick();
  } catch (error) { announce(error.message || 'Sound could not start. Press PLAY again.'); }
  finally { starting = false; $('play').disabled = false; }
}
function stop() {
  ++startGeneration; playing = false; clearInterval(timer); cancelAnimationFrame(animation);
  if (context && output) {
    const now = context.currentTime; output.gain.cancelScheduledValues(now); output.gain.setTargetAtTime(0, now, .003);
    sources.forEach(source => { try { source.stop(now + .025); } catch {} });
  }
  events = []; shownEvent = null; previousStep = -1; previousBar = -1;
  stepButtons.flat().forEach(b => b.classList.remove('current')); mapCells.forEach(c => c.classList.remove('current'));
  $('play').classList.remove('playing'); $('play').querySelector('span').textContent = 'PLAY'; $('play').setAttribute('aria-label', 'Play beat');
  $('position').innerHTML = '01 <small>/</small> 01'; $('sequence-status').textContent = 'SOURCE PATTERN';
  announce('Stopped. Your pattern is ready.'); preview();
}
function currentEvent() { return context ? events.find(e => e.start <= context.currentTime && e.end > context.currentTime) : null; }
function tick() {
  if (!playing) return;
  const event = currentEvent();
  if (event) {
    if (shownEvent !== event) { shownEvent = event; showMonitor(event.rendered, event.settings.enabled); }
    const step = Math.min(15, Math.floor((context.currentTime - event.start) / event.rendered.duration * 16));
    if (step !== previousStep || event.bar !== previousBar) {
      stepButtons.forEach((row, tr) => { row.forEach((button, s) => button.classList.toggle('current', s === step)); if (event.settings.pattern[tr][step] && !event.settings.muted[tr]) flashTrack(tr); });
      mapCells.forEach((cell, s) => cell.classList.toggle('current', s === step));
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
$('tempo').addEventListener('change', () => { const value = Number($('tempo').value); state.tempo = Math.max(60, Math.min(180, Number.isFinite(value) && value > 0 ? value : 96)); $('tempo').value = state.tempo; changed(); });
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
document.addEventListener('keydown', e => {
  if (e.code !== 'Space' || e.repeat || e.ctrlKey || e.metaKey || e.altKey || e.target.closest('input,select,button,textarea,[contenteditable]')) return;
  e.preventDefault(); play();
});
document.addEventListener('visibilitychange', () => { if (document.hidden && playing) { stop(); announce('Playback paused while the tab is hidden. Press PLAY to continue.'); } });
window.addEventListener('pagehide', () => { if (playing) stop(); });
refreshTracks(); refreshModes(); preview();

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
