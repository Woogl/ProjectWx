// Copyright Woogle. All Rights Reserved.
// Builds the Workflow page from the current sources in a temporary folder and tests its data and UI logic without a browser or npm packages.
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const { spawnSync } = require('node:child_process');
const root = path.resolve(__dirname, '../..');
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'wx-viewer-'));
// 만들어 둔 페이지가 아니라 지금 소스로 새로 만든 페이지를 검사한다(기록이 바뀌어 실패하거나 옛 코드로 통과하지 않게).
// 스크립트 사본에는 닫는 태그 글자를 넣어 Export-Wiki가 인라인 스크립트를 지키는지 본다.
fs.cpSync(path.join(root, '.agents/workflow'), path.join(temp, '.agents/workflow'), { recursive: true });
fs.cpSync(path.join(__dirname, 'wiki-viewer'), path.join(temp, '.agents/scripts/wiki-viewer'), { recursive: true });
fs.copyFileSync(path.join(__dirname, 'Export-Wiki.ps1'), path.join(temp, '.agents/scripts/Export-Wiki.ps1'));
fs.appendFileSync(path.join(temp, '.agents/scripts/wiki-viewer/diagrams.js'), '\n// 닫는 태그 글자 검사: </SCRIPT> </script>\n');
function exportPage() {
  for (const shell of ['pwsh', path.join(process.env.USERPROFILE || '', '.cache/codex-runtimes/codex-primary-runtime/dependencies/native/powershell/pwsh.exe')]) {
    const run = spawnSync(shell, ['-NoProfile', '-File', path.join(temp, '.agents/scripts/Export-Wiki.ps1'), '-RepoRoot', temp], { encoding: 'utf8', windowsHide: true });
    if (run.error?.code === 'ENOENT') continue;
    assert.equal(run.status, 0, run.stderr || run.stdout);
    return fs.readFileSync(path.join(temp, 'Saved/Workflow/index.html'), 'utf8');
  }
  throw new Error('PowerShell 7(pwsh)이 필요합니다.');
}
const settle = () => new Promise(resolve => setImmediate(resolve));
(async () => {
  const html = exportPage();
  const payload = html.match(/<script id="wiki-data" type="application\/json">([\s\S]*?)<\/script>/)[1];
  const data = JSON.parse(payload);
  assert.ok(!/[<>&]/.test(payload), 'the data block escapes <, > and & so record text cannot close the script tag');
  // 페이지에는 데이터, 순정 marked, 뷰어 스크립트, mermaid, mermaid 시작이 차례로 실리고, 스크립트 속 닫는 태그 글자는 대소문자 그대로 <\/로 바뀐다.
  assert.equal(html.match(/<script[\s>]/gi).length, 5); assert.equal(html.match(/<\/script>/gi).length, 5);
  assert.ok(html.includes('닫는 태그 글자 검사: <\\/SCRIPT> <\\/script>'), 'inlined scripts cannot close their element early');
  const scripts = [...html.matchAll(/<script>\s*([\s\S]*?)<\/script>/gi)].map(match => match[1]);
  const markedScript = scripts.find(s => s.includes('marked v18.0.14')), script = scripts.find(s => s.includes('function readRoute'));
  assert.ok(markedScript && scripts.indexOf(markedScript) < scripts.indexOf(script), 'marked loads before the viewer script');
  new vm.Script(script);
  assert.match(scripts.at(-1), /renderWikiDiagrams\(document\.body\)/, 'diagrams drawn before Mermaid loaded (a plan opened by a reload) are drawn when it loads');
  assert.deepEqual(Object.keys(data).sort(), ['ai', 'documents', 'generated'], 'the page carries only what the viewer uses');
  assert.equal(new Set(data.documents.map(d => d.path)).size, data.documents.length);
  assert.ok(data.documents.every(d => d.path.startsWith('.agents/workflow/')), 'the Workflow page bundles only workflow documents; the Wiki is read in Obsidian');
  for (const d of data.documents) {
    assert.equal(d.text, fs.readFileSync(path.join(temp, d.path), 'utf8'));
    assert.deepEqual(Object.keys(d).sort(), ['modified', 'path', 'text', 'title'], 'documents carry only their source; the page renders Markdown');
  }
  let focused = null;
  class Element {
    constructor(tag) { this.tagName = tag.toUpperCase(); this.children = []; this.value = ''; this.dataset = {}; this.style = {}; this.classList = { toggle() {} }; this.handlers = {}; this.attributes = {}; }
    set id(value) { this._id = value; elements.set(value, this); }
    get id() { return this._id; }
    append(...nodes) { this.children.push(...nodes); }
    replaceChildren(...nodes) { this.children = nodes; }
    addEventListener(name, handler) { this.handlers[name] = handler; }
    focus() { focused = this; }
    querySelector(selector) { return this.children.find(child => child.tagName === selector.toUpperCase()) || null; }
    setAttribute(name, value) { this.attributes[name] = value; }
    removeAttribute(name) { delete this.attributes[name]; }
  }
  const elements = new Map();
  function byId(id) { if (!elements.has(id)) { const node = new Element('div'); node.id = id; } return elements.get(id); }
  byId('wiki-data').textContent = payload;
  const context = vm.createContext({
    document: { getElementById: byId, createElement: tag => new Element(tag), querySelectorAll: selector => {
      assert.equal(selector, '#nav a');
      return byId('nav').children.flatMap(group => group.children.filter(n => n.tagName === 'A'));
    }, addEventListener() {} },
    location: { hash: '' }, window: { addEventListener() {}, scrollTo() {} }, fetch() { throw new Error('requests go through workflowRequest in this test'); },
  });
  vm.runInContext(markedScript, context);
  vm.runInContext(script, context);
  // 문서는 순정 marked로 그린다. 취소선은 ~~만, 남길 태그로만 된 HTML만 HTML이고 나머지 꺾쇠 글자는 보이는 글자로 둔다.
  const parse = text => vm.runInContext(`marked.parse(${JSON.stringify(text)})`, context).trim();
  assert.equal(parse('**굵게**'), '<p><strong>굵게</strong></p>', 'documents render with the vendored marked');
  assert.equal(parse('1~2단계와 3~4단계, ~~지운 말~~'), '<p>1~2단계와 3~4단계, <del>지운 말</del></p>', 'Korean ranges keep their tildes; only double tildes strike through');
  assert.equal(parse('TSubclassOf<UWxAbilityBase>와 <AI> 뒤 글'), '<p>TSubclassOf&lt;UWxAbilityBase&gt;와 &lt;AI&gt; 뒤 글</p>', 'angle-bracket text stays visible and keeps the text after it');
  assert.equal(parse('<script>alert(1)</script>'), '<p>&lt;script&gt;alert(1)&lt;/script&gt;</p>', 'raw scripts are shown as text');
  assert.match(parse('<details id="document-notes">\n<summary>참고</summary>\n\n본문\n\n</details>'), /^<details id="document-notes">\n<summary>참고<\/summary>/, 'the document notes box stays HTML');
  assert.equal(parse('<!-- test-feedback:x:1 -->'), '<!-- test-feedback:x:1 -->', 'record markers stay hidden comments');
  assert.match(parse('3. 셋\n4. 넷'), /^<ol start="3">/, 'numbered lists keep their start number');
  assert.equal(parse('```mermaid\nflowchart TD\n  A --> B\n```'), '<pre><code class="language-mermaid">flowchart TD\n  A --&gt; B\n</code></pre>', 'a Mermaid block in a plan reaches the diagram renderer');
  // 안전 필터: 위험한 태그는 내용째(대소문자 무관), 모르는 태그는 풀어서 글을 남기고, 속성은 링크·도식·목록 번호·표 정렬·문서 참고 칸만 남긴다.
  const removed = [], unwrapped = [];
  const node = (tagName, attributes = []) => ({ tagName, attributes: attributes.map(([name, value]) => ({ name, value })), dropped: [], childNodes: ['안의 글'],
    remove() { removed.push(tagName); }, replaceWith(...nodes) { unwrapped.push([tagName, nodes]); }, removeAttribute(name) { this.dropped.push(name); } });
  const kept = [node('OL', [['start', '3'], ['onclick', 'x']]), node('OL', [['start', '3x']]), node('TD', [['align', 'center'], ['style', 'x']]), node('DETAILS', [['id', 'document-notes']]), node('H2', [['id', 'nav']]), node('A', [['href', '#x'], ['target', '_top']]), node('CODE', [['class', 'language-mermaid']])];
  context.DOMParser = class { parseFromString() { return { body: { querySelectorAll: () => [node('IMG'), node('svg'), node('AI'), ...kept] } }; } };
  vm.runInContext("safeFragment('')", context);
  assert.deepEqual(removed, ['IMG', 'svg'], 'images and embedded SVG go with their content');
  assert.deepEqual(unwrapped, [['AI', ['안의 글']]], 'unknown tags are unwrapped and keep their text');
  assert.deepEqual(kept.map(n => n.dropped), [['onclick'], ['start'], ['style'], [], ['id'], ['target'], []]);
  assert.ok(!html.match(/http-equiv="Content-Security-Policy" content="([^"]*)"/)[1].includes('img-src'), 'the page loads no images');
  assert.match(script, /await mermaid\.run\(\{ nodes: \[diagram\] \}\)/, 'diagrams render with stock mermaid.run');
  assert.ok(html.includes('.wiki-diagram svg{display:block;'), 'rendered diagrams stay centered');
  console.log('PASS page rebuilt from sources, script closing text, marked rules, safe fragment and stock Mermaid rendering');
  // Work records are listed from their own state lines without the local server, including records without a state.
  assert.equal(vm.runInContext("taskRecordGroups().map(g=>g.title).join(',')", context), '확인 대기,진행 중,완료,리뷰·참고');
  const indexedPaths = Array.from(vm.runInContext('taskRecordGroups().flatMap(g=>g.items.map(item=>item.path))', context));
  const taskPaths = data.documents.map(d => d.path).filter(p => /^\.agents\/workflow\/tasks\/[^/]+\.md$/.test(p));
  assert.deepEqual(indexedPaths.slice().sort(), taskPaths.sort(), 'every Markdown task must be listed exactly once');
  assert.ok(byId('task-records').children.length > 0, 'records are available before the local server connects');
  // 메뉴는 화면 셋이고 지금 화면을 aria-current로 알린다. 첫 화면은 대시보드이고 제목으로 포커스를 옮긴다.
  const menu = byId('nav').children[0].children;
  assert.deepEqual(menu.map(link => link.textContent), ['대시보드', '작업', '작업 절차']);
  assert.deepEqual(menu.map(link => link.href), ['#', '#work', '#' + encodeURIComponent('.agents/workflow/process/index.md')]);
  assert.deepEqual(menu.map(link => link.attributes['aria-current']), ['page', undefined, undefined]);
  assert.equal(focused, byId('dashboard-title'), 'the first view focuses its heading');
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
  assert.deepEqual(byId('task-records').children[0].children.map(c => c.tagName + ':' + c.textContent), ['H2:확인할 일과 작업 기록', 'BUTTON:새 작업', 'BUTTON:Wiki 갱신'], 'the dashboard heading starts new tasks and the Wiki update');
  assert.ok(html.includes('id="test-feedback-panel"'), 'generated page includes the task panel');
  // 확인 대기·진행 중은 이어서 작업 버튼과 AI 처리 상태만 두고, 완료는 기록 열기만 둔다. 서버 없이 페이지에 없는 기록은 다시 실행하라고 알린다.
  const rowActions = (filter, title) => {
    vm.runInContext(String.raw`{
     data.documents.push({path:'.agents/workflow/tasks/zz-waiting.md',text:'# 대기 작업\n\n상태: 확인 대기 · 질문 1개\n다음 행동: 질문에 답한다.\n',modified:'9999'},{path:'.agents/workflow/tasks/zz-running.md',text:'# 처리 작업\n\n상태: 진행 중 · AI 구현 중\n다음 행동: 기다린다.\n',modified:'9999'},{path:'.agents/workflow/tasks/zz-done.md',text:'# 끝난 작업\n\n상태: 완료 · 체크리스트 1/1 통과\n다음 행동: 참고한다.\n',modified:'9999'});
     for(const name of ['zz-waiting','zz-done'])taskJobs['.agents/workflow/tasks/'+name+'.md']={latest:{status:'questions'}};
     try{recordFilters()[${filter}].onclick();}finally{data.documents.splice(-3,3);taskJobs=Object.create(null);}
    }`, Object.assign(context, { recordFilters }));
    const row = byId('task-records').children.at(-1).children.find(n => n.className === 'record-item' && n.children[0].children[0].textContent === title);
    return row.children.find(c => c.className === 'record-actions').children.map(c => c.tagName + ':' + c.textContent);
  };
  assert.deepEqual(rowActions(0, '대기 작업'), ['BUTTON:이어서 작업', 'SPAN:질문 답변 필요'], 'waiting records are handled in the task panel with their AI status');
  assert.equal(byId('task-records').children.at(-1).children.find(n => n.className === 'record-item' && n.children[0].children[0].textContent === '대기 작업').children.at(-1).children[0].attributes['aria-label'], '이어서 작업: 대기 작업', 'row buttons are named after their task');
  assert.deepEqual(rowActions(1, '처리 작업'), ['BUTTON:이어서 작업'], 'running records are followed in the task panel');
  assert.deepEqual(rowActions(2, '끝난 작업'), ['A:기록 열기 →'], 'completed records show no AI status');
  vm.runInContext(String.raw`{liveTaskRecords=[{path:'.agents/workflow/tasks/zz-after-export.md',title:'나중 기록',state:'완료',summary:'',next:'',modified:'9999'}];taskRecordFilter='complete';renderTaskRecords();}`, context);
  const lateActions = () => byId('task-records').children.at(-1).children.find(n => n.className === 'record-item').children.find(c => c.className === 'record-actions').children.map(c => c.tagName + ':' + c.textContent);
  assert.deepEqual(lateActions(), ['SPAN:새 기록 · OpenWorkflow.bat을 다시 실행하면 열립니다.'], 'without the server a record made after the export cannot be read');
  vm.runInContext("data.ai={token:'t',url:'http://127.0.0.1:1'};renderTaskRecords();", context);
  assert.deepEqual(lateActions(), ['A:기록 열기 →'], 'with the server every record opens');
  vm.runInContext("data.ai=null;liveTaskRecords=null;taskRecordFilter='waiting';", context);
  recordFilters()[0].onclick();
  vm.runInContext(String.raw`{
   const extra=[{path:'.agents/workflow/tasks/zz-active.md',text:'# 진행 작업\n\n- 상태: 진행 중 · 구현\n- 다음 행동: 빌드한다.\n',modified:'9999'},{path:'.agents/workflow/tasks/zz-review.md',text:'# 리뷰\n\n상태: 구현 중\n',modified:'9999'}];
   data.documents.push(...extra);
   try{const groups=taskRecordGroups();globalThis.activeItem=groups[1].items[0];globalThis.referenceTitles=groups[3].items.map(item=>item.title);}
   finally{data.documents.splice(-2,2);}
  }`, context);
  assert.deepEqual(JSON.parse(JSON.stringify(context.activeItem)), { title: '진행 작업', path: '.agents/workflow/tasks/zz-active.md', evidence: '구현', next: '빌드한다.' });
  assert.ok(context.referenceTitles.includes('리뷰'), 'a free-form legacy status is listed under reviews and references');
  // Workflow launcher starts the local AI server before opening the generated page.
  const launcher = fs.readFileSync(path.join(root, 'BatchFiles', 'OpenWorkflow.bat'), 'utf8');
  assert.ok(launcher.includes('Start-WikiAI.ps1') && launcher.includes('Export-Wiki.ps1" -Open'));
  assert.ok(html.includes("const workflowKey = 'wx-wiki-workflow-v1:' + location.pathname"), 'the storage key keeps saved drafts');
  assert.equal(vm.runInContext("resolvePath('.agents/workflow/tasks/a.md', '../process/index.md')", context), '.agents/workflow/process/index.md');
  assert.equal(vm.runInContext("resolvePath('.agents/workflow/index.md', '../../README.md')", context), 'README.md');
  assert.equal(vm.runInContext("route('.agents/workflow/tasks/한글 작업.md','절 제목')", context), '#' + encodeURIComponent('.agents/workflow/tasks/한글 작업.md') + '!' + encodeURIComponent('절 제목'));
  // 주소의 !는 경로와 절을 나누므로 경로와 절 속의 !는 인코딩하고, 읽을 때 되돌린다.
  const bang = vm.runInContext("route('.agents/workflow/tasks/a!b.md','절!')", context);
  assert.equal(bang, '#.agents%2Fworkflow%2Ftasks%2Fa%21b.md!%EC%A0%88%21');
  context.location.hash = bang; vm.runInContext('readRoute()', context);
  assert.deepEqual(byId('article').children.map(n => n.textContent), ['문서를 찾을 수 없습니다.', '.agents/workflow/tasks/a!b.md']);
  assert.equal(focused, byId('article').children[0], 'switching to the reader focuses its heading');
  context.location.hash = '#missing-document';
  vm.runInContext('readRoute()', context);
  assert.equal(byId('article').children[0].textContent, '문서를 찾을 수 없습니다.');
  // Wiki 갱신: AI 연결이 없으면 꺼져 있고, 있으면 누르는 즉시 고른 AI로 시작하며, 진행 중에는 다시 누를 수 없다.
  await settle(); // 첫 렌더가 시작한 상태 확인이 끝난 뒤에 상태를 바꾼다.
  context.location.hash = ''; vm.runInContext('readRoute()', context);
  const headingButtons = () => byId('task-records').children[0].children.filter(n => n.tagName === 'BUTTON');
  const wikiNotice = () => byId('task-records').children[1].textContent;
  vm.runInContext('data.ai=null;renderTaskRecords();', context);
  assert.equal(headingButtons()[1].disabled, true, 'Wiki update needs the local AI connection');
  vm.runInContext("data.ai={token:'t',url:'http://127.0.0.1:1'};taskProviders=[{id:'claude',label:'Claude Code'},{id:'codex',label:'Codex'}];renderProviderChoice();globalThis.fired=[];globalThis.wikiState={status:'idle'};workflowRequest=async(endpoint,body)=>{fired.push([endpoint,body]);if(body.action==='list')return {records:{},providers:taskProviders,tasks:null};if(body.action==='start')wikiState={status:'running',provider:body.provider,message:'AI가 작업하는 중입니다.'};return wikiState;};renderTaskRecords();", context);
  // 처리할 AI는 왼쪽 메뉴 아래에서 한 번 고르고 Wiki 갱신도 그 선택을 따른다.
  const aiChoice = byId('ai-provider');
  assert.deepEqual(aiChoice.children.map(option => option.value), ['claude', 'codex']);
  aiChoice.value = 'codex'; aiChoice.onchange();
  assert.equal(headingButtons()[1].disabled, false);
  const hashBefore = context.location.hash;
  await headingButtons()[1].onclick();
  assert.deepEqual(JSON.parse(JSON.stringify(context.fired.at(-1))), ['/wiki-update', { action: 'start', provider: 'codex' }]);
  assert.equal(context.location.hash, hashBefore, 'the Wiki update starts without leaving the dashboard');
  assert.equal(headingButtons()[1].disabled, true, 'a running Wiki update cannot start again');
  assert.equal(wikiNotice(), 'Wiki 갱신 중 · Codex · AI가 작업하는 중입니다.');
  vm.runInContext("wikiState={status:'complete',provider:'codex',summary:'원자료 1건을 수집했습니다.'};", context);
  await vm.runInContext('loadWikiUpdate()', context);
  assert.equal(headingButtons()[1].disabled, false);
  assert.match(wikiNotice(), /마쳤습니다\(Codex\)\. 원자료 1건을 수집했습니다\./, 'the dashboard keeps the last result');
  const listBefore = byId('task-records').children.at(-1);
  await vm.runInContext('loadWikiUpdate()', context);
  assert.equal(byId('task-records').children.at(-1), listBefore, 'Wiki progress updates only its line, not the record list');
  // 시작 응답을 기다리는 동안 두 번 눌러도 한 번만 시작한다.
  const firing = context.fired.length;
  await Promise.all([headingButtons()[1].onclick(), headingButtons()[1].onclick()]);
  assert.equal(context.fired.length, firing + 1, 'a double click starts the Wiki update once');
  vm.runInContext("wikiState={status:'complete',provider:'codex',summary:'원자료 1건을 수집했습니다.'};", context);
  await vm.runInContext('loadWikiUpdate()', context);
  // 고른 AI가 연결되어 있지 않으면 요청하지 않고 그 이유를 같은 자리에 보여준다.
  const before = context.fired.length;
  vm.runInContext("taskProviders=[{id:'claude',label:'Claude Code'}];renderProviderChoice();", context);
  await headingButtons()[1].onclick();
  assert.equal(context.fired.length, before); assert.match(wikiNotice(), /연결되어 있지 않습니다/);
  // 대시보드에 들어올 때마다 서버에서 목록을 다시 받는다.
  const lists = () => context.fired.filter(([, body]) => body.action === 'list').length, listsBefore = lists();
  context.location.hash = '#work'; vm.runInContext('readRoute()', context);
  assert.equal(focused, byId('task-title'), 'switching to the work tab focuses its heading');
  context.location.hash = ''; vm.runInContext('readRoute()', context); await settle();
  assert.equal(lists(), listsBefore + 1, 'the dashboard asks the server again on every visit');
  assert.equal(focused, byId('dashboard-title'));
  // 대시보드와 작업 탭: 버튼은 주소만 바꾸고 주소가 화면을 정한다.
  assert.deepEqual([byId('dashboard').hidden, byId('work').hidden, byId('reader').hidden], [false, true, true], 'the dashboard shows only the task overview');
  headingButtons()[0].onclick(); assert.equal(context.location.hash, '#work!new', 'new task switches to the work tab');
  vm.runInContext('readRoute()', context);
  assert.deepEqual([byId('dashboard').hidden, byId('work').hidden], [true, false]); assert.equal(byId('task-title').textContent, '새 작업');
  assert.deepEqual(menu.map(link => link.attributes['aria-current']), [undefined, 'page', undefined], 'the menu marks the current view');
  vm.runInContext(String.raw`{data.documents.push({path:'.agents/workflow/tasks/zz-waiting.md',text:'# 대기 작업\n\n상태: 확인 대기 · 질문 1개\n다음 행동: 질문에 답한다.\n',modified:'9999'});liveTaskRecords=null;taskRecordFilter='waiting';renderTaskRecords();}`, context);
  const continueButton = byId('task-records').children.at(-1).children.find(n => n.className === 'record-item' && n.children[0].children[0].textContent === '대기 작업').children.at(-1).children[0];
  continueButton.onclick(); assert.equal(context.location.hash, '#work!' + encodeURIComponent('.agents/workflow/tasks/zz-waiting.md'), '이어서 작업 switches to the work tab for that task');
  vm.runInContext('data.documents.pop()', context);
  // 기록 열기: 서버가 연결돼 있으면 작업 기록의 최신 내용을 그리고(페이지를 만든 뒤 생긴 기록도), 읽지 못하면 페이지의 내용이나 이유를 보인다.
  const snapshot = taskPaths[0];
  vm.runInContext("workflowRequest=async(endpoint,body)=>{globalThis.readBody=[endpoint,body];if(globalThis.readFails)throw Error('로컬 AI 서버에 연결하지 못했습니다.');return {title:'새 기록',text:'# 새 기록\\n\\n최신 본문\\n'};};showDocument=(d,anchor,notice)=>{globalThis.shown={d,anchor,notice};};", context);
  context.location.hash = vm.runInContext("route('.agents/workflow/tasks/zz-after-export.md','section-2')", context); vm.runInContext('readRoute()', context);
  assert.match(byId('article').children[0].textContent, /최신 내용을 불러오는 중/);
  await settle();
  assert.deepEqual(JSON.parse(JSON.stringify(context.readBody)), ['/test-feedback', { action: 'read', taskPath: '.agents/workflow/tasks/zz-after-export.md' }]);
  assert.deepEqual(JSON.parse(JSON.stringify(context.shown)), { d: { path: '.agents/workflow/tasks/zz-after-export.md', title: '새 기록', text: '# 새 기록\n\n최신 본문\n' }, anchor: 'section-2' }, 'the reader draws the live record');
  context.readFails = true; context.shown = null;
  context.location.hash = vm.runInContext(`route(${JSON.stringify(snapshot)})`, context); vm.runInContext('readRoute()', context); await settle();
  assert.equal(context.shown.d.path, snapshot); assert.match(context.shown.notice, /이 페이지를 만든 때의 내용.*연결하지 못했습니다/, 'an unreachable server falls back to the snapshot and says so');
  context.location.hash = vm.runInContext("route('.agents/workflow/tasks/zz-after-export.md')", context); vm.runInContext('readRoute()', context); await settle();
  assert.deepEqual(byId('article').children.map(n => n.textContent), ['문서를 찾을 수 없습니다.', '.agents/workflow/tasks/zz-after-export.md', '로컬 AI 서버에 연결하지 못했습니다.']);
  context.shown = null; vm.runInContext("data.ai=null;", context);
  context.location.hash = vm.runInContext("route('.agents/workflow/process/index.md')", context); vm.runInContext('readRoute()', context);
  assert.equal(context.shown.d.path, '.agents/workflow/process/index.md', 'other documents are read from the page');
  console.log(`PASS dashboard and work tab routes, menu state and focus, ${data.documents.length} document snapshots, JS syntax, task states, new task and task panel entries, row button names, record reading (live, after export, fallback), Wiki update button (progress line only, single start), launcher, routing with encoded ! and missing-document handling`);
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(() => {
  const resolved = path.resolve(temp); assert.equal(path.dirname(resolved), path.resolve(os.tmpdir())); assert.ok(path.basename(resolved).startsWith('wx-viewer-'));
  fs.rmSync(resolved, { recursive: true, force: true });
});
