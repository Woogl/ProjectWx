// Copyright Woogle. All Rights Reserved.
// Tests generated data and UI logic without launching a browser or requiring npm packages.
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'Saved/Wiki/index.html'), 'utf8');
const payload = html.match(/<script id="wiki-data" type="application\/json">([\s\S]*?)<\/script>/)[1];
const data = JSON.parse(payload);
const script = html.match(/<script>\s*([\s\S]*?)<\/script>/)[1];
new vm.Script(script);
const manifest = JSON.parse(fs.readFileSync(path.join(root, '.agents/wiki/sources.json'), 'utf8'));
assert.equal(new Set(data.documents.map(d => d.path)).size, data.documents.length);
for (const d of data.documents) {
  assert.equal(d.text, fs.readFileSync(path.join(root, d.path), 'utf8'));
  const meta = manifest.pages[d.path.replace('.agents/wiki/', '')];
  if (meta) assert.equal(d.status, meta.status);
  assert.ok(d.html.length > 0);
}
class Element {
  constructor(tag) { this.tagName = tag.toUpperCase(); this.children = []; this.value = ''; this.dataset = {}; this.style = {}; this.classList = { toggle() {} }; this.handlers = {}; }
  append(...nodes) { this.children.push(...nodes); }
  replaceChildren(...nodes) { this.children = nodes; }
  addEventListener(name, handler) { this.handlers[name] = handler; }
  focus() {}
}
const elements = new Map();
function byId(id) { if (!elements.has(id)) elements.set(id, new Element('div')); return elements.get(id); }
byId('wiki-data').textContent = payload;
const context = vm.createContext({
  document: { getElementById: byId, createElement: tag => new Element(tag), querySelectorAll: () => byId('nav').children, addEventListener() {} },
  location: { hash: '' }, window: { addEventListener() {}, scrollTo() {} },
});
vm.runInContext(script, context);
assert.equal(byId('cards').children.length, data.documents.length);
byId('search').value = 'WxCombat';
byId('search').handlers.input();
assert.ok(byId('cards').children.length > 0);
assert.ok(byId('cards').children.every(card => decodeURIComponent(card.href).startsWith('#.agents/')));
byId('search').value = 'NO_MATCH_78302914';
byId('search').handlers.input();
assert.match(byId('count').textContent, /0개/);
byId('search').value = '';
byId('status').value = 'blocked';
byId('status').handlers.change();
assert.equal(byId('cards').children.length, data.documents.filter(d => d.status === 'blocked').length);
byId('status').value = '';
byId('nav').children.find(n => n.dataset.category === 'modules').onclick();
assert.equal(byId('cards').children.length, 9);
assert.equal(vm.runInContext("resolvePath('.agents/wiki/modules/WxAI.md', '../systems/groggy.md')", context), '.agents/wiki/systems/groggy.md');
assert.equal(vm.runInContext("resolvePath('.agents/wiki/index.md', '../reports/index.md')", context), '.agents/reports/index.md');
assert.equal(vm.runInContext("route('.agents/wiki/한글 문서.md','절 제목')", context), '#' + encodeURIComponent('.agents/wiki/한글 문서.md') + '!' + encodeURIComponent('절 제목'));
context.location.hash = '#missing-document';
vm.runInContext('readRoute()', context);
assert.equal(byId('article').children[0].textContent, '문서를 찾을 수 없습니다.');
console.log(`PASS ${data.documents.length} document snapshots, metadata, JS syntax, search, status/category filters, routing and missing-document handling`);
