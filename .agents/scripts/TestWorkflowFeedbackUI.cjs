// Copyright Woogle. All Rights Reserved.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const elements=new Map(),storage=new Map();
class Element{
  constructor(tag){this.tagName=tag.toUpperCase();this.children=[];this.attributes={};this.value='';}
  set id(value){this._id=value;elements.set(value,this);}get id(){return this._id;}
  append(...nodes){for(const node of nodes){node.parentElement=this;this.children.push(node);}}
  replaceChildren(...nodes){this.children=[];this.append(...nodes);}
  setAttribute(name,value){this.attributes[name]=value;}getAttribute(name){return this.attributes[name];}
}
function descendants(node){return [node,...node.children.flatMap(descendants)];}
function byId(id){if(!elements.has(id)){const node=new Element('div');node.id=id;}return elements.get(id);}
function el(tag,text,className){const node=new Element(tag);node.textContent=text;node.className=className;return node;}
const item={title:'저장·복원',path:'.agents/workflow/tasks/example.md',next:'재개 후 좌표 확인'},newPath='.agents/workflow/tasks/boss-hp-1234abcd.md';
const row=(item,owner,result,evidence='')=>({item,method:item+' 확인 방법',owner,result,evidence});
// 가짜 서버: 작업마다 질문·계획·체크리스트·최근 AI 처리를 들고 있고, 서버처럼 AI가 작업 중이거나 기록 해시가 다르면 거절한다.
const tasks={[item.path]:{title:item.title,next:item.next,request:'',questions:[],plan:{text:'',approval:''},checklist:[row('빌드','AI','통과','exit 0'),row('저장 후 복원','사람','대기'),row('부활 시 적 재생성','사람','대기')],revision:0,latest:null}};
const t=tasks[item.path];
let providers=[{id:'codex',label:'Codex'},{id:'claude',label:'Claude Code'},{id:'gemini',label:'Gemini CLI'}];
let starts=0,reads=0,loss=false,refuse=false,storageBroken=false,savedRequest,sequence=0,recordsRendered=0;const terminals=[];
const view=taskPath=>({taskPath,providers,revision:tasks[taskPath].revision,latest:tasks[taskPath].latest,taskHash:'task-'+tasks[taskPath].revision});
const api=async(route,body)=>{
  assert.equal(route,'/test-feedback');
  body=JSON.parse(JSON.stringify(body));
  if(body.action==='read'){reads++;const task=tasks[body.taskPath];return {...view(body.taskPath),title:task.title,next:task.next||'',request:task.request,questions:task.questions,plan:task.plan,checklist:task.checklist,checklistError:''};}
  // 서버처럼 처리한 적이 없는 작업은 기록 목록에 없다.
  if(body.action==='list')return {records:Object.fromEntries(Object.keys(tasks).filter(taskPath=>tasks[taskPath].latest).map(taskPath=>[taskPath,view(taskPath)])),providers,tasks:[{path:item.path,title:item.title,state:'확인 대기',summary:'체크리스트 1/3 통과',next:item.next,modified:''}]};
  if(body.action==='terminal'){terminals.push(body);return {opened:true};}
  if(refuse||Object.values(tasks).some(task=>task.latest?.status==='running'))throw Error('다른 AI가 작업 중입니다. 입력은 유지됩니다. 잠시 후 전달하세요.');
  const taskPath=body.action==='create'?newPath:body.taskPath;
  if(body.action==='create')tasks[newPath]={title:body.title,request:'- 요청자: '+body.actor+'\n\n> '+body.request,questions:[],plan:{text:'',approval:''},checklist:[],revision:0,latest:null};
  else if(body.taskHash!=='task-'+tasks[taskPath].revision)throw Error('작업 기록이 바뀌었습니다. 최신 상태를 불러와 확인 후 전달하세요.');
  const task=tasks[taskPath];starts++;task.revision++;savedRequest=structuredClone(body);
  // 서버처럼 테스트 결과는 실패가 있으면 AI 수정, 모두 통과면 그 자리에서 완료, 나머지는 결과만 기록한다.
  const checks=body.checks?.map(check=>({item:task.checklist[check.index].item,result:check.result,note:check.note}));
  const after=task.checklist.map((row,index)=>({...row,result:body.checks?.find(check=>check.index===index)?.result||row.result}));
  const outcome=body.action!=='submit'||checks.some(check=>check.result==='실패')?{status:'running'}:after.every(row=>row.result==='통과')?{kind:'record',status:'complete'}:{kind:'record',status:'recorded'};
  task.latest={...body,checks,...outcome,at:'2026-09-25',startedAt:new Date(Date.now()-3*60000).toISOString(),report:null,error:''};
  if(loss){loss=false;throw Error('연결 끊김');}
  return {...view(taskPath),taskPath};
};
const blocked=()=>{if(storageBroken)throw Error('저장소 차단');};
const context=vm.createContext({document:{},$:byId,el,workflowKey:'test',data:{ai:{},documents:[]},location:{hash:''},fetch:()=>{},taskRecordFilter:'waiting',liveTaskRecords:null,
  localStorage:{getItem:k=>{blocked();return storage.get(k)||null;},setItem:(k,v)=>{blocked();storage.set(k,v);},removeItem:k=>{blocked();storage.delete(k);}},
  workflowRequest:api,workflowButton:(label,click)=>{const node=el('button',label);node.onclick=click;return node;},requestId:()=>String(++sequence),renderTaskRecords:()=>{recordsRendered++;}
});
vm.runInContext(fs.readFileSync(path.join(__dirname,'wiki-viewer/test-feedback.js'),'utf8'),context);
const run=code=>vm.runInContext(code,context);context.item=item;
const panelNodes=()=>descendants(byId('test-feedback-panel'));
const input=label=>panelNodes().find(node=>node.attributes['aria-label']===label);
const button=label=>panelNodes().find(node=>node.tagName==='BUTTON'&&node.textContent===label);
const text=id=>descendants(byId(id)).map(node=>node.textContent).join('\n');
const type=(label,value)=>{const field=input(label);field.value=value;field.oninput();};
const choose=label=>input(label).onchange();
const noteRow=name=>input(name+' 문제 상황과 재현 방법').parentElement;
const message=()=>byId('test-feedback-message').textContent;
const aiChoice=()=>byId('ai-provider');
const markOf=title=>descendants(byId('task-phase')).find(node=>String(node.className).startsWith('check-item')&&node.children.some(child=>child.tagName==='STRONG'&&child.textContent===title)).children[0].className;
const held=()=>[...storage.keys()].some(key=>key.includes('pending'));
(async()=>{
  // 체크리스트 단계: 사람 항목만 고르고, 실패에는 재현 방법을 적는다.
  await run('openTaskPanel(item)');
  assert.equal(byId('test-feedback-panel').hidden,false);assert.equal(byId('test-feedback-submit').textContent,'테스트 결과 전달');
  assert.equal(text('task-next'),'다음 행동 · 재개 후 좌표 확인','the panel shows the recorded next action');
  assert.ok(!descendants(byId('task-phase')).some(node=>node.attributes['aria-label']?.startsWith('빌드 ')),'AI rows are read-only');
  assert.deepEqual(descendants(byId('task-phase')).filter(node=>node.tagName==='H3').map(node=>node.textContent),['사람이 확인할 항목 · 2','AI가 확인한 항목 · 1'],'human work is listed before AI results');
  assert.ok(descendants(byId('task-phase')).some(node=>node.textContent==='통과 · exit 0'));
  assert.equal(markOf('빌드'),'check-mark pass','each item is a box whose check shows the result');assert.equal(markOf('저장 후 복원'),'check-mark wait');
  assert.equal(input('저장 후 복원 이번에 확인 안 함').checked,true);assert.equal(noteRow('저장 후 복원').hidden,true);
  assert.ok(!input('처리할 AI'),'the AI is chosen once at the top of the page, not per panel');
  // 폴링은 바뀐 것이 없으면 패널과 목록을 다시 그리지 않는다. 처리한 적 없는 작업의 revision은 0으로 본다.
  const readsBefore=reads;await run('loadTaskJobs()');assert.equal(reads,readsBefore,'a task without a server record is not reloaded on every poll');
  const rendered=recordsRendered;await run('loadTaskJobs()');assert.equal(recordsRendered,rendered,'the list is redrawn only when records change');
  assert.equal(aiChoice().children.length,3);aiChoice().value='claude';aiChoice().onchange();
  await run("sendTaskAction('submit')");assert.equal(starts,0);assert.match(message(),/하나 이상/);
  choose('저장 후 복원 실패');assert.equal(noteRow('저장 후 복원').hidden,false);assert.equal(markOf('저장 후 복원'),'check-mark fail','the chosen result fills the check box');
  choose('저장 후 복원 이번에 확인 안 함');assert.equal(markOf('저장 후 복원'),'check-mark wait');choose('저장 후 복원 실패');
  await run("sendTaskAction('submit')");assert.equal(starts,0);assert.match(message(),/실패한 항목/);
  type('저장 후 복원 문제 상황과 재현 방법','저장 → 재개 후 원점으로 이동');choose('부활 시 적 재생성 통과');
  await run("sendTaskAction('submit')");assert.equal(starts,0);assert.match(message(),/이름을 입력/);
  type('이름','테스터');
  await run('openTaskPanel(item)');
  assert.equal(input('이름').value,'테스터');assert.equal(input('저장 후 복원 실패').checked,true);assert.equal(input('부활 시 적 재생성 통과').checked,true);
  assert.match(input('저장 후 복원 문제 상황과 재현 방법').value,/원점/);assert.equal(aiChoice().value,'claude');
  await byId('test-feedback-submit').onclick();assert.equal(starts,1);
  assert.deepEqual(savedRequest.checks,[{index:1,result:'실패',note:'저장 → 재개 후 원점으로 이동'},{index:2,result:'통과',note:''}]);
  assert.equal(savedRequest.action,'submit');assert.equal(savedRequest.actor,'테스터');assert.equal(savedRequest.provider,'claude');
  assert.ok(!Object.hasOwn(savedRequest,'expectedRevision'),'the record hash alone guards stale sends');assert.ok(!held(),'no request is held back in the browser');
  assert.equal(byId('test-feedback-submit').disabled,true);assert.deepEqual(JSON.parse(JSON.stringify(run('taskDraft(item.path).checks'))),{},'sent choices are cleared');
  assert.match(text('task-phase'),/AI가 처리하는 동안/);assert.match(text('test-feedback-result'),/Claude Code 처리 중 · 3분 경과 · 터미널 창/);
  assert.equal(byId('task-terminal').disabled,true,'a running task cannot be continued in a terminal');assert.equal(byId('task-terminal').hidden,false,'an open task can be continued in a terminal');
  t.checklist=[row('빌드','AI','통과','exit 0'),row('저장 후 복원','사람','대기','좌표 복원 순서 수정 후 재확인'),row('부활 시 적 재생성','사람','통과','테스터')];
  t.latest={...t.latest,status:'retest',report:{summary:'복원 좌표 수정',changes:['좌표 복원 순서 수정'],evidence:['회귀 테스트 통과']}};t.revision++;
  await run('loadTaskJobs()');assert.equal(byId('test-feedback-submit').disabled,false);assert.equal(byId('test-feedback-fields').disabled,false);
  assert.match(message(),/AI 처리가 끝났습니다 · 사람 확인 필요/,'the end of a run is announced');
  assert.equal(run('liveTaskRecords[0].summary'),'체크리스트 1/3 통과','the server task list replaces the generated snapshot');assert.ok(recordsRendered>rendered);
  assert.match(text('test-feedback-result'),/실패 · 저장 후 복원: 저장 → 재개 후 원점으로 이동/);assert.match(text('test-feedback-result'),/AI 처리 근거 · 변경 1 · 근거 1/);
  assert.match(text('test-feedback-result'),/최근 전달 · 테스터 · /);assert.doesNotMatch(text('test-feedback-result'),/ · 2026-09-25 · /,'times are shown in local time');
  assert.match(text('task-phase'),/좌표 복원 순서 수정 후 재확인/);
  assert.equal(input('저장 후 복원 이번에 확인 안 함').checked,true,'a new round starts without previous choices');
  await run("sendTaskAction('submit')");assert.equal(starts,1);assert.match(message(),/하나 이상/);
  choose('저장 후 복원 실패');type('저장 후 복원 문제 상황과 재현 방법','여전히 원점');choose('저장 후 복원 통과');assert.equal(noteRow('저장 후 복원').hidden,true);
  refuse=true;await run("sendTaskAction('submit')");assert.ok(!held());assert.equal(input('저장 후 복원 통과').checked,true);assert.equal(byId('test-feedback-fields').disabled,false);
  refuse=false;await run("sendTaskAction('submit')");assert.equal(starts,2);assert.deepEqual(savedRequest.checks,[{index:1,result:'통과',note:''}],'hidden failure notes must not be submitted for a passed item');
  assert.match(message(),/모든 항목이 통과해 완료했습니다/,'passing the last item completes the task at once');
  t.checklist=t.checklist.map(r=>({...r,result:'통과'}));t.revision++;await run('loadTaskJobs()');
  assert.match(text('task-phase'),/모든 항목이 통과해 완료된 작업입니다/);assert.equal(byId('test-feedback-submit').hidden,true,'a completed task has nothing to send');
  assert.equal(byId('task-request').hidden,true,'a completed task takes no extra requests');assert.equal(byId('task-terminal').hidden,true,'a completed task is not continued in a terminal');
  // 일부만 통과하면 AI 없이 결과만 기록한다.
  t.checklist=[row('빌드','AI','통과','exit 0'),row('저장 후 복원','사람','대기'),row('부활 시 적 재생성','사람','대기')];t.latest={...t.latest,status:'complete'};t.revision++;await run('loadTaskJobs()');
  choose('저장 후 복원 통과');await byId('test-feedback-submit').onclick();assert.equal(t.latest.status,'recorded');assert.match(message(),/테스트 결과를 기록했습니다/);
  assert.match(text('test-feedback-result'),/최근 전달 · 테스터 · /);assert.doesNotMatch(text('test-feedback-result'),/처리 AI/,'a record-only submission names no processing AI');
  t.latest={...t.latest,status:'failed',error:'로그인 확인 필요'};t.revision++;await run('loadTaskJobs()');
  aiChoice().value='gemini';aiChoice().onchange();
  const retry=byId('test-feedback-result').children.find(node=>node.textContent==='저장된 요청으로 AI 다시 시도');assert.ok(retry);await retry.onclick();assert.equal(savedRequest.action,'retry');
  assert.equal(savedRequest.provider,'gemini');assert.ok(!Object.hasOwn(savedRequest,'actor'),'a retry reuses the stored request');
  t.latest={...t.latest,status:'failed'};t.revision++;providers=[providers[0]];await run('loadTaskJobs()');
  assert.equal(aiChoice().value,'gemini','unavailable selection must not silently fall back to another AI');
  const before=starts;await run("sendTaskAction('retry')");assert.equal(starts,before);assert.match(message(),/연결되어 있지/);
  providers=undefined;await run('refreshTaskContext()');assert.equal(run('availableProviders().length'),1,'old server capabilities allow only the original Codex route');
  // 사람 항목은 모두 통과했는데 AI 항목이 남으면 추가 요청으로 이어가라고 안내한다.
  t.checklist=[row('빌드','AI','실패','exit 1'),row('저장 후 복원','사람','통과','테스터')];t.latest={...t.latest,status:'issues'};t.revision++;await run('refreshTaskContext()');
  assert.match(text('task-phase'),/AI 항목이 남아 완료되지 않았습니다/);assert.equal(byId('task-request').hidden,false);

  // 질문 단계: 선택지를 고르거나(고른 것은 선택 안 함으로 되돌릴 수 있다) 직접 적고, 모든 질문에 답해야 전달된다.
  providers=[{id:'codex',label:'Codex'},{id:'claude',label:'Claude Code'},{id:'gemini',label:'Gemini CLI'}];t.checklist=[];
  t.questions=[{id:'Q0',question:'이전 질문',options:[],recommendation:'',answer:'A · 테스터 2026-09-25'},{id:'Q1',question:'감소 방식?',options:['즉시','지연'],recommendation:'지연 · 피격 확인이 쉽다',answer:''},{id:'Q2',question:'적용 범위?',options:[],recommendation:'',answer:''}];
  t.latest={...t.latest,status:'questions',error:''};t.revision++;await run('refreshTaskContext()');
  assert.equal(byId('test-feedback-submit').textContent,'답변 전달');assert.equal(byId('test-feedback-submit').hidden,false);
  assert.match(text('task-phase'),/AI 질문 · 2/);assert.match(text('task-phase'),/추천 · 지연 · 피격 확인이 쉽다/);assert.match(text('task-phase'),/답한 질문 · 1/);
  assert.equal(input('Q1 지연').checked,false,'the recommendation is not preselected');assert.equal(input('Q1 선택 안 함').checked,true);assert.ok(!input('Q2 선택 안 함'),'free-text questions have no choices');
  assert.match(text('task-request'),/읽기 전용으로 조사/,'extra requests before approval are read-only research');
  await byId('test-feedback-submit').onclick();assert.match(message(),/답하지 않은 질문이 있습니다: Q1, Q2/);
  choose('Q1 지연');choose('Q1 선택 안 함');assert.equal(run('taskDraft(item.path).answers.Q1.choice'),'','a chosen option can be cleared');
  assert.equal(input('Q1 직접 답변').placeholder,'선택지에 덧붙이거나 다르게 답할 내용');assert.equal(input('Q2 답변').placeholder,'이 질문에 대한 답을 적어주세요.','a free-text question has its own hint and name');assert.ok(!input('Q2 직접 답변'));
  choose('Q1 지연');type('Q1 직접 답변','0.5초 뒤');type('Q2 답변','보스만');
  const beforeAnswer=starts;await byId('test-feedback-submit').onclick();assert.equal(starts,beforeAnswer+1);
  assert.equal(savedRequest.action,'answer');assert.deepEqual(savedRequest.answers,[{id:'Q1',answer:'지연 — 0.5초 뒤'},{id:'Q2',answer:'보스만'}]);assert.equal(savedRequest.actor,'테스터');
  assert.deepEqual(JSON.parse(JSON.stringify(run('taskDraft(item.path).answers'))),{},'sent answers are cleared');

  // 승인 단계: 계획을 보여주고 권한을 알린 뒤 승인만 보낸다.
  t.questions=t.questions.map(q=>({...q,answer:q.answer||'답 · 테스터'}));t.plan={text:'1. 지연 감소 추가\n- 검증: 자동화 테스트',approval:''};t.latest={...t.latest,status:'approval'};t.revision++;
  await run('loadTaskJobs()');
  assert.equal(byId('test-feedback-submit').textContent,'구현 승인');assert.match(text('task-phase'),/1\. 지연 감소 추가\n- 검증: 자동화 테스트/);assert.match(text('task-phase'),/모든 명령/);
  assert.equal(byId('task-plan').hidden,true,'a plan waiting for approval is shown in the phase, not folded');
  await byId('test-feedback-submit').onclick();assert.equal(savedRequest.action,'approve');assert.equal(savedRequest.actor,'테스터');
  assert.ok(!['answers','checks','message'].some(key=>Object.hasOwn(savedRequest,key)),'approval sends no other input');

  // 추가 요청과 터미널 이어하기, 할 일이 없는 단계.
  t.plan={...t.plan,approval:'테스터 2026-09-25'};t.latest={...t.latest,status:'empty',report:{summary:'설명만 필요한 요청이었습니다.',changes:[],evidence:['읽은 파일']}};t.revision++;
  await run('loadTaskJobs()');
  assert.equal(byId('test-feedback-submit').hidden,true);assert.match(text('task-phase'),/지금 답하거나 확인할 항목이 없습니다/);
  assert.equal(byId('task-plan').hidden,false);assert.match(text('task-plan'),/승인된 구현 계획 · 테스터 2026-09-25\n1\. 지연 감소 추가/,'the approved plan stays readable, folded');
  assert.match(text('test-feedback-result'),/처리 결과 확인/);assert.match(text('test-feedback-result'),/설명만 필요한 요청이었습니다/,'the latest result shows its summary');
  assert.match(text('task-request'),/모든 명령을 허용/);
  // 계획이 바뀌어 승인 줄이 사라지면 체크리스트가 있어도 추가 요청은 다시 읽기 전용이다.
  t.plan={...t.plan,approval:''};t.checklist=[row('빌드','AI','통과','exit 0')];t.revision++;await run('refreshTaskContext()');
  assert.match(text('task-request'),/읽기 전용으로 조사/,'a plan waiting for re-approval is announced as read-only');
  t.plan={...t.plan,approval:'테스터 2026-09-25'};t.checklist=[];t.revision++;await run('refreshTaskContext()');
  await button('추가 요청 전달').onclick();assert.match(message(),/추가 요청을 입력/);
  type('추가 요청','모든 적에게도 적용');await button('추가 요청 전달').onclick();
  assert.equal(savedRequest.action,'request');assert.equal(savedRequest.message,'모든 적에게도 적용');assert.equal(run('taskDraft(item.path).message'),'');
  // 응답을 받지 못하면 입력을 그대로 두고 잠그지 않는다. 다시 보내도 서버가 먼저 받은 요청을 처리하는 중이라 거절한다.
  t.latest={...t.latest,status:'empty'};t.revision++;await run('loadTaskJobs()');
  type('추가 요청','보스에게만 적용');loss=true;const beforeLost=starts;await button('추가 요청 전달').onclick();assert.equal(starts,beforeLost+1);
  assert.match(message(),/연결 끊김/);assert.equal(run('taskDraft(item.path).message'),'보스에게만 적용','a lost response keeps the input');
  assert.equal(byId('task-request-send').disabled,false,'a lost response does not lock the inputs');assert.ok(!held());
  await button('추가 요청 전달').onclick();assert.equal(starts,beforeLost+1,'the resend is refused while the first request runs');assert.match(message(),/다른 AI/);
  t.latest={...t.latest,status:'retest'};t.revision++;await run('loadTaskJobs()');assert.equal(byId('task-terminal').disabled,false);
  const opening=terminals.length;await Promise.all([byId('task-terminal').onclick(),byId('task-terminal').onclick()]);assert.equal(terminals.length,opening+1,'a double click opens one terminal');
  assert.deepEqual(terminals.at(-1),{action:'terminal',taskPath:item.path,provider:'gemini'});assert.match(message(),/Gemini CLI 터미널 창을 열었습니다/);

  // 새 작업: 브라우저 저장소를 못 써도 입력을 메모리로 이어가고, 만든 뒤 작업 진행 패널로 넘어간다.
  storageBroken=true;
  run('openNewTask()');
  assert.equal(descendants(byId('test-feedback-panel')).find(node=>node.tagName==='H2').textContent,'새 작업');assert.equal(input('이름').value,'테스터','the name is remembered across tasks');
  await button('AI에게 전달').onclick();assert.match(message(),/제목을 입력/);
  type('제목','Boss HP');await button('AI에게 전달').onclick();assert.match(message(),/요청을 입력/);
  type('요청','보스 체력바를 지연 감소로');
  run('openNewTask()');assert.equal(input('제목').value,'Boss HP','the new task draft is kept without browser storage');
  const beforeCreate=starts;await button('AI에게 전달').onclick();assert.equal(starts,beforeCreate+1);
  assert.deepEqual(JSON.parse(JSON.stringify(savedRequest)),{action:'create',operationId:savedRequest.operationId,title:'Boss HP',request:'보스 체력바를 지연 감소로',actor:'테스터',provider:'gemini'},'a new task also follows the AI chosen at the top');
  assert.equal(run("storageGet(taskKey('new'))"),null,'a created task clears its draft');
  assert.equal(run('taskSelected.path'),newPath);assert.equal(descendants(byId('test-feedback-panel')).find(node=>node.tagName==='H2').textContent,'Boss HP');
  assert.match(message(),/새 작업을 만들었습니다/);assert.equal(run('taskRecordFilter'),'active');assert.match(text('task-phase'),/AI가 처리하는 동안/);
  assert.match(text('task-origin'),/보스 체력바를 지연 감소로/);
  // 새 작업도 응답을 받지 못하면 입력을 그대로 두고, 다시 보내면 조사 중이라 거절된다.
  tasks[newPath].latest.status='questions';
  run('openNewTask()');type('제목','Second');type('요청','두 번째 요청');loss=true;
  const beforeLostCreate=starts;await button('AI에게 전달').onclick();assert.equal(starts,beforeLostCreate+1);assert.match(message(),/연결 끊김/);
  assert.equal(byId('new-task-fields').disabled,false);assert.equal(input('제목').value,'Second');
  await button('AI에게 전달').onclick();assert.equal(starts,beforeLostCreate+1,'a resent creation is refused while the first one is researched');assert.match(message(),/다른 AI/);
  console.log('PASS task panel phases (checklist, questions, approval, complete, empty), next action and folded approved plan, instant completion and record-only messages, read-only AI rows, required failure notes and answers, clearable choices, draft preservation without browser storage, remembered name, lost responses keep inputs without resend, quiet polling, running lock, result refresh and announcement, local times, retry, additional requests (read-only before approval, hidden after completion), AI-only failure guidance and single terminal opening');
})().catch(error=>{console.error(error);process.exitCode=1;});
