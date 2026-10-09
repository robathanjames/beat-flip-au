// Optional actual-browser regression check. Install Playwright, or supply its module
// and a Chromium executable using DUSTBOX_PLAYWRIGHT_MODULE / DUSTBOX_BROWSER_BIN.
import assert from 'node:assert/strict';
const { chromium } = await import(process.env.DUSTBOX_PLAYWRIGHT_MODULE || 'playwright');
const browser = await chromium.launch({executablePath:process.env.DUSTBOX_BROWSER_BIN || undefined,headless:true,args:['--no-sandbox','--allow-file-access-from-files','--autoplay-policy=no-user-gesture-required']});
try {
  for(const [name,viewport] of Object.entries({desktop:{width:1440,height:1100},mobile:{width:430,height:932}})) {
    const page=await browser.newPage({viewport}); const errors=[];
    page.on('pageerror',error=>errors.push(error.message));
    await page.addInitScript(() => {
      const NativeContext=window.AudioContext,connect=AudioNode.prototype.connect;
      window.AudioContext=class extends NativeContext {
        constructor(...args) { super(...args); window.dustboxTestAudio={context:this,analyser:this.createAnalyser()}; }
      };
      AudioNode.prototype.connect=function(destination,...args) {
        if(destination===this.context.destination) connect.call(this,window.dustboxTestAudio.analyser);
        return connect.call(this,destination,...args);
      };
    });
    await page.goto(new URL('./index.html',import.meta.url).href);
    await page.locator('#clear').click();
    await page.locator('[data-view=pattern]').click(); await page.locator('#synth-seq-clear').click();
    await page.locator('[data-view=sound]').click();
    await page.locator('#synth-attack').press('Home'); await page.locator('#synth-release').press('Home');
    await page.locator('[data-view=fx]').click(); await page.locator('#mix').press('End'); await page.locator('#flipped').click();
    await page.locator('[data-view=sound]').click();
    await page.locator('#play').click();
    const rms=()=>page.evaluate(() => {
      const a=window.dustboxTestAudio.analyser,samples=new Float32Array(a.fftSize); a.getFloatTimeDomainData(samples);
      return Math.sqrt(samples.reduce((sum,x)=>sum+x*x,0)/samples.length);
    });
    await page.waitForTimeout(300); assert.ok(await rms()<.0001,'Blank patterns should be silent');
    for(const key of ['a','d','g']) {
      await page.locator('.piano-key[data-note="48"]').focus();
      await page.keyboard.down(key); await page.waitForTimeout(120);
      assert.ok(await rms()>.003,`${name}: ${key} must sound immediately inside the first bar at 100% wet`);
      assert.ok((await page.locator('#position').textContent()).trim().startsWith('01'),'All three live notes must fit within the same first bar');
      await page.keyboard.up(key); await page.waitForTimeout(150);
      assert.ok(await rms()<.0001,'Note-off must release, not persist for a whole bar');
    }
    await page.locator('.piano-key[data-note="48"]').focus(); await page.keyboard.down('a'); await page.waitForTimeout(100);
    await page.locator('#stop').click(); await page.waitForTimeout(100);
    assert.ok(await rms()>.003,'Stopping the backing pattern must not mute a held live key');
    await page.keyboard.up('a'); await page.locator('#panic').click(); await page.waitForTimeout(150);
    assert.ok(await rms()<.0001,'Panic must silence live keys and release tails');
    await page.locator('[data-view=pattern]').click(); await page.locator('#synth-seq-preset').selectOption('chords');
    assert.equal(await page.locator('.synth-step.has-note').count(),4);
    await page.locator('.synth-step').nth(2).click(); await page.locator('#seq-note').selectOption('62'); await page.locator('#seq-chord').selectOption('2');
    assert.match(await page.locator('.synth-step').nth(2).getAttribute('aria-label'),/D4 Minor/);
    await page.locator('[data-view=fx]').click(); await page.locator('#original').click(); await page.locator('#play').click();
    let sequencePeak=0;
    for(let probe=0;probe<15;probe++) { await page.waitForTimeout(40); sequencePeak=Math.max(sequencePeak,await rms()); }
    assert.ok(sequencePeak>.003,'Programmed synth chords must play between their gates and rests');
    await page.locator('#stop').click(); await page.waitForTimeout(150); assert.ok(await rms()<.0001,'Stopping must silence programmed notes');
    // The performance surface must wire real pads, bank recall and fader targets.
    assert.equal(await page.locator('.performance-pad').count(),12);
    await page.locator('[data-view=drums]').click(); await page.locator('#clear').click();
    await page.locator('.step[data-track="0"][data-step="0"]').click();
    await page.locator('[data-group="1"]').click();
    assert.equal(await page.locator('.step[data-track="0"][data-step="0"]').getAttribute('data-value'),'0');
    await page.locator('#performance-pattern').selectOption('99');
    await page.locator('.step[data-track="1"][data-step="2"]').click();
    await page.locator('[data-group="0"]').click(); await page.locator('#performance-pattern').selectOption('1');
    assert.equal(await page.locator('.step[data-track="0"][data-step="0"]').getAttribute('data-value'),'1');
    await page.locator('[data-group="1"]').click(); await page.locator('#performance-pattern').selectOption('99');
    assert.equal(await page.locator('.step[data-track="1"][data-step="2"]').getAttribute('data-value'),'1');
    await page.locator('#master-assignment').selectOption('dust'); await page.locator('#master-fader').press('End');
    assert.equal(await page.locator('#dust-value').textContent(),'100%');
    await page.locator('#pad-bank').selectOption('keys');
    await page.locator('.performance-pad').first().focus(); await page.keyboard.down('Enter'); await page.waitForTimeout(120);
    assert.ok(await rms()>.003,'Performance keys must sound with transport stopped');
    await page.keyboard.up('Enter'); await page.waitForTimeout(150); assert.ok(await rms()<.0001,'Performance note must release on key-up');
    await page.locator('#pad-bank').selectOption('drums');
    const rail=await page.locator('.fader-rail').boundingBox();
    assert.ok(rail.height<=280,'The performance fader must not stretch the instrument vertically');
    const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>document.documentElement.clientWidth+1);
    assert.equal(overflow,false,'The page must not overflow horizontally'); assert.deepEqual(errors,[]);
    const path=process.env.DUSTBOX_SCREENSHOT_DIR;
    if(path) {
      await page.locator('[data-group=\"0\"]').click(); await page.locator('#performance-pattern').selectOption('1');
      await page.locator('#groove').selectOption('pocket');
      for(const [id,value] of [['synth-attack',30],['synth-release',450],['mix',80],['dust',34]]) {
        await page.locator(`#${id}`).evaluate((input,value)=> { input.value=value; input.dispatchEvent(new Event('input',{bubbles:true})); },value);
      }
      await page.locator('h1').click();
      await page.waitForTimeout(100);
      await page.locator('[data-view=drums]').click();
      await page.screenshot({path:`${path}/dustbox-web-${name}.png`,fullPage:true});
      if(name==='desktop') { await page.locator('[data-view=sound]').click(); await page.screenshot({path:`${path}/dustbox-web-sound.png`,fullPage:true}); }
    }
    console.log(`PASS ${name}: mid-bar live notes, note-off, stop/panic, sequence controls/audio, responsive layout`);
    await page.close();
  }
} finally { await browser.close(); }
