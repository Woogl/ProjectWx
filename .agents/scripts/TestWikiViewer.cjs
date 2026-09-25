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
const script = html.match(/<script>\s*([\s\S]*?)<\/script>/)[1].replace('if(isWorkflow)syncSharedTasks();','');
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
// Legacy browser task lists must not populate the shared task picker.
{const c=vm.createContext({location:{pathname:'/workflow'},localStorage:{getItem:()=>JSON.stringify({ghost:{taskId:'ghost'}})}});
vm.runInContext(['workflow-model.js','workflow.js'].map(f=>fs.readFileSync(path.join(root,'.agents/scripts/wiki-viewer',f),'utf8')).join('\n'),c);
assert.equal(vm.runInContext('Object.keys(otherTasks).length',c),0);}
const imagePath = '.agents/workflow/assets/ai-workflow.png';
assert.equal(data.images[imagePath], 'data:image/png;base64,' + fs.readFileSync(path.join(root, imagePath)).toString('base64'));
// Asset bundling is independent of whether a particular guide currently embeds the image.
assert.equal(vm.runInContext("wikiImageSource('.agents/workflow/usage.md', 'assets/ai-workflow.png')", context), data.images[imagePath]);
for (const source of ['https://example.com/a.png', '//example.com/a.png', 'file:///C:/a.png', 'data:image/svg+xml,test', '../assets/missing.png', '%invalid']) {
  context.testImageSource = source;
  assert.equal(vm.runInContext("wikiImageSource('.agents/workflow/usage.md', testImageSource)", context), null);
}
const imageAttributes = new Map([['src', 'assets/ai-workflow.png'], ['alt', '워크플로우'], ['onload', 'bad()'], ['srcset', 'https://example.com/a.png']]);
const imageNode = { tagName: 'IMG', get attributes() { return [...imageAttributes.keys()].map(name => ({ name })); },
  getAttribute: key => imageAttributes.get(key), removeAttribute: key => imageAttributes.delete(key), setAttribute: (key, value) => imageAttributes.set(key, value) };
context.DOMParser = class { parseFromString() { return { body: { querySelectorAll: () => [imageNode] } }; } };
vm.runInContext("safeFragment('', '.agents/workflow/usage.md')", context);
assert.equal(imageAttributes.get('src'), data.images[imagePath]);
assert.equal(imageAttributes.get('alt'), '워크플로우');
assert.equal(imageAttributes.size, 2, 'image handlers and srcset must be removed');
assert.ok(html.includes('img-src data:'));
assert.ok(html.includes('article img{display:block;max-width:100%;height:auto;'));
console.log('PASS bundled workflow PNG, relative image resolution, external image rejection and sanitized responsive rendering');
assert.equal(byId('work-summary').children.length,0,'no stage choices before a task is selected');
assert.match(byId('current-task-title').textContent,/공용 작업 연결/);
// Recorded work remains visible offline and opens evidence without creating or approving a web task.
const indexedPaths=Array.from(vm.runInContext('taskRecordGroups().flatMap(g=>g.items.map(item=>item.path))',context));
const taskPaths=fs.readdirSync(path.join(root,'.agents/workflow/tasks')).filter(name=>name.endsWith('.md')&&name!=='index.md'&&!/^workflow_.+_(planning|implementation)\.md$/.test(name)).map(name=>'.agents/workflow/tasks/'+name);
assert.deepEqual(indexedPaths.slice().sort(),taskPaths.sort(),'every Markdown task must be listed exactly once');
assert.equal(byId('task-records').hidden,false,'records are available before the shared server connects');
const recordState=vm.runInContext('JSON.stringify(workflowState)',context);
const recordFilters=()=>byId('task-records').children.find(n=>n.className==='record-filters').children;
recordFilters()[3].onclick();
assert.equal(vm.runInContext('taskRecordFilter',context),'complete');
assert.equal(recordFilters()[3].attributes['aria-pressed'],'true');
const completedList=byId('task-records').children.at(-1);
assert.equal(completedList.attributes['aria-label'],'완료 기록');
const completedLinks=completedList.children.filter(n=>n.className==='record-item').map(n=>n.children.find(c=>c.className==='record-actions').children.find(c=>c.tagName==='A'));
assert.ok(completedLinks.length>0);
assert.ok(completedLinks.every(n=>indexedPaths.some(p=>n.href==='#'+encodeURIComponent(p))));
assert.ok(html.includes('id="test-feedback-panel"'),'generated page includes the feedback panel');
assert.equal(vm.runInContext('JSON.stringify(workflowState)',context),recordState,'browsing records must not change approval state');
recordFilters()[0].onclick();
vm.runInContext(String.raw`{
 const index=data.documents.find(d=>d.path==='.agents/workflow/tasks/index.md'),original=index.text;
 try{index.text='## 플레이 확인\n| [누락](missing.md) | 미확인 | 확인 |\n| [외부](https://example.com/task.md) | 미확인 | 확인 |\n## 갱신 방법\n| [과거](checkpoint-savegame.md) | 완료 | 없음 |';
 globalThis.invalidRecordCount=taskRecordGroups().flatMap(g=>g.items).length;
 }finally{index.text=original;}
}`,context);
assert.equal(context.invalidRecordCount,0,'unknown, external and non-status links must not become work items');
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
const menu=byId('nav').children[0].children;
assert.deepEqual(menu.map(link=>link.textContent),['작업 현황 대시보드','새 작업 만들기','기존 작업 이어하기','사용 방법 안내','LLM 위키 검색']);
assert.deepEqual(menu.map(link=>link.href),['#','#new-task','#resume-task','#'+encodeURIComponent('.agents/workflow/usage.md'),'knowledge.html']);
assert.equal(byId('other-space-row').hidden,true);
// Classify independent saved tasks without changing the currently open task.
vm.runInContext(`
globalThis.boardFixtures=[];
function boardTask(name,confirmCount,execution){
 const task=newTask(name);task.source='기획';
 for(const stage of ['planning','implementation'].slice(0,confirmCount)){
  task[stage].result={draft:stage+' 확정본',blockers:[]};
  task[stage].reviewBasis=loopBasis(stage,task);
  task[stage].confirmation={basis:loopBasis(stage,task),draft:task[stage].result.draft};
 }
 if(execution)task.execution=execution;
 boardFixtures.push(task);return task;
}
boardTask('기획 일감',0);boardTask('설계 일감',1);boardTask('구현 일감',2,{status:'running',phase:'implement'});
boardTask('리뷰 일감',2,{status:'review'});boardTask('테스트 일감',2,{status:'blocked',phase:'verify'});
boardTask('정리 일감',2,{status:'cleanup'});boardTask('완료 일감',2,{status:'complete'});boardTask('이전 일감',2).changeTo='기획 일감';
`,context);
assert.equal(vm.runInContext('boardFixtures.map(dashboardStage).join(",")',context),'planning,design,implementation,review,testing,cleanup,completion,changed');
assert.match(vm.runInContext('dashboardStatus(boardFixtures[4],"testing")',context),/보류/);
vm.runInContext('sharedReady=true;otherTasks=Object.fromEntries(boardFixtures.map(task=>[task.taskId,task]));renderWorkSummary()',context);
assert.equal(byId('work-summary').children.length,8);
assert.ok(byId('work-summary').children.every(column=>column.children.length===2));
assert.equal(vm.runInContext('workflowState.taskId',context),'');
vm.runInContext('boardFixtures[2].source="수정된 기획"',context);
assert.equal(vm.runInContext('dashboardStage(boardFixtures[2])',context),'planning','stale confirmations must not advance a task');
vm.runInContext('otherTasks=Object.create(null);renderWorkSummary()',context);
assert.equal(byId('work-summary').children.length,0,'empty web stages must not distract from recorded work');
assert.match(byId('current-task-title').textContent,/웹에서 진행 중인 작업이 없습니다/);
vm.runInContext('sharedReady=false',context);

for(const [hash,title] of [['#new-task','새 작업 만들기'],['#resume-task','기존 작업 이어하기']]){
context.location.hash=hash;vm.runInContext('readRoute()',context);
assert.equal(byId('dashboard').hidden,false);assert.equal(byId('dashboard-title').textContent,title);assert.equal(byId('work-summary').hidden,true);
assert.equal(byId('task-records').hidden,true,'record browsing is hidden while creating or resuming web work');
}
context.location.hash='';vm.runInContext('readRoute()',context);assert.equal(byId('work-summary').hidden,false);
assert.equal(vm.runInContext("resolvePath('.wiki/wiki/topics/ai.md', '../concepts/combat-groggy.md')", context), '.wiki/wiki/concepts/combat-groggy.md');
assert.equal(vm.runInContext("resolvePath('.wiki/_index.md', '../.agents/workflow/tasks/index.md')", context), '.agents/workflow/tasks/index.md');
assert.equal(vm.runInContext("route('.wiki/wiki/topics/한글 문서.md','절 제목')", context), '#' + encodeURIComponent('.wiki/wiki/topics/한글 문서.md') + '!' + encodeURIComponent('절 제목'));
context.location.hash = '#missing-document';
vm.runInContext('readRoute()', context);
assert.equal(byId('article').children[0].textContent, '문서를 찾을 수 없습니다.');
console.log(`PASS ${data.documents.length} document snapshots, metadata, JS syntax, search without categories, index navigation, routing and missing-document handling`);

// Run actual browser handlers with mocked network and durable server snapshots.
const storage=new Map();
context.localStorage={getItem:k=>storage.get(k)??null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)};
context.history={replaceState(_state,_title,url){context.location.hash=url;}};
const run=code=>vm.runInContext(code,context);
// File persistence and conflicts are exercised against the real API in TestWikiTasks/Recovery.
run('readRoute=()=>{};sharedReady=true;flushSharedTasks=async()=>{draftRecovery=Object.create(null)};loadSharedTasks=async()=>{}');
function descendants(n){return[n,...n.children.flatMap(descendants)];}
function control(text){const b=descendants(byId('workflow-controls')).find(n=>n.tagName==='BUTTON'&&n.textContent===text);assert.ok(b,`missing: ${text}`);return b;}
function input(label){return descendants(byId('workflow-controls')).find(n=>n.attributes?.['aria-label']===label);}
const result={summary:'검토',draft:'회피 비용 20',facts:['비용 20'],evidence:['기획 원문'],blockers:[],items:[],questions:[]};
const question={id:'Q1',title:'취소 정책',requirement:'기획 미정',scope:'취소 시 반환?',options:['반환','유지'],recommendation:'유지',impact:'비용 처리',reopenReason:''};
let response={...result,questions:[question]},requests=[],handoffs=[];
run("data.ai={url:'http://127.0.0.1:18743/analyze',token:'test',providers:[{id:'codex',label:'Codex'},{id:'claude',label:'Claude Code'},{id:'gemini',label:'Gemini CLI'}]}");
context.fetch=async(url,options)=>{
  const body=JSON.parse(options.body);assert.equal(options.headers['X-Wx-Token'],'test');
  if(url.endsWith('/execution'))return{ok:true,json:async()=>({taskId:body.taskId,status:'running',phase:'implement',revision:1})};
  if(url.endsWith('/tasks'))return{ok:true,json:async()=>({tasks:{}})};
  if(url.endsWith('/test-feedback'))return{ok:true,json:async()=>({records:{}})};
  if(url.endsWith('/rename'))return{ok:true,json:async()=>({revision:body.expectedRevision+1,taskId:body.title,title:body.title,revisions:{[body.title]:body.expectedRevision+1}})};
  if(url.endsWith('/change'))return{ok:true,json:async()=>({revision:body.expectedRevision+1,taskId:body.newTaskId,childRevision:0,changeFrom:{taskId:body.taskId,stage:body.stage,reason:body.reason,scope:body.scope}})};
  if(url.endsWith('/handoff')){handoffs.push(body);return{ok:true,json:async()=>({path:`.agents/workflow/tasks/test-${handoffs.length}.md`,dataPath:'test.json',revision:body.expectedRevision+1})};}
  requests.push(body);return{ok:true,json:async()=>response};
};
function applyExclusions(){if(response.exclusions?.length)control('검토 결과 반영 · 기존 판단 보존').onclick();}
(async()=>{
  run("renderWorkflow('.agents/workflow/process/design_review.md')");
  input('새 작업 제목').value='회피 시스템 개선';input('새 작업 제목').oninput();await control('새 작업 만들기').onclick();
  assert.equal(run('workflowState.title'),'회피 시스템 개선');
  assert.equal(run('workflowState.taskId'),'회피 시스템 개선','the title itself is the task key');
  input('현재 작업 제목').value='회피 설계 초안';await control('제목 변경').onclick();
  assert.equal(run('workflowState.taskId'),'회피 설계 초안');
  input('현재 작업 제목').value='회피 시스템 개선';await control('제목 변경').onclick();
  assert.equal(input('기존 작업 선택').value,'회피 시스템 개선');
  assert.equal(input('기존 작업 선택').tagName,'SELECT');
  assert.equal(input('새 작업 제목').value,'');
  assert.equal(input('검토할 AI 서비스').children.length,3);
  assert.ok(!descendants(byId('workflow-controls')).some(n=>['현재 진행 단계','진행상황과 다음 할 일','추가 요청·수정 의견'].includes(n.attributes?.['aria-label'])),'fresh planning only asks for the source');
  assert.equal(run("reviewStatus('planning')"),'기획서 입력부터 시작');
  const createSection=descendants(byId('workflow-controls')).find(n=>n.tagName==='DETAILS'&&n.children.some(c=>c.textContent==='새 작업 만들기'));
  assert.ok(createSection&&!createSection.open,'new task form is collapsed while working');
  run("globalThis.allProviders=data.ai.providers;data.ai.providers=[allProviders[0]];renderWorkflow('.agents/workflow/process/design_review.md')");
  assert.equal(input('검토할 AI 서비스').children.length,1,'provider selection remains visible with one provider');
  assert.ok(!descendants(byId('workflow-controls')).filter(n=>n.tagName==='DETAILS').some(n=>descendants(n).includes(input('검토할 AI 서비스'))),'provider selection must not be collapsible');
  run("data.ai.providers=allProviders;renderWorkflow('.agents/workflow/process/design_review.md')");
  input('검토할 AI 서비스').value='claude';input('검토할 AI 서비스').onchange();
  input('기획서 원본').value='원본 비용 20';input('기획서 원본').oninput();
  const fetchBeforeWait=context.fetch;
  let finishReview,waitingRequests=0;
  const waiting=new Promise(resolve=>{finishReview=resolve;});
  context.fetch=async(...args)=>{waitingRequests++;await waiting;return fetchBeforeWait(...args);};
  const reviewing=control('AI 기획서 검토').onclick();
  assert.equal(run("workflowGuideDocument(docs.find(d=>d.path==='.agents/workflow/process/design_review.md')).path"),'.agents/workflow/process/design_review.md','guide follows the active workflow stage');
  assert.equal(byId('workflow-busy').hidden,false,'waiting indicator is visible');
  assert.equal(byId('app-shell').inert,true,'all background controls and navigation are locked');
  await control('AI 기획서 검토').onclick();
  assert.equal(waitingRequests,1,'duplicate review cannot start');
  const reviewingTask=run('workflowState.taskId');
  input('새 작업 제목').value='검토 중 새 작업';input('새 작업 제목').oninput();await control('새 작업 만들기').onclick();
  assert.equal(run('workflowState.taskId'),reviewingTask,'task switch is blocked during review');
  finishReview();await reviewing;context.fetch=fetchBeforeWait;
  assert.equal(byId('workflow-busy').hidden,true);
  assert.equal(byId('app-shell').inert,false,'success unlocks the screen');
  assert.equal(requests.at(-1).provider,'claude');
  assert.equal(run('workflowState.provider'),'claude');
  assert.equal(run('workflowState.planning.decisions.length'),1,'questions must be answerable immediately');
  assert.ok(input('추가 요청·수정 의견'),'feedback becomes available after review');
  assert.equal(run("reviewStatus('planning')"),'1개 질문에 답변 필요');
  assert.equal(control('검토본 확정 · 설계 검토').disabled,true,'unresolved decision disables handoff');
  assert.ok(descendants(byId('workflow-controls')).some(n=>n.textContent==='답변 결정 필요: 취소 정책'));
  assert.ok(descendants(byId('workflow-controls')).some(n=>n.className==='decision-card decision-unanswered'));
  control('반환').onclick();
  assert.ok(descendants(byId('workflow-controls')).some(n=>n.className==='decision-card decision-unanswered'));
  input('Q1 답변').value += '\n\n추가 의견';input('Q1 답변').oninput();
  control('유지').onclick();
  assert.equal(input('Q1 답변').value,'유지\n\n추가 의견');
  assert.equal(run('workflowState.planning.decisions[0].confirmed'),false,'choice is not confirmation');
  applyExclusions();
  assert.equal(run("loopReady('planning')"),false);
  response={...result,questions:[{...question,id:'Q2',title:'후속 판단',scope:'추가 정책 선택?'}]};
  const beforeAutomatic=requests.length;
  input('Q1 답변').value='반환하지 않음';input('Q1 답변').oninput();await control('이 답변으로 결정').onclick();
  assert.equal(requests.length,beforeAutomatic+1,'confirmation automatically requests review');
  assert.ok(input('Q2 답변'),'follow-up question is immediately answerable');
  assert.equal(run('workflowState.planning.decisions[0].confirmed'),true);
  assert.ok(descendants(byId('workflow-controls')).some(n=>n.className==='decision-card decision-confirmed'));
  assert.equal(run("loopReady('planning')"),false,'follow-up decision blocks finalization');
  response={...result,questions:[]};
  input('Q2 답변').value='추가 정책 결정';input('Q2 답변').oninput();
  await control('이 답변으로 결정').onclick();
  assert.equal(run("loopReady('planning')"),true,'review converges without automatic final confirmation');
  response={...result,draft:'회피 비용 20, 취소해도 반환하지 않음'};
  await control('AI 기획서 검토').onclick();
  assert.equal(requests.at(-1).context.decisions[0].answer,'반환하지 않음');
  applyExclusions();
  assert.equal(run('workflowState.planning.decisions[0].confirmed'),true,'omission preserves confirmed decision');
  assert.equal(run("loopReady('planning')"),true);
  assert.equal(run("loopConfirmed('planning')"),false,'AI never finalizes');
  assert.equal(control('검토본 확정 · 설계 검토').disabled,false);
  const saveFetch=context.fetch;let releaseSave;
  const saveWait=new Promise(resolve=>{releaseSave=resolve;});
  context.fetch=async(...args)=>{await saveWait;return saveFetch(...args);};
  const saving=control('검토본 확정 · 설계 검토').onclick();
  await new Promise(resolve=>setImmediate(resolve));
  assert.equal(byId('workflow-busy').hidden,false);
  assert.equal(byId('workflow-busy-title').textContent,'저장 중');
  releaseSave();await saving;context.fetch=saveFetch;
  assert.equal(byId('workflow-busy').hidden,true);
  assert.equal(requests.at(-1).mode,'implementation','planning approval automatically starts design investigation');
  assert.equal(run("loopConfirmed('planning')"),true);assert.equal(handoffs[0].decisions[0].answer,'반환하지 않음');assert.equal(handoffs[0].title,'회피 시스템 개선');
  assert.ok(!descendants(byId('workflow-controls')).some(n=>n.attributes?.['aria-label']==='변경 요청'));

  response={...result,draft:'설계: 기존 비용 경로 재사용',items:[{title:'비용 처리',requirement:'20 소모',scope:'기존 경로 구현',acceptance:'취소 시 반환 없음',dependencies:[]}],questions:[{...question,id:'D1',title:'설계 선택',scope:'기존 컴포넌트에 배치?'}]};
  await control('AI 설계 검토 · 답변 반영').onclick();
  assert.equal(requests.at(-1).context.planningRecord.path,'.agents/workflow/tasks/test-1.md');
  applyExclusions();
  input('D1 답변').value='기존 컴포넌트 재사용';input('D1 답변').oninput();await control('이 답변으로 결정').onclick();
  response={...response,questions:[]};await control('AI 설계 검토 · 답변 반영').onclick();applyExclusions();
  await control('설계 확정').onclick();
  assert.equal(run("workflowGuideDocument(docs.find(d=>d.path==='.agents/workflow/process/design_review.md')).path"),'.agents/workflow/process/implementation.md','guide follows the active workflow stage');
  assert.equal(run("loopConfirmed('implementation')"),true);assert.equal(handoffs[1].upstream.path,'.agents/workflow/tasks/test-1.md');
  const approvedDesign=run('JSON.stringify(workflowState)');
  run("renderWorkflow('.agents/workflow/process/design_review.md')");
  await run("beginChange('planning',undefined,'회피 정책 변경')");run("renderWorkflow('.agents/workflow/process/design_review.md')");
  input('Q1 답변').value='반환하도록 변경';input('Q1 답변').oninput();
  assert.equal(run("loopConfirmed('planning')"),false);assert.equal(run("loopConfirmed('implementation')"),false);
  assert.equal(run('workflowState.implementation.decisions[0].answer'),'기존 컴포넌트 재사용');
  assert.equal(run('workflowState.planning.decisions[0].history[0].answer'),'반환하지 않음');
  run("renderWorkflow('.agents/workflow/process/implementation.md')");
  assert(!descendants(byId('workflow-controls')).some(n=>n.textContent==='AI 설계 검토 · 답변 반영'),'revoked planning cannot feed stale design');
  run("renderWorkflow('.agents/workflow/process/design_review.md')");
  await control('이 답변으로 결정').onclick();
  response={...result,draft:handoffs[0].result.draft,questions:[]};
  await control('AI 기획서 검토').onclick();applyExclusions();
  await control('검토본 확정 · 설계 검토').onclick();
  assert.equal(requests.at(-1).mode,'implementation','new planning approval triggers design rereview even when draft text is unchanged');
  const confirmedBefore=run('workflowState.planning.confirmation.path');
  const workingFetch=context.fetch;
  context.fetch=async()=>({ok:false,json:async()=>({error:'디스크 저장 실패'})});
  await assert.rejects(run("beginChange('planning',undefined,'회피 정책 변경')"),/디스크 저장 실패/);
  assert.equal(run('workflowState.planning.confirmation.path'),confirmedBefore,'failed handoff must not create a new confirmation');
  context.fetch=workingFetch;
  await run('(async()=>{const route=readRoute;readRoute=()=>{};try{await syncSharedTasks();}finally{readRoute=route;}})()');
  run("renderWorkflow('.agents/workflow/process/design_review.md')");
  response={...result,questions:[],exclusions:[{id:'Q1',reason:'범위 제외',basis:'사용자 요청'}]};await control('AI 기획서 검토').onclick();
  input('기획서 원본').value='추가 변경';input('기획서 원본').oninput();
  const before=run('workflowState.planning.result.draft');applyExclusions();assert.equal(run('workflowState.planning.result.draft'),before,'stale preview rejected');
  assert.equal(input('기획서 원본').value,'추가 변경');
  response={...result,blockers:['에셋 정책 근거 부족']};
  await control('AI 기획서 검토').onclick();applyExclusions();
  assert.equal(run("loopReady('planning')"),false,'missing material cannot be treated as finalizable');
  assert.equal(control('검토본 확정 · 설계 검토').disabled,true);
  assert.ok(!descendants(byId('workflow-controls')).some(n=>['AI가 정리한 사실·조사 근거','확정 전 해결할 자료·충돌'].includes(n.textContent)),'AI reference sections are hidden');
  const originalFetch=context.fetch;context.fetch=async()=>({ok:false,json:async()=>({error:'AI 실패'})});
  await control('AI 기획서 검토').onclick();assert.equal(run('workflowState.planning.decisions[0].answer'),'반환하도록 변경');context.fetch=originalFetch;
  assert.equal(byId('workflow-busy').hidden,true);
  assert.equal(byId('app-shell').inert,false,'failure unlocks the screen for retry');
  const priorDraft=run('workflowState.planning.result.draft');
  response={...result,draft:'오래된 분석 결과'};
  context.fetch=async(...args)=>{
    input('기획서 원본').value='분석 도중 바꾼 의견';input('기획서 원본').oninput();
    return originalFetch(...args);
  };
  await control('AI 기획서 검토').onclick();
  assert.equal(run('workflowState.planning.result.draft'),priorDraft,'changed input rejects automatic integration');
  assert.equal(input('기획서 원본').value,'분석 도중 바꾼 의견');context.fetch=originalFetch;
  const previousTask=run('workflowState.taskId');
  input('새 작업 제목').value='';input('새 작업 제목').oninput();await control('새 작업 만들기').onclick();
  assert.equal(run('workflowState.taskId'),previousTask,'empty input does not create a task');
  input('새 작업 제목').value='새 기획';input('새 작업 제목').oninput();
  assert.equal(run('workflowState.taskId'),previousTask,'typing does not switch tasks');
  await control('새 작업 만들기').onclick();
  assert.notEqual(run('workflowState.taskId'),previousTask);assert.equal(run('workflowState.title'),'새 기획');
  assert.equal(run('workflowState.planning.decisions.length'),0);
  assert.equal(run('workflowState.planning.notes'),'');
  assert.equal(run('otherTasks["'+previousTask+'"].planning.decisions[0].answer'),'반환하도록 변경');
  input('기존 작업 선택').value=previousTask;await input('기존 작업 선택').onchange();
  assert.equal(run('workflowState.planning.decisions[0].answer'),'반환하도록 변경');
  input('새 작업 제목').value='새 기획 2';input('새 작업 제목').oninput();await control('새 작업 만들기').onclick();
  input('기획서 원본').value='회피 비용 20';input('기획서 원본').oninput();
  response={...result,questions:[question]};
  await control('AI 기획서 검토').onclick();applyExclusions();
  input('Q1 답변').value='취소 기능은 이번 범위에서 제외';input('Q1 답변').oninput();
  response={...result,exclusions:[{id:'Q1',reason:'취소 기능 제외',basis:'사용자 추가 요청'}]};
  await control('AI 기획서 검토').onclick();applyExclusions();
  assert.equal(run('workflowState.planning.decisions[0].status'),'excluded');
  assert.equal(run("loopReady('planning')"),true,'excluded unanswered question does not block');
  await control('검토본 확정 · 설계 검토').onclick();
  run("renderWorkflow('.agents/workflow/process/implementation.md')");
  response={...result,questions:[{...question,id:'CHANGE',kind:'planning',scope:'회피 비용을 10으로 변경?'}]};
  await control('AI 설계 검토 · 답변 반영').onclick();applyExclusions();
  input('CHANGE 답변').value='10으로 변경';input('CHANGE 답변').oninput();response={...result,draft:'회피 비용 10'};await control('이 답변으로 결정').onclick();
  assert.equal(requests.at(-1).mode,'planning','planning questions automatically return for planning review');
  assert.equal(run('workflowState.planning.confirmation'),null);
  assert.equal(run('workflowState.planning.decisions.at(-1).answer'),'10으로 변경');
  assert.equal(run('workflowState.planning.decisions.at(-1).confirmed'),true);
  assert.equal(run('workflowState.implementation.decisions[0].status'),'transferred');
  response={...result,draft:'회피 비용 10'};
  await control('AI 기획서 검토').onclick();applyExclusions();
  await control('검토본 확정 · 설계 검토').onclick();
  assert.equal(requests.at(-1).mode,'implementation','new planning automatically receives design rereview');
  run("renderWorkflow('.agents/workflow/process/implementation.md')");
  await control('AI 설계 검토 · 답변 반영').onclick();applyExclusions();
  assert.equal(run("loopReady('implementation')"),true);
  run("renderWorkflow('.agents/workflow/process/design_review.md')");
  await run("beginChange('planning',undefined,'회피 정책 변경')");run("renderWorkflow('.agents/workflow/process/design_review.md')");
  assert.equal(run('workflowState.taskId'),run('workflowState.title'));
  console.log('PASS title task keys, rename, task isolation, scope exclusion, change recovery and planning-change roundtrip');
  const editingTask=run('JSON.stringify(workflowState)');context.approvedDesign=approvedDesign;
  context.executionReport={summary:'실제 변경 요약',changes:['기능 구현'],checks:[{name:'회귀 검사',status:'passed',evidence:'test: exit 0'}],humanChecks:['조작감 확인'],blockers:[]};
  run("workflowState=JSON.parse(approvedDesign);workflowState.execution={revision:2,status:'review',phase:'implement',report:executionReport,diff:'+ change',decisions:[]};renderWorkflow('.agents/workflow/process/design_review.md')");
  assert.ok(control('코드 리뷰 승인'));assert.ok(!input('기획서 원본'),'only the current judgment is shown');
  const normalFetch=context.fetch;let executionAction;
  context.fetch=async(url,options)=>{if(!url.endsWith('/execution'))return normalFetch(url,options);executionAction=JSON.parse(options.body);return{ok:true,json:async()=>({revision:executionAction.expectedRevision+1,status:executionAction.action==='finish'?'complete':executionAction.action==='accept'?'cleanup':'running',phase:'verify',closure:executionAction.action==='finish'?{...executionAction.closure,actor:executionAction.actor,at:'2026-09-22'}:undefined,report:context.executionReport,decisions:[]})};};
  input('승인자 이름').value='리뷰어';input('승인자 이름').oninput();
  await control('코드 리뷰 승인').onclick();assert.equal(executionAction.actor,'리뷰어');assert.equal(executionAction.action,'approve');assert.equal(run('currentWorkStage()'),'testing');
  assert.equal(run("workflowGuideDocument(docs.find(d=>d.path==='.agents/workflow/process/design_review.md')).path"),'.agents/workflow/process/testing.md','guide follows the active workflow stage');
  run("workflowState.execution.status='acceptance';renderWorkflow('.agents/workflow/process/design_review.md')");
  const humanCheck=descendants(byId('workflow-controls')).find(n=>n.tagName==='INPUT'&&n.type==='checkbox');humanCheck.checked=true;humanCheck.onchange();
  input('승인자 이름').value='테스터';input('승인자 이름').oninput();
  input('판단 의견').value='직접 확인 완료';input('판단 의견').oninput();
  run("renderWorkflow('.agents/workflow/process/design_review.md')");assert.equal(input('판단 의견').value,'직접 확인 완료','judgment survives rerender');
  await control('테스트 완료').onclick();assert.equal(executionAction.action,'accept');assert.deepEqual(executionAction.confirmedChecks,[0]);assert.equal(run('currentWorkStage()'),'completion');
  assert.equal(run("workflowGuideDocument(docs.find(d=>d.path==='.agents/workflow/process/design_review.md')).path"),'.agents/workflow/process/completion.md','guide follows the active workflow stage');
  assert.equal(executionAction.actor,'테스터');assert.equal(run('dashboardStage(workflowState)'),'cleanup');
  input('승인자 이름').value='정리 확인자';input('승인자 이름').oninput();
  input('Wiki 처리').value='skipped';input('Wiki 처리').onchange();
  input('Wiki 반영 근거 또는 생략 사유').value='추가 지식 없음';input('Wiki 반영 근거 또는 생략 사유').oninput();
  input('자료 정리 결과').value='판단 원본 보존';input('자료 정리 결과').oninput();
  run("renderWorkflow('.agents/workflow/process/completion.md')");assert.equal(input('자료 정리 결과').value,'판단 원본 보존');
  await control('정리 완료').onclick();assert.equal(executionAction.action,'finish');assert.equal(executionAction.closure.wikiStatus,'skipped');assert.equal(executionAction.actor,'정리 확인자');assert.equal(run('dashboardStage(workflowState)'),'completion');
  assert.ok(!input('판단 의견'),'completion has no more input');context.fetch=normalFetch;
  context.editingTask=editingTask;run("workflowState=JSON.parse(editingTask);renderWorkflow('.agents/workflow/process/design_review.md')");
  context.localStorage.setItem=()=>{throw new Error('quota')};input('기획서 원본').value='저장 실패 입력';input('기획서 원본').oninput();assert.match(run('workflowStorageError'),/저장하지 못했습니다/);
  assert.equal(input('기획서 원본').value,'저장 실패 입력');
  const model=fs.readFileSync(path.join(root,'.agents/scripts/wiki-viewer/workflow-model.js'),'utf8');
  const workflow=fs.readFileSync(path.join(root,'.agents/scripts/wiki-viewer/workflow.js'),'utf8');
  console.log('PASS planning/design iteration, retained answers, AI rereview, explicit confirmation, handoff chain, invalidation, stale preview, failures');
})().catch(error=>{console.error(error);process.exitCode=1;});
