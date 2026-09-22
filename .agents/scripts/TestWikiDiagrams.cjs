// Copyright Woogle. All Rights Reserved.
const assert = require('node:assert/strict');
const path = require('node:path');
const {pathToFileURL} = require('node:url');
const {chromium} = require(process.env.WX_WIKI_PLAYWRIGHT || 'playwright');
const root = path.resolve(__dirname, '../..');
(async () => {
  const browser = await chromium.launch(process.env.WX_WIKI_BROWSER ? {executablePath:process.env.WX_WIKI_BROWSER} : {channel:'msedge'});
  try {
    const page = await browser.newPage({viewport:{width:1440,height:1100}});
    const external = [], errors = [];
    page.on('request', request => {if(/^https?:/.test(request.url())) external.push(request.url());});
    page.on('pageerror', error => errors.push(error.message));
    const url = pathToFileURL(path.join(root,'Saved/Wiki/knowledge.html')).href;
    await page.goto(url);
    const diagrams = ['combat/index','combat/damage','combat/groggy','combat/finisher','tooling/wiki-workflow'];
    for(const name of diagrams) {
      await page.evaluate(name => {location.hash=encodeURIComponent('.agents/wiki/knowledge/'+name+'.md');}, name);
      await page.waitForFunction(() => document.querySelector('.diagram-error') || (document.querySelectorAll('.wiki-diagram').length > 0 && [...document.querySelectorAll('.wiki-diagram')].every(node => {const img=node.querySelector('img');return img?.complete && img.naturalWidth>0;})));
      assert.equal(await page.locator('.diagram-error').count(), 0, name);
      assert.equal(await page.locator('.wiki-diagram').count(), 1, name);
      for(const diagram of await page.locator('.wiki-diagram').all()) {
        assert.ok(await diagram.locator('img').evaluate(img=>img.naturalWidth>0 && img.naturalHeight>0),name);
        assert.ok((await diagram.locator('details code').textContent()).length>20);
      }
      if(name==='combat/damage' || name==='combat/groggy' || name==='tooling/wiki-workflow') {
        await page.locator('.wiki-diagram').screenshot({path:path.join(root,'Saved/Wiki/'+(name==='combat/damage'?'damage':name==='combat/groggy'?'groggy':'workflow')+'-diagram.png')});
      }
    }
    assert.deepEqual(external, [], 'offline diagrams must not make network requests');
    assert.deepEqual(errors, [], 'valid diagrams must not produce page errors');
    for(const source of ['flowchart TD\n A[broken', '%%{init: {securityLevel: "loose"}}%%\nflowchart TD\nA-->B']) {
      await page.evaluate(async source => {
        const pre=document.createElement('pre'), code=document.createElement('code');
        code.className='language-mermaid';code.textContent=source;pre.append(code);
        document.getElementById('article').replaceChildren(pre);
        await renderWikiDiagrams();
      },source);
      assert.equal(await page.locator('.diagram-error').count(),1);
      assert.equal(await page.locator('.wiki-diagram details').evaluate(e=>e.open),true);
      assert.equal(await page.locator('.wiki-diagram code').textContent(),source);
    }
    await page.evaluate(async () => {
      const original=mermaid.render;
      let finish;
      mermaid.render=()=>new Promise(resolve=>{finish=resolve;});
      const pre=document.createElement('pre'),code=document.createElement('code');
      code.className='language-mermaid';code.textContent='flowchart TD\nA-->B';pre.append(code);
      const article=document.getElementById('article');article.replaceChildren(pre);
      const pending=renderWikiDiagrams();article.replaceChildren(document.createTextNode('새 문서'));
      finish({svg:'<svg xmlns="http://www.w3.org/2000/svg"></svg>'});await pending;
      mermaid.render=original;
    });
    assert.equal(await page.locator('#article').textContent(),'새 문서');
    console.log('PASS five official Mermaid diagrams in Edge, offline rendering, source fallback, configuration rejection and stale navigation isolation');
  } finally {await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
