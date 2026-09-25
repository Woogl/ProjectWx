// Copyright Woogle. All Rights Reserved.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../..');
const read = file => fs.readFileSync(path.join(root, file), 'utf8');
const wiki = read('Saved/Wiki/knowledge.html');
const workflow = read('Saved/Wiki/index.html');
const payload = html => JSON.parse(html.match(/<script id="wiki-data" type="application\/json">([\s\S]*?)<\/script>/)[1]);
const data = payload(wiki);
assert.equal(data.mode, 'Wiki');
for (const page of [wiki, workflow]) {
  assert.ok(!Object.hasOwn(payload(page), 'sources'), 'viewers must not bundle source files');
}
assert.equal(data.ai, null);
assert.equal(payload(workflow).mode, 'Workflow');
assert.ok(wiki.includes("connect-src 'none'"));
const script = wiki.match(/<script>\s*([\s\S]*?)<\/script>/)[1];
assert.ok(!script.includes('localStorage'));
assert.ok(!script.includes('workflowRequest'));
const nodes = new Map();
function element(tag) {
  return { tagName: tag.toUpperCase(), children: [], style: {}, dataset: {}, value: '', classList: { toggle() {} }, append(...children) { this.children.push(...children); }, replaceChildren(...children) { this.children = children; }, addEventListener() {}, focus() {} };
}
function byId(id) { if (!nodes.has(id)) nodes.set(id, element('div')); return nodes.get(id); }
byId('wiki-data').textContent = JSON.stringify(data);
const context = vm.createContext({
  document: { getElementById: byId, createElement: element, querySelectorAll: () => [], addEventListener() {} },
  location: { hash: '' }, window: { addEventListener() {}, scrollTo() {} },
});
vm.runInContext(script, context);
assert.equal(byId('dashboard').hidden, true);
assert.equal(byId('search-page').hidden, false);
assert.equal(byId('sidebar').hidden, true);
assert.equal(byId('page-header').hidden, true);
assert.equal(byId('page-footer').hidden, true);
assert.equal(byId('nav').children.length, 0);
assert.equal(byId('cards').children.length, 0, 'empty search must not flood the home with documents');
byId('search').value='WxCombat';
vm.runInContext('renderCards()', context);
assert.ok(byId('cards').children.length > 0);
assert.ok(byId('cards').children.some(card => decodeURIComponent(card.href).includes('/wiki/topics/combat.md')));
assert.ok(data.documents.every(d => !d.path.startsWith('.wiki/raw/')), 'imported legacy source text must not appear as current knowledge');
assert.ok(data.documents.every(d => !d.path.startsWith('.wiki/.sessions/')), 'personal runtime state must not be bundled');
assert.ok(byId('cards').children.every(card => !decodeURIComponent(card.href).includes('/workflow/tasks/')));
byId('search').value='NO_MATCH_938475';
vm.runInContext('renderCards()', context);
assert.match(byId('count').textContent, /0개/);
byId('search').value='';
assert.ok(!wiki.includes('id="stage"'), 'search must not offer a category filter');
assert.ok(data.documents.every(d=>!Object.hasOwn(d,'stages')), 'search data must not carry stage classifications');
byId('search').value=data.documents.find(d=>d.path==='.agents/workflow/process/index.md').title;
vm.runInContext('renderCards()', context);
assert.ok(byId('cards').children.some(card=>decodeURIComponent(card.href)==='index.html#.agents/workflow/process/index.md'));
byId('search').value='';
vm.runInContext('renderCards()', context);
assert.equal(byId('cards').children.length, 0);
byId('search').value='WxCombat';
context.location.hash='';vm.runInContext('readRoute()', context);
assert.equal(byId('search').value,'WxCombat', 'returning from a document retains the query');
assert.equal(byId('other-space').href, 'index.html');
assert.equal(vm.runInContext("route('.agents/workflow/process/index.md')", context), 'index.html#' + encodeURIComponent('.agents/workflow/process/index.md'));
assert.ok(!read('BatchFiles/OpenWiki.bat').includes('Start-WikiAI.ps1'));
assert.ok(read('BatchFiles/OpenWorkflow.bat').includes('Start-WikiAI.ps1'));
assert.ok(read('BatchFiles/OpenWorkflow.bat').includes('-View Workflow -Open'));
assert.ok(workflow.includes("const workflowKey = 'wx-wiki-workflow-v1:' + location.pathname"));
console.log('PASS Wiki search home, no AI token/network/storage, separate launcher/menu, Workflow routing and preserved storage key');
