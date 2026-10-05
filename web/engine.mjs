export const TRACKS = [
  { id: 'kick', name: 'Kick', note: 'LOW / ROUND', color: '#f2a65a' },
  { id: 'snare', name: 'Snare', note: 'BODY / SNAP', color: '#eecb65' },
  { id: 'clap', name: 'Clap', note: 'LOOSE / WIDE', color: '#ed8c78' },
  { id: 'closedHat', name: 'Closed hat', note: 'SHORT / METAL', color: '#91d6bd' },
  { id: 'openHat', name: 'Open hat', note: 'AIR / SIZZLE', color: '#75bebb' },
  { id: 'tom', name: 'Low tom', note: 'DEEP / TUNED', color: '#a9a5d9' },
  { id: 'rim', name: 'Rim', note: 'WOOD / CLICK', color: '#d09ab8' },
  { id: 'ride', name: 'Ride', note: 'BELL / TAIL', color: '#a8bccd' }
];
export const EFFECTS = ['live', 'stutter', 'reverse', 'shuffle', 'gate', 'half'];
export const EFFECT_LABELS = ['LIVE', 'STUT', 'REV', 'JUMP', 'GATE', 'HALF'];
export const SAMPLE_RATE = 44100;
export const blankPattern = () => TRACKS.map(() => Array(16).fill(0));
export const clonePattern = p => p.map(row => [...row]);
export function groove(name) {
  const p = blankPattern();
  const set = (track, hits, accents = []) => { hits.forEach(i => p[track][i] = 1); accents.forEach(i => p[track][i] = 2); };
  if (name === 'pocket') {
    set(0, [0, 6, 8, 14], [0, 8]); set(1, [4, 12], [4, 12]); set(2, [12]);
    set(3, [0, 2, 4, 6, 8, 10, 12, 14], [2, 10]); set(4, [7, 15]); set(6, [3, 11]);
  } else if (name === 'warehouse') {
    set(0, [0, 4, 8, 12], [0, 8]); set(1, [4, 12]); set(2, [4, 12], [12]);
    set(3, [0, 2, 4, 6, 8, 10, 12, 14]); set(4, [2, 6, 10, 14]); set(7, [0, 8]);
  } else if (name === 'broken') {
    set(0, [0, 3, 6, 10, 14], [0, 10]); set(1, [4, 7, 12, 15], [4, 12]);
    set(3, [0, 1, 2, 4, 6, 7, 8, 9, 10, 12, 14, 15], [2, 6, 10, 14]);
    set(4, [11]); set(5, [13]); set(6, [5, 11]);
  }
  return p;
}
export function random(seed) {
  let x = seed >>> 0 || 1;
  return () => { x ^= x << 13; x ^= x >>> 17; x ^= x << 5; return (x >>> 0) / 4294967296; };
}
export function variationSeed(seed, variation) { return ((seed - 1 + variation * 7919) % 1048575) + 1; }
export function makeFlip(seed, settings) {
  const rng = random(seed);
  const weighted = [1, 1, 1, 2, 2, 3, 3, 4, 4, 5].filter(x => settings.effects.includes(EFFECTS[x]));
  return Array.from({ length: 16 }, (_, i) => {
    const density = rng(), chosen = rng(), source = rng(), repeats = rng();
    const effect = (i === 0 && settings.protect) || density > settings.amount || !weighted.length ? 0 : weighted[Math.floor(chosen * weighted.length)];
    return { effect, source: Math.floor(source * 16), repeats: settings.speed || [2, 4, 8][Math.floor(repeats * 3)] };
  });
}
export function voice(id, sr = SAMPLE_RATE) {
  const duration = ({ kick: .65, snare: .38, clap: .32, closedHat: .095, openHat: .52, tom: .55, rim: .085, ride: .85 })[id];
  if (!duration) throw new Error('Unknown drum voice');
  const out = new Float32Array(Math.ceil(duration * sr));
  const rng = random(TRACKS.findIndex(x => x.id === id) * 773 + 909);
  let phase = 0, noiseLow = 0, metallicLow = 0;
  const freqs = [205, 369, 522, 801, 1139, 1567];
  for (let i = 0; i < out.length; i++) {
    const t = i / sr, noise = rng() * 2 - 1;
    noiseLow += .22 * (noise - noiseLow);
    const bright = noise - noiseLow;
    let sample = 0;
    if (id === 'kick') {
      phase += 2 * Math.PI * (49 + 155 * Math.exp(-t * 55)) / sr;
      sample = Math.sin(phase) * Math.exp(-t * 8) * .94 + bright * Math.exp(-t * 190) * .17;
    } else if (id === 'snare') {
      sample = (Math.sin(2 * Math.PI * 185 * t) * .22 + Math.sin(2 * Math.PI * 330 * t) * .12) * Math.exp(-t * 22)
        + bright * .82 * Math.exp(-t * 16);
    } else if (id === 'clap') {
      const envelope = [0, .009, .020, .031].reduce((sum, delay, index) => sum + (t >= delay ? Math.exp(-(t - delay) * (index === 3 ? 22 : 170)) * (index === 3 ? .55 : .3) : 0), 0);
      sample = bright * envelope * .95;
    } else if (id === 'closedHat' || id === 'openHat' || id === 'ride') {
      const metal = freqs.reduce((sum, f) => sum + Math.sign(Math.sin(2 * Math.PI * f * (id === 'ride' ? 1.57 : 1) * t)), 0) / 6;
      metallicLow += .18 * (metal - metallicLow);
      sample = ((metal - metallicLow) * .48 + bright * .19) * Math.exp(-t * (id === 'closedHat' ? 65 : id === 'openHat' ? 10 : 6));
      if (id === 'ride') sample += Math.sin(2 * Math.PI * 2437 * t) * Math.exp(-t * 11) * .16;
    } else if (id === 'tom') {
      phase += 2 * Math.PI * (91 + 81 * Math.exp(-t * 34)) / sr;
      sample = Math.sin(phase) * Math.exp(-t * 10) * .72 + bright * Math.exp(-t * 110) * .12;
    } else if (id === 'rim') {
      sample = (Math.sin(2 * Math.PI * 480 * t) * .55 + Math.sin(2 * Math.PI * 1720 * t) * .35 + bright * .13) * Math.exp(-t * 75);
    }
    const attack = Math.min(1, t * 1400);
    const release = Math.min(1, (duration - t) * 350);
    out[i] = Math.tanh(sample) * attack * release;
  }
  return out;
}
const bankCache = new Map();
export function bank(sr = SAMPLE_RATE) {
  if (!bankCache.has(sr)) bankCache.set(sr, TRACKS.map(x => voice(x.id, sr)));
  return bankCache.get(sr);
}
export function barLength(tempo, sr = SAMPLE_RATE) { return Math.round(sr * 240 / tempo); }
export function renderDry(pattern, settings, sr = SAMPLE_RATE) {
  const count = barLength(settings.tempo, sr), out = new Float32Array(count), samples = bank(sr);
  const step = count / 16;
  for (let tr = 0; tr < 8; tr++) {
    if (settings.muted[tr]) continue;
    for (let s = 0; s < 16; s++) {
      const hit = pattern[tr][s];
      if (!hit) continue;
      const at = Math.round((s + (s % 2 ? settings.swing * .48 : 0)) * step);
      const amp = (hit === 2 ? 1 : .68) * settings.levels[tr];
      let limit = samples[tr].length;
      if (tr === 4) {
        for (let next = 1; next <= 16; next++) {
          const closed = (s + next) % 16;
          if (pattern[3][closed] && !settings.muted[3]) {
            const chokeAt = Math.round((s + next + (closed % 2 ? settings.swing * .48 : 0)) * step);
            limit = Math.min(limit, Math.max(0, chokeAt - at)); break;
          }
        }
      }
      for (let j = 0; j < limit; j++) {
        const dest = (at + j) % count;
        const chokeFade = Math.min(1, (limit - j) / (sr * .002));
        out[dest] += samples[tr][j] * amp * chokeFade;
      }
    }
  }
  return colorAudio(out, settings.dust, sr);
}
export function colorAudio(input, dust = 0, sr = SAMPLE_RATE) {
  const out = new Float32Array(input.length);
  const alpha = 1 - Math.exp(-2 * Math.PI * (14000 - dust * 10500) / sr);
  const steps = 2 ** (15 - Math.round(dust * 7)), hold = 1 + Math.floor(dust * 3);
  let low = 0, held = 0;
  for (let i = 0; i < input.length; i++) {
    low += alpha * (input[i] - low);
    if (i % hold === 0) held = Math.round(Math.tanh(low * (1 + dust * 1.6)) * steps) / steps;
    out[i] = held * .8;
  }
  return out;
}
const interpolated = (data, index) => {
  const n = data.length, a = Math.floor(index), fraction = index - a;
  return data[((a % n) + n) % n] * (1 - fraction) + data[(((a + 1) % n) + n) % n] * fraction;
};
export function renderFlip(dry, flip, settings, sr = SAMPLE_RATE) {
  if (!settings.enabled || settings.mix === 0 || settings.amount === 0 || flip.every(cell => cell.effect === 0)) return new Float32Array(dry);
  const out = new Float32Array(dry.length), count = dry.length, cell = count / 16;
  const fade = Math.min(sr * .0015, cell / 12);
  const wetSample = (index, local) => {
    const f = flip[index], start = index * cell;
    if (f.effect === 0) return interpolated(dry, start + local);
    if (f.effect === 2) return interpolated(dry, start + cell - 1 - local);
    if (f.effect === 3) return interpolated(dry, f.source * cell + local);
    if (f.effect === 5) return interpolated(dry, start + local * .5);
    const pulse = cell / f.repeats, pair = pulse * 2, swing = settings.repeatSwing * pulse;
    const pairLocal = local % pair;
    const long = pulse + swing;
    const cursor = pairLocal < long ? pairLocal : pairLocal - long;
    const length = pairLocal < long ? long : pulse - swing;
    if (f.effect === 1) {
      const edge = Math.min(1, cursor / fade, Math.max(0, length - cursor) / fade);
      return interpolated(dry, start + cursor) * edge;
    }
    return interpolated(dry, start + local) * Math.max(0, Math.min(1, cursor / fade, (length * .53 - cursor) / fade));
  };
  for (let i = 0; i < count; i++) {
    const index = Math.min(15, Math.floor(i / cell)), local = i - index * cell;
    let wet = wetSample(index, local);
    const previous = (index + 15) % 16;
    if (local < fade && (flip[index].effect !== 0 || flip[previous].effect !== 0)) {
      const old = wetSample(previous, cell - fade + local);
      wet = old + (wet - old) * local / fade;
    }
    out[i] = dry[i] + (wet - dry[i]) * settings.mix;
  }
  return out;
}
export function renderBar(pattern, settings, seed, sr = SAMPLE_RATE) {
  const flip = makeFlip(seed, settings), dry = renderDry(pattern, settings, sr);
  return { dry, audio: renderFlip(dry, flip, settings, sr), flip, seed, duration: dry.length / sr };
}
