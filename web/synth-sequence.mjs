export const CHORDS = ['Single', 'Major', 'Minor', 'Sus2', 'Octave'];
const intervals = [[0], [0,4,7], [0,3,7], [0,2,7], [0,12]];
export const noteName = note => note < 0 ? 'REST' : `${['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'][note % 12]}${Math.floor(note / 12) - 1}`;
export const chordNotes = (root, chord = 0) => Number.isInteger(root) && root >= 0 && root <= 127
  ? (intervals[chord] || intervals[0]).map(i => root + i).filter(note => note <= 127) : [];
export const blankSynthPattern = () => Array.from({length:16}, () => ({note:-1,chord:0,velocity:.8,gate:.65}));
export function synthPattern(name) {
  const pattern = blankSynthPattern();
  if (name === 'bass') [36,36,43,39,36,46,43,39].forEach((note,i) => pattern[i*2] = {note,chord:0,velocity:.82,gate:.65});
  if (name === 'chords') [48,56,51,58].forEach((note,i) => pattern[i*4] = {note,chord:i ? 1 : 2,velocity:i%2 ? .7 : .75,gate:1});
  return pattern;
}
export function synthSequenceEvents(pattern, tempo = 96, swing = 0) {
  const duration = 240 / Math.max(20, Math.min(400, tempo)), cell = duration / 16;
  const delay = Math.max(0, Math.min(.6, swing)) * .48;
  return (pattern || []).slice(0,16).flatMap((step,index) => {
    const start = (index + (index%2 ? delay : 0)) * cell;
    const width = cell * (index%2 ? 1-delay : 1+delay);
    const gate = Math.max(.1, Math.min(1, Number.isFinite(step.gate) ? step.gate : .65));
    const velocity = Math.max(.01, Math.min(1, Number.isFinite(step.velocity) ? step.velocity : .8));
    return chordNotes(step.note,step.chord).map(note => ({note,velocity,start,duration:width*gate,step:index}));
  });
}
