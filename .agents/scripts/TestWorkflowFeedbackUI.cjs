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
// 가짜 서버: 작업마다 질문·계획·체크리스트·최근 AI 처리를 들고 있고, 같은 접수는 한 번만 시작한다.
const tasks={[item.path]:{title:item.title,request:'',questions:[],plan:{text:'',approval:''},checklist:[row('빌드','AI','통과','exit 0'),row('저장 후 복원','사람','대기'),row('부활 시 적 재생성','사람','대기')],derived:false,revision:0,latest:null}};
const t=tasks[item.path];
let providers=[{id:'codex',label:'Codex'},{id:'claude',label:'Claude Code'},{id:'gemini',label:'Gemini CLI'}];
let starts=0,loss=false,refuse=false,savedRequest,sequence=0,recordsRendered=0;const terminals=[];
const view=taskPath=>({taskPath,providers,revision:tasks[taskPath].revision,latest:tasks[taskPath].latest,taskHash:'task-'+tasks[taskPath].revision});
const api=async(route,body)=>{
  assert.equal(route,'/test-feedback');
  body=JSON.parse(JSON.stringify(body));
  if(body.action==='read'){const task=tasks[body.taskPath];return {...view(body.taskPath),title:task.title,request:task.request,questions:task.questions,plan:task.plan,checklist:task.checklist,derived:task.derived,checklistError:''};}
  if(body.action==='list')return {records:Object.fromEntries(Object.keys(tasks).map(taskPath=>[taskPath,view(taskPath)])),providers,tasks:[{path:item.path,title:item.title,state:'확인 대기',summary:'체크리스트 1/3 통과',next:item.next,modified:''}]};
  if(body.action==='terminal'){terminals.push(body);return {opened:true};}
  if(refuse){const error=Error('다른 AI 작업 중');error.responded=true;throw error;}
  const taskPath=body.action==='create'?newPath:body.taskPath;
  if(!savedRequest||savedRequest.operationId!==body.operationId){
    if(body.action==='create')tasks[newPath]={title:body.title,request:'- 요청자: '+body.actor+'\n\n> '+body.request,questions:[],plan:{text:'',approval:''},checklist:[],derived:false,revision:0,latest:null};
    else assert.equal(body.expectedRevision,tasks[taskPath].revision);
    const task=tasks[taskPath];starts++;task.revision++;savedRequest=structuredClone(body);
    // 서버처럼 테스트 결과는 실패가 있으면 AI 수정, 모두 통과면 완료 뒤 정리, 나머지는 결과만 기록한다.
    const checks=body.checks?.map(check=>({item:task.checklist[check.index].item,result:check.result,note:check.note}));
    const after=task.checklist.map((row,index)=>({...row,result:body.checks?.find(check=>check.index===index)?.result||row.result}));
    const outcome=body.action!=='submit'||checks.some(check=>check.result==='실패')?{status:'running'}:after.every(row=>row.result==='통과')?{action:'cleanup',kind:'cleanup',status:'running'}:{status:'recorded'};
    task.latest={...body,checks,...outcome,at:'2026-09-25',startedAt:new Date(Date.now()-3*60000).toISOString(),report:null,error:''};
  }else assert.deepEqual(body,savedRequest,'unknown response retries must use the original request');
  if(loss){loss=false;throw Error('연결 끊김');}
  return {...view(taskPath),taskPath};
};
const context=vm.createContext({document:{},$:byId,el,workflowKey:'test',data:{ai:{},documents:[]},location:{hash:''},fetch:()=>{},taskRecordFilter:'waiting',liveTaskRecords:null,
  localStorage:{getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},
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
const markOf=title=>descendants(byId('task-phase')).find(node=>String(node.className).startsWith('check-item')&&node.children.some(child=>child.tagName==='STRONG'&&child.textContent===title)).children[0].className;
(async()=>{
  // 체크리스트 단계: 사람 항목만 고르고, 실패에는 재현 방법을 적는다.
  await run('openTaskPanel(item)');
  assert.equal(byId('test-feedback-panel').hidden,false);assert.equal(byId('test-feedback-submit').textContent,'테스트 결과 전달');
  assert.ok(!descendants(byId('task-phase')).some(node=>node.attributes['aria-label']?.startsWith('빌드 ')),'AI rows are read-only');
  assert.deepEqual(descendants(byId('task-phase')).filter(node=>node.tagName==='H3').map(node=>node.textContent),['사람이 확인할 항목 · 2','AI가 확인한 항목 · 1'],'human work is listed before AI results');
  assert.ok(descendants(byId('task-phase')).some(node=>node.textContent==='통과 · exit 0'));
  assert.equal(markOf('빌드'),'check-mark pass','each item is a box whose check shows the result');assert.equal(markOf('저장 후 복원'),'check-mark wait');
  assert.equal(input('저장 후 복원 이번에 확인 안 함').checked,true);assert.equal(noteRow('저장 후 복원').hidden,true);
  assert.equal(input('처리할 AI').children.length,3);input('처리할 AI').value='claude';input('처리할 AI').onchange();
  await run("sendTaskAction('submit')");assert.equal(starts,0);assert.match(message(),/하나 이상/);
  choose('저장 후 복원 실패');assert.equal(noteRow('저장 후 복원').hidden,false);assert.equal(markOf('저장 후 복원'),'check-mark fail','the chosen result fills the check box');
  choose('저장 후 복원 이번에 확인 안 함');assert.equal(markOf('저장 후 복원'),'check-mark wait');choose('저장 후 복원 실패');
  await run("sendTaskAction('submit')");assert.equal(starts,0);assert.match(message(),/실패한 항목/);
  type('저장 후 복원 문제 상황과 재현 방법','저장 → 재개 후 원점으로 이동');choose('부활 시 적 재생성 통과');
  await run("sendTaskAction('submit')");assert.equal(starts,0);assert.match(message(),/이름을 입력/);
  type('이름','테스터');
  await run('openTaskPanel(item)');
  assert.equal(input('이름').value,'테스터');assert.equal(input('저장 후 복원 실패').checked,true);assert.equal(input('부활 시 적 재생성 통과').checked,true);
  assert.match(input('저장 후 복원 문제 상황과 재현 방법').value,/원점/);assert.equal(input('처리할 AI').value,'claude');
  loss=true;await byId('test-feedback-submit').onclick();assert.equal(starts,1);
  assert.deepEqual(savedRequest.checks,[{index:1,result:'실패',note:'저장 → 재개 후 원점으로 이동'},{index:2,result:'통과',note:''}]);
  assert.equal(savedRequest.action,'submit');assert.equal(savedRequest.actor,'테스터');assert.equal(savedRequest.provider,'claude');
  assert.ok(storage.has('test:test-feedback:pending:'+item.path));assert.equal(byId('test-feedback-fields').disabled,true);assert.match(message(),/보존/);
  await run("sendTaskAction('submit')");assert.equal(starts,1);assert.equal(storage.has('test:test-feedback:pending:'+item.path),false);
  assert.equal(byId('test-feedback-submit').disabled,true);assert.deepEqual(JSON.parse(JSON.stringify(run('taskDraft(item.path).checks'))),{},'sent choices are cleared');
  assert.match(text('task-phase'),/AI가 처리하는 동안/);assert.match(text('test-feedback-result'),/Claude Code 처리 중 · 3분 경과 · 터미널 창/);
  assert.equal(byId('task-terminal').disabled,true,'a running task cannot be continued in a terminal');
  t.checklist=[row('빌드','AI','통과','exit 0'),row('저장 후 복원','사람','대기','좌표 복원 순서 수정 후 재확인'),row('부활 시 적 재생성','사람','통과','테스터')];
  t.latest={...t.latest,status:'retest',report:{summary:'복원 좌표 수정',changes:['좌표 복원 순서 수정'],evidence:['회귀 테스트 통과']}};t.revision++;
  await run('loadTaskJobs()');assert.equal(byId('test-feedback-submit').disabled,false);assert.equal(byId('test-feedback-fields').disabled,false);
  assert.equal(run('liveTaskRecords[0].summary'),'체크리스트 1/3 통과','the server task list replaces the generated snapshot');assert.ok(recordsRendered>0);
  assert.match(text('test-feedback-result'),/실패 · 저장 후 복원: 저장 → 재개 후 원점으로 이동/);assert.match(text('test-feedback-result'),/AI 처리 근거 · 변경 1 · 근거 1/);
  assert.match(text('task-phase'),/좌표 복원 순서 수정 후 재확인/);
  assert.equal(input('저장 후 복원 이번에 확인 안 함').checked,true,'a new round starts without previous choices');
  await run("sendTaskAction('submit')");assert.equal(starts,1);assert.match(message(),/하나 이상/);
  choose('저장 후 복원 실패');type('저장 후 복원 문제 상황과 재현 방법','여전히 원점');choose('저장 후 복원 통과');assert.equal(noteRow('저장 후 복원').hidden,true);
  refuse=true;await run("sendTaskAction('submit')");assert.equal(storage.has('test:test-feedback:pending:'+item.path),false);assert.equal(input('저장 후 복원 통과').checked,true);assert.equal(byId('test-feedback-fields').disabled,false);
  refuse=false;await run("sendTaskAction('submit')");assert.equal(starts,2);assert.deepEqual(savedRequest.checks,[{index:1,result:'통과',note:''}],'hidden failure notes must not be submitted for a passed item');
  assert.match(message(),/모든 항목이 통과해 완료했습니다/,'passing the last item completes the task at once');
  t.checklist=t.checklist.map(r=>({...r,result:'통과'}));t.revision++;await run('loadTaskJobs()');
  assert.match(text('task-phase'),/모든 항목이 통과해 완료된 작업입니다/);assert.equal(byId('test-feedback-submit').hidden,true,'a completed task has nothing to send');
  // 일부만 통과하면 AI 없이 결과만 기록한다.
  t.checklist=[row('빌드','AI','통과','exit 0'),row('저장 후 복원','사람','대기'),row('부활 시 적 재생성','사람','대기')];t.latest={...t.latest,status:'complete'};t.revision++;await run('loadTaskJobs()');
  choose('저장 후 복원 통과');await byId('test-feedback-submit').onclick();assert.equal(t.latest.status,'recorded');assert.match(message(),/테스트 결과를 기록했습니다/);
  t.latest={...t.latest,status:'failed',error:'로그인 확인 필요'};t.revision++;await run('loadTaskJobs()');
  input('처리할 AI').value='gemini';input('처리할 AI').onchange();
  const retry=byId('test-feedback-result').children.find(node=>node.textContent==='저장된 요청으로 AI 다시 시도');assert.ok(retry);await retry.onclick();assert.equal(savedRequest.action,'retry');
  assert.equal(savedRequest.provider,'gemini');assert.ok(!Object.hasOwn(savedRequest,'actor'),'a retry reuses the stored request');
  t.latest={...t.latest,status:'failed'};t.revision++;providers=[providers[0]];await run('loadTaskJobs()');
  assert.equal(input('처리할 AI').value,'gemini','unavailable selection must not silently fall back to another AI');
  const before=starts;await run("sendTaskAction('retry')");assert.equal(starts,before);assert.match(message(),/연결되어 있지/);
  providers=undefined;await run('refreshTaskContext()');assert.equal(run('availableProviders().length'),1,'old server capabilities allow only the original Codex route');
  t.derived=true;t.checklist=[row(item.next,'사람','대기')];await run('refreshTaskContext()');
  assert.match(text('task-phase'),/아직 체크리스트가 없어/);assert.ok(input(item.next+' 통과'));

  // 질문 단계: 선택지를 고르거나 직접 적고, 모든 질문에 답해야 전달된다.
  providers=[{id:'codex',label:'Codex'},{id:'claude',label:'Claude Code'},{id:'gemini',label:'Gemini CLI'}];t.derived=false;t.checklist=[];
  t.questions=[{id:'Q0',question:'이전 질문',options:[],recommendation:'',answer:'A · 테스터 2026-09-25'},{id:'Q1',question:'감소 방식?',options:['즉시','지연'],recommendation:'지연 · 피격 확인이 쉽다',answer:''},{id:'Q2',question:'적용 범위?',options:[],recommendation:'',answer:''}];
  t.latest={...t.latest,status:'questions',error:''};t.revision++;await run('refreshTaskContext()');
  assert.equal(byId('test-feedback-submit').textContent,'답변 전달');assert.equal(byId('test-feedback-submit').hidden,false);
  assert.match(text('task-phase'),/AI 질문 · 2/);assert.match(text('task-phase'),/추천 · 지연 · 피격 확인이 쉽다/);assert.match(text('task-phase'),/답한 질문 · 1/);
  assert.equal(input('Q1 지연').checked,false,'the recommendation is not preselected');
  await byId('test-feedback-submit').onclick();assert.match(message(),/답하지 않은 질문이 있습니다: Q1, Q2/);
  choose('Q1 지연');type('Q1 직접 답변','0.5초 뒤');type('Q2 직접 답변','보스만');
  const beforeAnswer=starts;await byId('test-feedback-submit').onclick();assert.equal(starts,beforeAnswer+1);
  assert.equal(savedRequest.action,'answer');assert.deepEqual(savedRequest.answers,[{id:'Q1',answer:'지연 — 0.5초 뒤'},{id:'Q2',answer:'보스만'}]);assert.equal(savedRequest.actor,'테스터');
  assert.deepEqual(JSON.parse(JSON.stringify(run('taskDraft(item.path).answers'))),{},'sent answers are cleared');

  // 승인 단계: 계획을 보여주고 권한을 알린 뒤 승인만 보낸다.
  t.questions=t.questions.map(q=>({...q,answer:q.answer||'답 · 테스터'}));t.plan={text:'1. 지연 감소 추가\n- 검증: 자동화 테스트',approval:''};t.latest={...t.latest,status:'approval'};t.revision++;
  await run('loadTaskJobs()');
  assert.equal(byId('test-feedback-submit').textContent,'구현 승인');assert.match(text('task-phase'),/1\. 지연 감소 추가\n- 검증: 자동화 테스트/);assert.match(text('task-phase'),/모든 명령/);
  await byId('test-feedback-submit').onclick();assert.equal(savedRequest.action,'approve');assert.equal(savedRequest.actor,'테스터');
  assert.ok(!['answers','checks','message'].some(key=>Object.hasOwn(savedRequest,key)),'approval sends no other input');

  // 추가 요청과 터미널 이어하기, 할 일이 없는 단계.
  t.plan={...t.plan,approval:'테스터 2026-09-25'};t.latest={...t.latest,status:'blocked',report:{summary:'확인 필요',changes:[],checks:[],checklist:[],blockers:['범위 확인']}};t.revision++;
  await run('loadTaskJobs()');
  assert.equal(byId('test-feedback-submit').hidden,true);assert.match(text('task-phase'),/지금 답하거나 확인할 항목이 없습니다/);
  assert.match(text('test-feedback-result'),/남은 확인 · 범위 확인/,'old results still show their remarks');
  await button('추가 요청 전달').onclick();assert.match(message(),/추가 요청을 입력/);
  type('추가 요청','모든 적에게도 적용');await button('추가 요청 전달').onclick();
  assert.equal(savedRequest.action,'request');assert.equal(savedRequest.message,'모든 적에게도 적용');assert.equal(run('taskDraft(item.path).message'),'');
  t.latest={...t.latest,status:'retest'};t.revision++;await run('loadTaskJobs()');assert.equal(byId('task-terminal').disabled,false);
  await byId('task-terminal').onclick();assert.deepEqual(terminals.at(-1),{action:'terminal',taskPath:item.path,provider:'gemini'});assert.match(message(),/Gemini CLI 터미널 창을 열었습니다/);

  // 새 작업: 입력을 보존하고, 응답을 못 받으면 같은 접수를 다시 보낸 뒤 작업 진행 패널로 넘어간다.
  run('openNewTask()');
  assert.equal(descendants(byId('test-feedback-panel')).find(node=>node.tagName==='H2').textContent,'새 작업');assert.equal(input('이름').value,'테스터','the name is remembered across tasks');
  await button('AI에게 전달').onclick();assert.match(message(),/제목을 입력/);
  type('제목','Boss HP');await button('AI에게 전달').onclick();assert.match(message(),/요청을 입력/);
  type('요청','보스 체력바를 지연 감소로');
  run('openNewTask()');assert.equal(input('제목').value,'Boss HP','the new task draft is kept');
  const beforeCreate=starts;loss=true;await button('AI에게 전달').onclick();assert.equal(starts,beforeCreate+1);assert.match(message(),/보존/);
  assert.deepEqual(JSON.parse(JSON.stringify(savedRequest)),{action:'create',operationId:savedRequest.operationId,title:'Boss HP',request:'보스 체력바를 지연 감소로',actor:'테스터',provider:'codex'});
  assert.ok(storage.has('test:test-feedback:pending'));assert.equal(byId('new-task-fields').disabled,true);
  await button('AI에게 전달').onclick();assert.equal(starts,beforeCreate+1,'a replayed creation does not start twice');
  assert.equal(storage.has('test:test-feedback:pending'),false);assert.equal(storage.has('test:test-feedback:new'),false);
  assert.equal(run('taskSelected.path'),newPath);assert.equal(descendants(byId('test-feedback-panel')).find(node=>node.tagName==='H2').textContent,'Boss HP');
  assert.match(message(),/새 작업을 만들었습니다/);assert.equal(run('taskRecordFilter'),'active');assert.match(text('task-phase'),/AI가 처리하는 동안/);
  assert.match(text('task-origin'),/보스 체력바를 지연 감소로/);
  console.log('PASS task panel phases (checklist, questions, approval, complete, empty), instant completion and record-only messages, read-only AI rows, required failure notes and answers, draft preservation, remembered name, unknown-response replay for tasks and new tasks, running lock, result refresh, retry, additional requests and terminal continuation');
})().catch(error=>{console.error(error);process.exitCode=1;});
