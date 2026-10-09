// Source patterns are independent from sound design and the live-note path.
// Sparse storage avoids allocating 396 complete sequences until they are edited.
export class PatternBanks {
  constructor() { this.group=0; this.pattern=1; this.slots=new Map(); }
  capture(state) {
    this.slots.set(`${this.group}/${this.pattern}`, {
      pattern:state.pattern.map(row=>[...row]), synthPattern:state.synthPattern.map(step=>({...step})),
      synthSequenceEnabled:state.synthSequenceEnabled
    });
  }
  select(group,pattern,state) {
    if(!Number.isInteger(group)||group<0||group>3||!Number.isInteger(pattern)||pattern<1||pattern>99) return false;
    if(group===this.group&&pattern===this.pattern) return false;
    this.capture(state); this.group=group; this.pattern=pattern;
    const saved=this.slots.get(`${group}/${pattern}`);
    state.pattern=saved?saved.pattern.map(row=>[...row]):Array.from({length:8},()=>Array(16).fill(0));
    state.synthPattern=saved?saved.synthPattern.map(step=>({...step})):Array.from({length:16},()=>({note:-1,chord:0,velocity:.8,gate:.65}));
    state.synthSequenceEnabled=saved?.synthSequenceEnabled??false; return true;
  }
}
export function padVelocity(y,height) { return Math.max(.15,Math.min(1,1-.85*y/Math.max(1,height))); }
export function setupPerformance({state,tracks,noteOn,noteOff,audition,refreshTracks,refreshSynthSequence,changed,announce,isPlaying}) {
  const $=id=>document.getElementById(id), banks=new PatternBanks(), pads=[], held=new Map(), flashes=new Map();
  const patternSelect=$('performance-pattern');
  for(let p=1;p<=99;p++) { const option=document.createElement('option');option.value=p;option.textContent=`PATTERN ${String(p).padStart(2,'0')}`;patternSelect.append(option); }
  function selectPattern(group,pattern) {
    if(!banks.select(group,pattern,state)) return;
    $('groove').value='custom';$('synth-seq-preset').value='custom';refreshTracks();refreshSynthSequence();changed();
    document.querySelectorAll('[data-group]').forEach(button=>button.setAttribute('aria-pressed',Number(button.dataset.group)===banks.group));
    announce(`Group ${'ABCD'[group]}, pattern ${String(pattern).padStart(2,'0')}${isPlaying()?' queued for the next bar.':' recalled.'}`);refresh();
  }
  document.querySelectorAll('[data-group]').forEach(button=>button.addEventListener('click',()=>selectPattern(Number(button.dataset.group),banks.pattern)));
  patternSelect.addEventListener('change',()=>selectPattern(banks.group,Number(patternSelect.value)));
  function setView(view) {
    document.querySelectorAll('[data-workspace]').forEach(panel=>{panel.hidden=panel.dataset.workspace!==view;});
    document.querySelectorAll('[data-view]').forEach(button=>button.setAttribute('aria-pressed',button.dataset.view===view));
  }
  document.querySelectorAll('[data-view]').forEach(button=>button.addEventListener('click',()=>setView(button.dataset.view)));
  function release(id) {
    const item=held.get(id);if(!item)return;
    if(item.note!==null) noteOff(item.note);held.delete(id);
    if(![...held.values()].some(x=>x.pad===item.pad)) pads[item.pad].classList.remove('held');
  }
  function press(id,pad,velocity) {
    if(held.has(id))return;
    const keys=$('pad-bank').value==='keys',note=keys?48+pad:null;
    if(keys) { if(!state.synth.enabled) $('synth-power').click();noteOn(note,velocity); }
    else audition(pad<8?pad:pad-8,velocity*(pad<8?1:.55));
    held.set(id,{pad,note});pads[pad].classList.add('held');$('lcd-pad').textContent=pads[pad].dataset.sound;
  }
  for(let pad=0;pad<12;pad++) {
    const button=document.createElement('button');button.className='performance-pad';button.innerHTML=`<strong>${String(pad+1).padStart(2,'0')}</strong><span></span><i aria-hidden="true"></i>`;
    button.addEventListener('click',event=>{if(event.detail===0){press(`assistive${pad}`,pad,.8);setTimeout(()=>release(`assistive${pad}`),100);}});
    button.addEventListener('pointerdown',event=>{
      if(event.button!==0)return;event.preventDefault();button.focus({preventScroll:true});button.setPointerCapture(event.pointerId);
      const rect=button.getBoundingClientRect();press(`pointer${event.pointerId}`,pad,padVelocity(event.clientY-rect.top,rect.height));
    });
    for(const type of ['pointerup','pointercancel','lostpointercapture']) button.addEventListener(type,event=>release(`pointer${event.pointerId}`));
    button.addEventListener('keydown',event=>{if(['Space','Enter'].includes(event.code)){event.preventDefault();press(`button${pad}`,pad,.8);}});
    button.addEventListener('keyup',event=>{if(['Space','Enter'].includes(event.code)){event.preventDefault();release(`button${pad}`);}});
    button.addEventListener('blur',()=>release(`button${pad}`));
    $('performance-pads').append(button);pads.push(button);
  }
  function refreshPads() {
    for(const id of [...held.keys()])release(id);
    const keys=$('pad-bank').value==='keys',notes=['C3','C♯3','D3','D♯3','E3','F3','F♯3','G3','G♯3','A3','A♯3','B3'];
    pads.forEach((button,i)=>{const sound=keys?notes[i]:tracks[i<8?i:i-8].name+(i>=8?' SOFT':'');button.dataset.sound=sound;button.querySelector('span').textContent=sound;button.setAttribute('aria-label',`Performance pad ${i+1}: ${sound}`);});
    $('lcd-pad').textContent=pads[0].dataset.sound;
  }
  $('pad-bank').addEventListener('change',refreshPads);refreshPads();
  const padCodes=['Digit1','Digit2','Digit3','Digit4','Digit5','Digit6','Digit7','Digit8','Digit9','Digit0','Minus','Equal'];
  document.addEventListener('keydown',event=>{
    const pad=padCodes.indexOf(event.code);if(pad<0||event.repeat||event.ctrlKey||event.metaKey||event.altKey||event.target.closest('input,select,textarea,[contenteditable]'))return;
    event.preventDefault();press(event.code,pad,.8);
  });
  document.addEventListener('keyup',event=>release(event.code));
  function releaseAll() {for(const id of [...held.keys()])release(id);}
  window.addEventListener('blur',releaseAll);window.addEventListener('pagehide',releaseAll);document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseAll();});
  $('panic').addEventListener('click',releaseAll);
  const fader=$('master-fader'),assignment=$('master-assignment');let gestureTarget=null;
  function refreshFader() {
    const target=$(assignment.value);if(gestureTarget)return;
    for(const key of ['min','max','step','value'])fader[key]=target[key];
    const label=$(assignment.value+'-value')?.textContent??target.value;$('master-value').textContent=label;$('lcd-fader').textContent=`FADER / ${assignment.selectedOptions[0].textContent.toUpperCase()} ${label}`;
  }
  // Pin the destination for the gesture, so changing assignment cannot split automation.
  fader.addEventListener('pointerdown',()=>{gestureTarget=assignment.value;assignment.disabled=true;});
  for(const type of ['pointerup','pointercancel','blur'])fader.addEventListener(type,()=>{gestureTarget=null;assignment.disabled=false;refreshFader();});
  fader.addEventListener('input',()=>{const target=$(gestureTarget||assignment.value);target.value=fader.value;target.dispatchEvent(new Event('input',{bubbles:true}));const label=$(target.id+'-value')?.textContent??target.value;$('master-value').textContent=label;$('lcd-fader').textContent=`FADER / ${assignment.selectedOptions[0].textContent.toUpperCase()} ${label}`;});
  assignment.addEventListener('change',refreshFader);
  document.querySelectorAll('.sound-strip input,.synth-controls input,#mix').forEach(input=>input.addEventListener('input',refreshFader));
  function refresh() {
    $('lcd-pattern').textContent=`${'ABCD'[banks.group]}.${String(banks.pattern).padStart(2,'0')}`;
    patternSelect.value=banks.pattern;
    $('lcd-tempo').innerHTML=`${Number(state.tempo).toFixed(1)}<small>BPM</small>`;
    $('lcd-status').textContent=isPlaying()?'PLAY · '+$('position').textContent.trim():'STOP · LIVE PADS READY';refreshFader();
  }
  function flash(track) {
    if($('pad-bank').value!=='drums')return;
    pads.forEach((button,i)=>{if((i<8?i:i-8)!==track)return;button.classList.add('fired');clearTimeout(flashes.get(i));flashes.set(i,setTimeout(()=>button.classList.remove('fired'),100));});
  }
  refresh();setView('drums');return {refresh,flash,releaseAll,banks,setView};
}
