// Copyright Woogle. All Rights Reserved.
// Tests generated data and UI logic without launching a browser or requiring npm packages.
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '../..');
const html = fs.readFileSync(path.join(root, 'Saved/Wiki/index.html'), 'utf8');
const payload = html.match(/<script id="wiki-data" type="application\/json">([\s\S]*?)<\/script>/)[1];
const data = JSON.parse(payload);
const script = html.match(/<script>\s*([\s\S]*?)<\/script>/)[1];
new vm.Script(script);
const expectedNavigation = [];
for (const line of fs.readFileSync(path.join(root, '.agents/workflow/index.md'), 'utf8').split(/\r?\n/)) {
  if (line === '## 한줄 요약') continue;
  const heading = line.match(/^## (.+)$/);
  const link = line.match(/^\s*(?:-|\d+\.) \[([^\]]+)\]\(([^)]+)\)$/);
  if (heading) expectedNavigation.push({ title: heading[1], items: [] });
  else if (link && expectedNavigation.length) expectedNavigation.at(-1).items.push({ title: link[1], path: path.posix.normalize('.agents/workflow/' + link[2]) });
}
assert.deepEqual(data.navigation, expectedNavigation, 'navigation must follow the current Wiki index');
assert.equal(new Set(data.documents.map(d => d.path)).size, data.documents.length);
for (const d of data.documents) {
  assert.equal(d.text, fs.readFileSync(path.join(root, d.path), 'utf8'));
  const frontmatter = d.text.match(/^---\r?\n([\s\S]*?)\r?\n---(?:\r?\n|$)/);
  assert.equal(d.frontmatter, frontmatter?.[1] || '');
  assert.ok(!d.html.includes('<p>category:'), 'YAML must not render as article prose');
  assert.ok(!d.html.includes('[['), 'paired Obsidian links must render once');
  assert.ok(!Object.hasOwn(d, 'status'), 'reader must not invent freshness from a separate manifest');
  assert.ok(d.html.length > 0);
}
class Element {
  constructor(tag) { this.tagName = tag.toUpperCase(); this.children = []; this.value = ''; this.dataset = {}; this.style = {}; this.classList = { toggle() {} }; this.handlers = {}; }
  append(...nodes) { this.children.push(...nodes); }
  replaceChildren(...nodes) { this.children = nodes; }
  addEventListener(name, handler) { this.handlers[name] = handler; }
  focus() {}
  setAttribute(name, value) { (this.attributes ||= {})[name] = value; }
}
const elements = new Map();
function byId(id) { if (!elements.has(id)) elements.set(id, new Element('div')); return elements.get(id); }
byId('wiki-data').textContent = payload;
const context = vm.createContext({
  document: { getElementById: byId, createElement: tag => new Element(tag), querySelectorAll: selector => {
    assert.equal(selector, '#nav a');
    return byId('nav').children.flatMap(group => group.children.filter(n => n.tagName === 'A'));
  }, addEventListener() {} },
  location: { hash: '' }, window: { addEventListener() {}, scrollTo() {} },
});
vm.runInContext(script, context);
// Markdown images are not shown; generated diagrams are the only data: images.
let removedImage = false;
const imageNode = { tagName: 'IMG', remove: () => { removedImage = true; } };
context.DOMParser = class { parseFromString() { return { body: { querySelectorAll: () => [imageNode] } }; } };
vm.runInContext("safeFragment('')", context);
assert.ok(removedImage, 'markdown images must be removed');
assert.ok(html.includes('img-src data:'), 'diagram images stay allowed');
assert.ok(html.includes('.wiki-diagram img{display:block;'), 'diagram images stay centered');
console.log('PASS markdown image removal and diagram image policy');
// Work records are listed from their own state lines without the local server, including records without a state.
assert.equal(vm.runInContext("taskRecordGroups().map(g=>g.title).join(',')", context), '확인 대기,진행 중,완료,리뷰·참고');
const indexedPaths = Array.from(vm.runInContext('taskRecordGroups().flatMap(g=>g.items.map(item=>item.path))', context));
const taskPaths = data.documents.map(d => d.path).filter(p => /^\.agents\/workflow\/tasks\/[^/]+\.md$/.test(p));
assert.deepEqual(indexedPaths.slice().sort(), taskPaths.sort(), 'every Markdown task must be listed exactly once');
assert.ok(byId('task-records').children.length > 0, 'records are available before the local server connects');
const recordFilters = () => byId('task-records').children.find(n => n.className === 'record-filters').children;
assert.equal(recordFilters().length, 4);
recordFilters()[2].onclick();
assert.equal(vm.runInContext('taskRecordFilter', context), 'complete');
assert.equal(recordFilters()[2].attributes['aria-pressed'], 'true');
const completedList = byId('task-records').children.at(-1);
assert.equal(completedList.attributes['aria-label'], '완료');
const completedItems = completedList.children.filter(n => n.className === 'record-item');
assert.ok(completedItems.length > 0);
for (const item of completedItems) {
  const actions = item.children.find(c => c.className === 'record-actions').children;
  assert.ok(indexedPaths.some(p => actions.find(c => c.tagName === 'A').href === '#' + encodeURIComponent(p)));
  assert.ok(!actions.some(c => c.tagName === 'BUTTON'), 'completed records only open the record');
}
recordFilters()[3].onclick();
for (const item of byId('task-records').children.at(-1).children.filter(n => n.className === 'record-item'))
  assert.ok(!item.children.find(c => c.className === 'record-actions').children.some(c => c.tagName === 'BUTTON'), 'reference records have no task panel');
assert.deepEqual(byId('task-records').children[0].children.map(c => c.tagName + ':' + c.textContent), ['H2:확인할 일과 작업 기록', 'BUTTON:새 작업'], 'the dashboard heading only starts new tasks');
assert.ok(html.includes('id="test-feedback-panel"'), 'generated page includes the task panel');
// 확인 대기·진행 중은 작업 진행 버튼과 AI 처리 상태만 두고, 완료는 기록 열기만 둔다.
const rowActions = (filter, title) => {
  vm.runInContext(String.raw`{
   data.documents.push({path:'.agents/workflow/tasks/zz-waiting.md',text:'# 대기 작업\n\n상태: 확인 대기 · 질문 1개\n다음 행동: 질문에 답한다.\n',modified:'9999'},{path:'.agents/workflow/tasks/zz-running.md',text:'# 처리 작업\n\n상태: 진행 중 · AI 구현 중\n다음 행동: 기다린다.\n',modified:'9999'},{path:'.agents/workflow/tasks/zz-done.md',text:'# 끝난 작업\n\n상태: 완료 · 체크리스트 1/1 통과\n다음 행동: 참고한다.\n',modified:'9999'});
   for(const name of ['zz-waiting','zz-done'])taskJobs['.agents/workflow/tasks/'+name+'.md']={latest:{status:'questions'}};
   try{recordFilters()[${filter}].onclick();}finally{data.documents.splice(-3,3);taskJobs=Object.create(null);}
  }`, Object.assign(context, { recordFilters }));
  const row = byId('task-records').children.at(-1).children.find(n => n.className === 'record-item' && n.children[0].children[0].textContent === title);
  return row.children.find(c => c.className === 'record-actions').children.map(c => c.tagName + ':' + c.textContent);
};
assert.deepEqual(rowActions(0, '대기 작업'), ['BUTTON:작업 진행', 'SPAN:질문 답변 필요'], 'waiting records are handled in the task panel with their AI status');
assert.deepEqual(rowActions(1, '처리 작업'), ['BUTTON:작업 진행'], 'running records are followed in the task panel');
assert.deepEqual(rowActions(2, '끝난 작업'), ['A:기록 열기 →'], 'completed records show no AI status');
recordFilters()[0].onclick();
vm.runInContext(String.raw`{
 const extra=[{path:'.agents/workflow/tasks/zz-active.md',text:'# 진행 작업\n\n- 상태: 진행 중 · 구현\n- 다음 행동: 빌드한다.\n',modified:'9999'},{path:'.agents/workflow/tasks/zz-review.md',text:'# 리뷰\n\n상태: 구현 중\n',modified:'9999'}];
 data.documents.push(...extra);
 try{const groups=taskRecordGroups();globalThis.activeItem=groups[1].items[0];globalThis.referenceTitles=groups[3].items.map(item=>item.title);}
 finally{data.documents.splice(-2,2);}
}`, context);
assert.deepEqual(JSON.parse(JSON.stringify(context.activeItem)), { title: '진행 작업', path: '.agents/workflow/tasks/zz-active.md', evidence: '구현', next: '빌드한다.' });
assert.ok(context.referenceTitles.includes('리뷰'), 'a free-form legacy status is listed under reviews and references');
assert.equal(byId('cards').children.length, 0, 'home must not list documents');
context.location.hash = '#search';
vm.runInContext('readRoute()', context);
assert.equal(context.location.hash, '');
assert.equal(byId('search-page').hidden, true);
assert.equal(byId('dashboard').hidden, false);
byId('search').value = 'WxCombat';
byId('search').handlers.input();
assert.equal(byId('cards').children.length, 0, 'Workflow must not search documents');
assert.ok(!html.includes('문서 검색 →'));
const menu = byId('nav').children[0].children;
assert.deepEqual(menu.map(link => link.textContent), ['작업 현황 대시보드', '작업 절차', 'LLM 위키 검색']);
assert.deepEqual(menu.map(link => link.href), ['#', '#' + encodeURIComponent('.agents/workflow/process/index.md'), 'knowledge.html']);
assert.equal(byId('other-space-row').hidden, true);
// The retired web task path must not come back through the generated page.
for (const removed of ['/analyze', '/handoff', '/execution', "'/tasks'", '새 작업 만들기', '기존 작업 이어하기']) assert.ok(!script.includes(removed), removed);
assert.equal(vm.runInContext("resolvePath('.wiki/wiki/topics/ai.md', '../concepts/combat-groggy.md')", context), '.wiki/wiki/concepts/combat-groggy.md');
assert.equal(vm.runInContext("resolvePath('.wiki/_index.md', '../.agents/workflow/process/index.md')", context), '.agents/workflow/process/index.md');
assert.equal(vm.runInContext("route('.wiki/wiki/topics/한글 문서.md','절 제목')", context), '#' + encodeURIComponent('.wiki/wiki/topics/한글 문서.md') + '!' + encodeURIComponent('절 제목'));
context.location.hash = '#missing-document';
vm.runInContext('readRoute()', context);
assert.equal(byId('article').children[0].textContent, '문서를 찾을 수 없습니다.');
console.log(`PASS ${data.documents.length} document snapshots, metadata, JS syntax, task states, new task and task panel entries, index navigation, routing and missing-document handling`);
