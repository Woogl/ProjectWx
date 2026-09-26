// Copyright Woogle. All Rights Reserved.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const {spawn}=require('node:child_process');
const {createFeedbackService,runJob,openTerminal,openSession,taskPrompt,schemaFor,readTasks,writeChecklist,writeHead}=require('./Workflow-TestFeedback.cjs');
const {readChecklist,readHead,readTaskRecord}=require('./wiki-viewer/task-records.js');
const {createServer,createWikiUpdate}=require('./Wiki-AI.cjs');
const {toWsl,wslArgs}=require('./Wiki-Obsidian.cjs');
const base=fs.mkdtempSync(path.join(os.tmpdir(),'wx-test-feedback-'));
const taskPath='.agents/workflow/tasks/example.md';
const checklistText='## 테스트 체크리스트\n\n| 항목 | 확인 방법 | 담당 | 결과 | 근거 |\n| --- | --- | --- | --- | --- |\n| 빌드 | Development 빌드 | AI | 통과 | 빌드 exit 0 |\n| 저장 후 복원 | 저장 → 종료 → 재개, 저장 위치에서 시작한다. | 사람 | 대기 |  |\n';
const taskText='# 예시 작업\n\n상태: 확인 대기 · 구현·빌드 확인\n다음 행동: 저장 후 복원을 확인한다.\n\n원본 결정과 검증 근거\n\n'+checklistText+'\n## 이력\n\n- 구현\n';
const row=(item,owner,result,evidence='')=>({item,method:'확인 방법',owner,result,evidence});
const passedRows=[row('빌드','AI','통과','빌드 exit 0'),row('저장 후 복원','사람','통과','테스터')];
// 단계마다 AI가 돌려주는 결과 모양
const fixReport=(checklist,extra={})=>({summary:'수정',evidence:['회귀 테스트 exit 0'],changes:['좌표 복원 순서 수정'],questions:[],checklist,...extra});
const research={summary:'조사',evidence:['관련 코드 읽음'],questions:[],plan:''};
const settle=()=>new Promise(resolve=>setImmediate(resolve));
let sequence=0,server;
function fixture(providers=['codex'],content=taskText){
  const root=path.join(base,'case-'+(++sequence)),folder=path.join(root,'.agents/workflow/tasks'),stateFolder=path.join(root,'Saved/Workflow/test-feedback');fs.mkdirSync(folder,{recursive:true});
  fs.writeFileSync(path.join(root,taskPath),content);
  let finish,reject,runs=0,lastInput;const opened=[];
  const options={root,providers,run:input=>{lastInput=input;runs++;return new Promise((a,b)=>{finish=a;reject=b;});},open:(...args)=>opened.push(args)};
  let service=createFeedbackService(options);
  const context=(relative=taskPath)=>service.act({action:'read',taskPath:relative});
  const request=(extra={})=>({action:'submit',taskPath,operationId:'op-'+(++sequence),taskHash:context().taskHash,actor:'테스터',checks:[{index:1,result:'통과',note:''}],...extra});
  // 어느 기록에나 최신 revision·기록 해시로 동작을 보낸다.
  const send=(relative,action,extra={})=>{const current=context(relative);return service.act({action,taskPath:relative,operationId:'op-'+(++sequence),taskHash:current.taskHash,actor:'테스터',provider:providers[0],...extra});};
  const task=(relative=taskPath)=>fs.readFileSync(path.join(root,relative),'utf8');
  return {root,folder,stateFolder,context,request,send,task,opened,get service(){return service;},get runs(){return runs;},get input(){return lastInput;},finish:value=>finish(value),fail:()=>reject(Error('일시적 CLI 실패')),restart:()=>{service=createFeedbackService(options);},rows:(relative=taskPath)=>readChecklist(task(relative)).rows,head:(relative=taskPath)=>readHead(task(relative))};
}
(async()=>{try{
  // 기록 맨 위 상태 줄: 기존 표기(- 접두사·설명)를 읽고, 없으면 제목 아래에 만든다.
  assert.deepEqual(readHead('# 제목\n\n- 상태: 확인 대기 · 단계: 확인하기\n- 다음 행동: 리뷰한다.\n\n## 절\n\n상태: 완료\n'),{title:'제목',state:'확인 대기',detail:'단계: 확인하기',next:'리뷰한다.'});
  assert.equal(readHead('# 제목\n\n상태: 구현 중. 1단계 완료\n').state,'','free-form legacy status is not a state');
  assert.equal(readHead('# 제목\n\n상태: 완료된 일 정리\n').state,'','a state word must be complete');
  assert.equal(writeHead('# 제목\n\n본문\n','완료','체크리스트 1/1 통과','참고한다.'),'# 제목\n\n상태: 완료 · 체크리스트 1/1 통과\n다음 행동: 참고한다.\n\n본문\n');
  assert.equal(writeHead('# 제목\n\n- 상태: 확인 대기 · 단계\n\n본문\n','완료','','없음'),'# 제목\n\n- 상태: 완료\n- 다음 행동: 없음\n\n본문\n');
  assert.equal(readTaskRecord('a.md','# 리뷰\n\n본문\n').state,'','records without a state line are listed as references');
  // 표 형식: 머리글·담당·결과를 검사하고 셀의 | 와 줄바꿈은 기록할 때 지운다.
  assert.equal(readChecklist('# 제목\n'),null);
  assert.throws(()=>readChecklist('## 테스트 체크리스트\n\n| 항목 | 결과 |\n| --- | --- |\n'),/머리글/);
  assert.throws(()=>readChecklist(checklistText.replace('| 사람 | 대기 |','| 테스터 | 대기 |')),/2번째 행/);
  assert.deepEqual(readChecklist(writeChecklist(taskText,[row('줄|바꿈\n항목','사람','대기')])).rows,[row('줄 바꿈 항목','사람','대기')]);
  assert.match(writeChecklist('# 제목\n\n본문\n\n## 이력\n',[row('새 항목','사람','대기')]),/본문\n\n## 테스트 체크리스트\n\n\| 항목[\s\S]*\| 새 항목 [\s\S]*\n\n## 이력/);
  // 단계마다 AI가 채울 수 있는 칸
  assert.deepEqual(schemaFor('plan').required,['summary','evidence','questions','plan']);
  assert.deepEqual(schemaFor('implement').required,['summary','evidence','changes','questions','checklist']);
  for(const kind of ['plan','implement','request','fix'])assert.ok(!('blockers' in schemaFor(kind).properties)&&!('checks' in schemaFor(kind).properties),kind);

  const f=fixture();
  assert.deepEqual(f.context().checklist.map(r=>[r.item,r.owner,r.result]),[['빌드','AI','통과'],['저장 후 복원','사람','대기']]);
  assert.equal(f.context().state,'확인 대기');assert.equal(f.context().next,'저장 후 복원을 확인한다.','the panel shows the recorded next action');assert.ok(!Object.hasOwn(f.context(),'codeVersion'),'code versions no longer gate submissions');
  assert.throws(()=>f.service.act(f.request({provider:'claude'})),/선택한 AI/);
  assert.throws(()=>f.service.act(f.request({provider:'unknown'})),/선택한 AI/);
  assert.throws(()=>f.service.act(f.request({checks:[]})),/하나 이상/);
  assert.throws(()=>f.service.act(f.request({checks:[{index:0,result:'통과',note:''}]})),/담당이 사람/,'AI rows are not human-editable');
  assert.throws(()=>f.service.act(f.request({checks:[{index:1,result:'통과',note:''},{index:1,result:'실패',note:'중복'}]})),/담당이 사람/);
  assert.throws(()=>f.service.act(f.request({checks:[{index:1,result:'보류',note:''}]})),/통과 또는 실패/);
  assert.throws(()=>f.service.act(f.request({checks:[{index:1,result:'실패',note:' '}]})),/문제 상황/);
  assert.throws(()=>f.service.act(f.request({actor:'\n사용자'})),/이름/);
  assert.throws(()=>f.service.act({action:'read',taskPath:'.agents/workflow/tasks/../process/index.md'}),/경로/);
  assert.throws(()=>f.service.act({action:'read',taskPath:'.agents/workflow/tasks/missing.md'}),/작업 기록 폴더/);
  const oldTask=f.request();fs.appendFileSync(path.join(f.root,taskPath),'\n새 결정\n');assert.throws(()=>f.service.act(oldTask),/바뀌었습니다/);
  assert.equal(f.task().includes('| 사람 | 대기 |'),true,'rejected submissions must not touch the checklist');
  // 모든 항목이 통과하면 서버가 AI를 부르지 않고 그 자리에서 완료한다.
  const sent=f.request(),accepted=f.service.act(sent);
  assert.deepEqual([accepted.latest.kind,accepted.latest.status],['record','complete']);
  assert.deepEqual(f.head(),{title:'예시 작업',state:'완료',detail:'체크리스트 2/2 통과',next:'변경 시 기록된 테스트 범위와 제약을 참고한다.'},'the human result completes the task at once');
  await settle();
  assert.equal(f.rows()[1].result,'통과');assert.match(f.rows()[1].evidence,/^테스터 \d{4}-\d{2}-\d{2}$/);
  assert.equal(f.runs,0,'completion calls no AI');assert.equal(f.service.isBusy(),false);assert.doesNotMatch(f.task(),/AI 완료 정리/);
  assert.match(f.task(),/\n## 사용자 테스트 결과 · [^\n]+\n\n<!-- test-feedback:op-\d+:submitted -->\n- 전달한 사람: 테스터\n\n> 통과 · 저장 후 복원\n/,'the human result is recorded at once');
  assert.equal(f.service.act(sent).latest.status,'complete','a replay returns the same record');assert.equal(f.runs,0);
  assert.throws(()=>f.service.act({...sent,actor:'다른 사람'}),/같은 접수/);
  assert.deepEqual(f.service.act({action:'list'}).tasks.map(t=>[t.path,t.state,t.summary]),[[taskPath,'완료','체크리스트 2/2 통과']],'the task list is built from records');
  const recorded=f.task();f.service.act(sent);await settle();assert.equal(f.runs,0);assert.equal(f.task(),recorded);
  f.restart();assert.equal(f.context().latest.status,'complete');
  f.service.act({action:'terminal',taskPath,provider:'codex'});assert.deepEqual(f.opened,[[taskPath,'codex','예시 작업']]);
  assert.throws(()=>f.service.act({action:'terminal',taskPath,provider:'gemini'}),/선택한 AI/);

  // 일부만 통과하고 실패가 없으면 AI를 부르지 않고 결과만 기록한다.
  const twoHuman='# 부분 작업\n\n상태: 확인 대기 · 구현\n다음 행동: 확인한다.\n\n## 테스트 체크리스트\n\n| 항목 | 확인 방법 | 담당 | 결과 | 근거 |\n| --- | --- | --- | --- | --- |\n| 빌드 | 빌드 | AI | 통과 | exit 0 |\n| 저장 | 저장한다 | 사람 | 대기 |  |\n| 복원 | 복원한다 | 사람 | 대기 |  |\n';
  const part=fixture(['codex'],twoHuman),partial=part.request({checks:[{index:1,result:'통과',note:''}]});
  assert.equal(part.service.act(partial).latest.status,'recorded');assert.equal(part.runs,0,'no AI is called when nothing needs AI');assert.equal(part.service.isBusy(),false);
  assert.deepEqual(part.head(),{title:'부분 작업',state:'확인 대기',detail:'체크리스트 2/3 통과',next:'사람 확인: 복원 (대기)'});
  assert.match(part.task(),/## 사용자 테스트 결과[\s\S]*> 통과 · 저장/);
  const once=part.task();part.service.act(partial);assert.equal(part.task(),once,'a replay records nothing twice');
  part.service.act(part.request({checks:[{index:2,result:'통과',note:''}]}));assert.equal(part.head().state,'완료');await settle();assert.equal(part.runs,0,'completion calls no AI');

  // 실패가 있으면 결과를 먼저 기록하고 AI가 고친다. 실패·재시도·AI 변경을 보존한다.
  const multi=fixture(['claude','gemini']);
  assert.deepEqual(multi.context().providers.map(p=>p.id),['claude','gemini']);
  const claude=multi.request({provider:'claude',checks:[{index:1,result:'실패',note:'문제 원문'}]});
  multi.service.act(claude);await settle();assert.deepEqual([multi.input.provider,multi.input.kind],['claude','fix']);assert.equal(multi.rows()[1].result,'실패');
  assert.deepEqual([multi.head().state,multi.head().detail],['진행 중','AI 수정 중']);assert.match(multi.task(),/> 실패 · 저장 후 복원: 문제 원문/,'the failure is recorded before the AI runs');
  assert.throws(()=>multi.service.act({action:'terminal',taskPath,provider:'claude'}),/처리하는 중/,'a running task cannot be opened in a terminal');
  multi.fail();await settle();
  assert.deepEqual(multi.head(),{title:'예시 작업',state:'확인 대기',detail:'AI 처리 실패',next:'작업 진행 화면에서 다시 시도한다.'},'a failed run is visible in the task list');
  multi.restart();assert.equal(multi.context().latest.provider,'claude');
  const switchAI=multi.request({action:'retry',provider:'gemini'});multi.service.act(switchAI);await settle();
  assert.deepEqual([multi.input.provider,multi.input.kind],['gemini','fix']);assert.equal(multi.input.checks[0].note,'문제 원문');assert.equal(multi.input.attempts[0].provider,'claude');
  assert.throws(()=>multi.service.act(switchAI),/다른 AI/,'a resent retry is refused while running');assert.equal(multi.runs,2);multi.fail();await settle();
  multi.service.act(multi.request({action:'retry'}));await settle();assert.equal(multi.input.provider,'gemini','retry without a selection preserves its provider');
  multi.finish(fixReport([passedRows[0],row('저장 후 복원','사람','대기','수정 후 재확인'),row('코드 리뷰','사람','대기','좌표 복원 수정')]));await settle();
  assert.equal(multi.context().latest.status,'retest');assert.match(multi.task(),/## AI 수정 결과 · [\s\S]*처리 AI: Gemini CLI/);
  assert.deepEqual(multi.head(),{title:'예시 작업',state:'확인 대기',detail:'체크리스트 1/3 통과',next:'사람 확인: 저장 후 복원 (대기)'});
  assert.throws(()=>multi.service.act({...claude,provider:'gemini'}),/같은 접수/);

  // 완료한 작업에도 새 문제가 발생하면 확인 대기로 돌아간다.
  const issue=f.request({checks:[{index:1,result:'실패',note:'복원 직후 좌표가 (0, 0, 0)입니다.\n재현: 저장 → 종료 → 재개'}]});
  f.service.act(issue);await settle();assert.equal(f.input.kind,'fix');
  f.finish(fixReport([passedRows[0],row('저장 후 복원','사람','대기','좌표 복원 순서 수정 후 재확인'),row('코드 리뷰','사람','대기','좌표 복원 수정')]));await settle();
  assert.equal(f.context().latest.status,'retest');assert.match(f.task(),/> 재현: 저장 → 종료 → 재개/);
  assert.deepEqual(f.head(),{title:'예시 작업',state:'확인 대기',detail:'체크리스트 1/3 통과',next:'사람 확인: 저장 후 복원 (대기)'});
  assert.match(f.task(),/\| 코드 리뷰 \| 확인 방법 \| 사람 \| 대기 \|/,'a fix adds a human code review item');
  f.service.act(f.request({checks:[{index:1,result:'통과',note:''},{index:2,result:'통과',note:''}]}));assert.equal(f.head().state,'완료');await settle();
  assert.equal(f.context().latest.status,'complete');

  // AI는 사람 항목을 통과시키거나 지우거나 담당을 바꿀 수 없다.
  const guard=fixture();guard.service.act(guard.request({checks:[{index:1,result:'실패',note:'원점 이동'}]}));await settle();
  guard.finish(fixReport([passedRows[0],row('새 사람 항목','사람','통과','AI 주장'),row('저장 후 복원','AI','통과','AI가 대신 확인')]));await settle();
  assert.deepEqual(guard.rows().map(r=>[r.item,r.owner,r.result]),[['빌드','AI','통과'],['새 사람 항목','사람','대기'],['저장 후 복원','사람','대기']]);
  assert.equal(guard.context().latest.status,'retest');
  const dropped=fixture();dropped.service.act(dropped.request({checks:[{index:1,result:'실패',note:'원점 이동'}]}));await settle();
  dropped.finish(fixReport([passedRows[0]]));await settle();
  assert.deepEqual(dropped.rows().map(r=>[r.item,r.result]),[['빌드','통과'],['저장 후 복원','실패']]);assert.equal(dropped.context().latest.status,'issues');
  assert.equal(dropped.head().next,'실패 확인: 저장 후 복원(사람) · 추가 요청으로 방향을 정한다.');
  // AI가 실행하지 못한 항목은 사람에게 넘어온다.
  const handed=fixture();handed.service.act(handed.request({checks:[{index:1,result:'실패',note:'원점'}]}));await settle();
  handed.finish(fixReport([row('빌드','AI','통과','exit 0'),row('PIE 확인','AI','미실행','에디터 없음'),row('저장 후 복원','사람','대기','재확인')]));await settle();
  assert.deepEqual(handed.rows().map(r=>[r.item,r.owner,r.result,r.evidence]),[['빌드','AI','통과','exit 0'],['PIE 확인','사람','대기','AI가 실행하지 못함: 에디터 없음'],['저장 후 복원','사람','대기','재확인']]);
  // 상태는 체크리스트·질문에서만 정한다. 남은 확인·검사 목록 같은 옛 칸은 버린다.
  for(const [label,value,expected] of [
    ['ai-row-failed',fixReport([row('빌드','AI','실패','링크 오류'),row('저장 후 복원','사람','대기','재확인')]),'issues'],
    ['question',fixReport([],{questions:[{id:'Q1',question:'범위를 넓힐까요?',options:['넓힌다','그대로'],recommendation:'그대로'}]}),'questions'],
    ['ignored-fields',fixReport([passedRows[0],row('저장 후 복원','사람','대기','재확인')],{blockers:['무관한 경고'],checks:[{name:'lint',status:'failed',evidence:'다른 문서'}]}),'retest'],
    ['nothing',fixReport([]),'failed']
  ]){const c=fixture();c.service.act(c.request({checks:[{index:1,result:'실패',note:'문제'}]}));await settle();c.finish(value);await settle();assert.equal(c.context().latest.status,expected,label);assert.equal(c.head().state,'확인 대기',label);}
  // 처리 중 기록이 바뀌면 결과를 반영하지 않고 기록 충돌로 둔다.
  const revised=fixture();revised.service.act(revised.request({checks:[{index:1,result:'실패',note:'문제'}]}));await settle();fs.appendFileSync(path.join(revised.root,taskPath),'\n추가 결정\n');
  revised.finish(fixReport([passedRows[0],row('저장 후 복원','사람','대기','재확인')]));await settle();
  assert.equal(revised.context().latest.status,'conflict');assert.match(revised.task(),/추가 결정/);assert.deepEqual([revised.head().state,revised.head().detail],['확인 대기','기록 충돌']);
  assert.equal(revised.rows()[1].result,'실패','a conflicting result is not applied');

  const failed=fixture();failed.service.act(failed.request({checks:[{index:1,result:'실패',note:'문제 원문'}]}));await settle();failed.fail();await settle();
  assert.equal(failed.context().latest.status,'failed');assert.equal(failed.context().latest.checks[0].note,'문제 원문');
  const retry=failed.request({action:'retry'});failed.service.act(retry);await settle();assert.throws(()=>failed.service.act(retry),/다른 AI/);assert.equal(failed.runs,2);
  failed.finish(fixReport([passedRows[0],row('저장 후 복원','사람','대기','재확인')]));await settle();assert.equal(failed.context().latest.status,'retest');
  const interrupted=fixture();interrupted.service.act(interrupted.request({checks:[{index:1,result:'실패',note:'문제'}]}));await settle();interrupted.restart();assert.equal(interrupted.context().latest.status,'interrupted');
  assert.equal(interrupted.head().detail,'AI 처리 중단','an interrupted run is visible in the task list');
  interrupted.service.act(interrupted.request({action:'retry'}));await settle();interrupted.finish(fixReport([passedRows[0],row('저장 후 복원','사람','대기','재확인')]));await settle();assert.equal(interrupted.context().latest.status,'retest');
  const stored=fixture();stored.service.act(stored.request({checks:[{index:1,result:'실패',note:'문제'}]}));await settle();
  assert.ok(fs.readdirSync(stored.stateFolder).some(name=>name.startsWith('test_feedback_'))&&!fs.readdirSync(stored.folder).some(name=>name.startsWith('test_feedback_')),'request state lives outside the tracked records folder');
  fs.writeFileSync(path.join(stored.stateFolder,'test_feedback_'+'0'.repeat(64)+'.json'),'<<<<<<< conflict');
  stored.restart();assert.equal(stored.service.isBusy(),false,'no stored process id keeps the server busy after a restart');assert.equal(stored.context().latest.status,'interrupted','an unreadable state file is skipped');
  const broken=fixture(['codex'],taskText.replace('| 사람 | 대기 |','| 누군가 | 대기 |'));
  assert.match(broken.context().checklistError,/2번째 행/);assert.throws(()=>broken.service.act(broken.request({checks:[{index:0,result:'통과',note:''}]})),/2번째 행/);
  // 옛 기록에도 추가 요청을 보낼 수 있고, 요청 절은 체크리스트 앞에 생긴다. 설명만 한 결과는 상태를 바꾸지 않는다.
  const extra=fixture();extra.send(taskPath,'request',{message:'복원 위치를 로그로 남겨줘'});await settle();
  assert.equal(extra.input.kind,'request');assert.match(extra.task(),/원본 결정과 검증 근거\n\n## 요청\n\n- 추가 요청 · 테스터 \d{4}-\d{2}-\d{2}\n\n> 복원 위치를 로그로 남겨줘\n\n## 테스트 체크리스트/);
  extra.finish({summary:'로그 위치 설명',evidence:['코드 읽음'],changes:[],questions:[],plan:'',checklist:[]});await settle();
  assert.deepEqual([extra.context().latest.status,extra.head().state,extra.head().next],['retest','확인 대기','사람 확인: 저장 후 복원 (대기)']);
  assert.throws(()=>extra.service.act({action:'read',taskPath:'x'}),/경로/);
  // 구현 승인 전의 추가 요청은 정하기(읽기 전용)로 처리한다.
  const planning=fixture(['codex'],'# 계획 작업\n\n## 요청\n\n- 요청자: 테스터 · 2026-09-26\n\n> 요청 원문\n\n## 구현 계획\n\n1. 저장 위치를 바꾼다\n');
  planning.send(taskPath,'request',{message:'계획에 로그를 더해줘'});await settle();assert.equal(planning.input.kind,'plan','a request before approval is read-only planning');
  // 계획이 바뀌어 승인 줄을 지운 기록은 체크리스트가 있어도 다시 승인받기 전까지 정하기다.
  const reapproval=fixture(['codex'],taskText.replace('## 테스트 체크리스트','## 구현 계획\n\n1. 바뀐 계획\n\n## 테스트 체크리스트'));
  reapproval.send(taskPath,'request',{message:'바뀐 계획을 설명해줘'});await settle();assert.equal(reapproval.input.kind,'plan','a plan waiting for re-approval keeps extra requests read-only');
  // 코드가 바뀌면 이미 통과한 코드 리뷰를 다시 받는다.
  const review=fixture(['codex'],taskText.replace('| 저장 후 복원 |','| 코드 리뷰 | 변경 파일 | 사람 | 통과 | 테스터 |\n| 저장 후 복원 |'));
  review.service.act(review.request({checks:[{index:2,result:'실패',note:'복원 위치가 다름'}]}));await settle();
  review.finish(fixReport([passedRows[0],row('코드 리뷰','사람','통과','테스터'),row('저장 후 복원','사람','대기','재확인')]));await settle();
  assert.deepEqual(review.rows().map(r=>[r.item,r.result]),[['빌드','통과'],['코드 리뷰','대기'],['저장 후 복원','대기']],'changed code needs a new code review');

  // 새 작업: 기록을 만들고 읽기 전용 조사 → 질문 → 답변 → 구현 계획 → 구현 승인 → 구현 → 추가 요청 순서로 진행한다.
  const n=fixture(['codex','claude']);
  const createBody={action:'create',operationId:'create-1',title:'Boss HP bar 개선',request:'보스 체력바가 늦게 줄어든다.\n## 제목처럼 보이는 줄',actor:'테스터',provider:'claude'};
  for(const [label,body,error] of [['title',{title:' '},/제목/],['title-line',{title:'a\nb'},/제목/],['request',{request:' '},/요청/],['actor',{actor:''},/이름/],['provider',{provider:'gemini'},/선택한 AI/],['operation',{operationId:undefined},/접수 식별자/]])
    assert.throws(()=>n.service.act({...createBody,...body}),error,label);
  assert.deepEqual(fs.readdirSync(n.folder).filter(name=>name.endsWith('.md')).sort(),['example.md'],'rejected creations write nothing');
  const created=n.service.act(createBody),newPath=created.taskPath;
  assert.equal(newPath,'.agents/workflow/tasks/Boss-HP-bar-개선.md','the title becomes the record name');assert.equal(created.latest.status,'running');await settle();
  assert.equal(n.runs,1);assert.deepEqual([n.input.action,n.input.kind,n.input.title,n.input.provider],['create','plan','Boss HP bar 개선','claude']);
  assert.match(n.task(newPath),/^# Boss HP bar 개선\n\n상태: 진행 중 · AI 조사 중\n다음 행동: AI 처리 결과를 기다린다\. 진행 과정은 터미널 창에 보인다\.\n\n## 요청\n\n- 요청자: 테스터 · \d{4}-\d{2}-\d{2}\n\n> 보스 체력바가 늦게 줄어든다\.\n> ## 제목처럼 보이는 줄\n$/);
  assert.equal(n.service.act(createBody).taskPath,newPath,'a replayed creation returns the same record');assert.equal(n.runs,1);
  assert.throws(()=>n.service.act({...createBody,request:'다른 요청'}),/같은 접수의 내용이 바뀌었습니다/);
  assert.throws(()=>n.service.act({...createBody,operationId:'create-2'}),/다른 AI/);
  assert.equal(readTasks(n.root).find(t=>t.path===newPath).state,'진행 중');
  const question=(id,text)=>({id,question:text,options:['즉시 / 바로','지연 후 감소'],recommendation:'지연 후 감소 · 피격 확인이 쉽다'});
  n.finish({...research,questions:[question('Q1','체력바 감소 방식?'),question('Q2','적용 범위?')],checklist:[row('빌드','AI','통과','조사 결과에 끼운 표')]});await settle();
  let current=n.context(newPath);
  assert.equal(current.latest.status,'questions');assert.deepEqual(current.questions.map(q=>[q.id,q.answer]),[['Q1',''],['Q2','']]);assert.deepEqual(current.questions[0].options,['즉시/바로','지연 후 감소']);
  assert.deepEqual(current.checklist,[],'a research result cannot write a checklist');assert.match(current.request,/^- 요청자: 테스터/);
  assert.deepEqual(n.head(newPath),{title:'Boss HP bar 개선',state:'확인 대기',detail:'질문 2개',next:'질문 2개에 답한다.'});
  const order=(...headings)=>{const text=n.task(newPath),at=headings.map(h=>text.indexOf(h));assert.ok(at.every((v,i)=>v>=0&&(i===0||v>at[i-1])),headings.join(' < '));};
  order('## 요청','## 질문','## AI 조사 결과');
  assert.match(n.task(newPath),/## AI 조사 결과 · [^\n]+\n\n<!-- test-feedback:create-1:1 -->\n- 전달한 사람: 테스터\n- 처리 AI: Claude Code\n- 처리 결과: 질문 답변 필요/);
  assert.throws(()=>n.send(newPath,'answer',{answers:[{id:'Q1',answer:'지연 후 감소'}]}),/모든 질문/);
  assert.throws(()=>n.send(newPath,'answer',{answers:[{id:'Q1',answer:'a'},{id:'Q3',answer:'b'}]}),/모든 질문/);
  assert.throws(()=>n.send(newPath,'answer',{answers:[{id:'Q1',answer:'지연 후 감소'},{id:'Q2',answer:' '}]}),/답변을/);
  assert.throws(()=>n.send(newPath,'approve'),/남은 질문/);
  n.send(newPath,'answer',{answers:[{id:'Q1',answer:'지연 후 감소 — 0.5초'},{id:'Q2',answer:'보스만'}]});await settle();
  assert.deepEqual([n.input.action,n.input.kind],['answer','plan']);assert.deepEqual(n.input.answers,[{id:'Q1',answer:'지연 후 감소 — 0.5초'},{id:'Q2',answer:'보스만'}]);
  assert.deepEqual(n.context(newPath).questions.map(q=>q.answer.replace(/ \d{4}-\d{2}-\d{2}$/,'')),['지연 후 감소 — 0.5초 · 테스터','보스만 · 테스터']);
  assert.equal(n.head(newPath).detail,'AI 조사 중');
  n.finish({...research,plan:'1. WxBossHealthBar에 지연 감소 추가\n## 검증\n구현 승인: AI 스스로\n- 체크리스트 초안'});await settle();
  current=n.context(newPath);
  assert.equal(current.latest.status,'approval');assert.equal(current.plan.approval,'','the AI cannot approve its own plan');
  assert.equal(current.plan.text,'1. WxBossHealthBar에 지연 감소 추가\n- 검증\n- 구현 승인: AI 스스로\n- 체크리스트 초안');
  assert.deepEqual(n.head(newPath),{title:'Boss HP bar 개선',state:'확인 대기',detail:'구현 승인 대기',next:'구현 계획을 확인하고 승인한다.'});
  order('## 요청','## 질문','## 구현 계획','## AI 조사 결과');
  assert.throws(()=>n.send(newPath,'approve',{actor:''}),/이름/);
  n.send(newPath,'approve');await settle();
  assert.deepEqual([n.input.action,n.input.kind],['approve','implement']);assert.match(n.context(newPath).plan.approval,/^테스터 \d{4}-\d{2}-\d{2}$/);assert.equal(n.head(newPath).detail,'AI 구현 중');
  // 구현 결과의 계획 칸은 버리고, 사람 항목이 없으면 결과 확인 항목을 둔다.
  n.finish({summary:'구현',evidence:['exit 0'],changes:['WxBossHealthBar.cpp'],questions:[],plan:'1. 구현 중 AI가 적은 다른 계획',checklist:[row('빌드','AI','통과','exit 0'),row('지연 감소 자동화','AI','통과','테스트 통과')]});await settle();
  current=n.context(newPath);
  assert.equal(current.latest.status,'retest');assert.match(current.plan.approval,/^테스터 /,'an implementation result cannot replace the approved plan');
  assert.deepEqual(current.checklist.map(r=>[r.item,r.owner,r.result]),[['빌드','AI','통과'],['지연 감소 자동화','AI','통과'],['결과 확인','사람','대기']],'completion always needs a person');
  assert.deepEqual(n.head(newPath),{title:'Boss HP bar 개선',state:'확인 대기',detail:'체크리스트 2/3 통과',next:'사람 확인: 결과 확인 (대기)'});
  order('## 요청','## 질문','## 구현 계획','## 테스트 체크리스트','## AI 조사 결과','## AI 구현 결과');
  assert.throws(()=>n.send(newPath,'approve'),/승인할 구현 계획이 없습니다/);
  assert.throws(()=>n.send(newPath,'request',{message:' '}),/요청을/);
  n.send(newPath,'request',{message:'모든 적에게도 적용\n## 가짜 절'});await settle();
  assert.deepEqual([n.input.action,n.input.kind],['request','request']);assert.equal(n.input.message,'모든 적에게도 적용\n## 가짜 절');
  assert.match(n.context(newPath).request,/> ## 제목처럼 보이는 줄\n\n- 추가 요청 · 테스터 \d{4}-\d{2}-\d{2}\n\n> 모든 적에게도 적용\n> ## 가짜 절$/);
  n.finish({summary:'범위 변경',evidence:['코드 읽음'],changes:[],plan:'1. 모든 적 체력바에 적용',questions:[question('Q1','일반 적 감소 속도?')],checklist:[]});await settle();
  current=n.context(newPath);
  assert.equal(current.latest.status,'questions','open questions come before a new approval');assert.deepEqual(current.questions.map(q=>[q.id,!!q.answer]),[['Q1',true],['Q2',true],['Q3',false]]);
  assert.equal(current.plan.approval,'','a new plan needs a new approval');assert.equal(current.checklist.length,3,'the checklist is kept');
  n.service.act({action:'terminal',taskPath:newPath,provider:'claude'});assert.deepEqual(n.opened.at(-1),[newPath,'claude','Boss HP bar 개선']);
  // 구현 중 판단이 필요하면 질문으로 받는다. 승인된 계획은 새 계획이 올 때까지 그대로다.
  const ask=fixture(),asked=ask.service.act({...createBody,operationId:'ask-1',provider:'codex',title:'질문 흐름'});await settle();
  ask.finish({...research,plan:'1. 계획'});await settle();ask.send(asked.taskPath,'approve');await settle();
  ask.finish({summary:'구현 중 판단 필요',evidence:['코드 읽음'],changes:[],questions:[{id:'Q1',question:'캐시를 둘까요?',options:['둔다','안 둔다'],recommendation:'안 둔다'}],checklist:[]});await settle();
  assert.equal(ask.context(asked.taskPath).latest.status,'questions');assert.match(ask.context(asked.taskPath).plan.approval,/^테스터 /);
  const failedPlan=fixture(),koreanBody={...createBody,operationId:'create-9',title:'보스 체력바',provider:'codex'};const failedCreate=failedPlan.service.act(koreanBody);await settle();
  assert.equal(failedCreate.taskPath,'.agents/workflow/tasks/보스-체력바.md','a Korean title is kept as the record name');
  failedPlan.finish(research);await settle();assert.equal(failedPlan.context(failedCreate.taskPath).latest.status,'failed','a plan run needs questions or a plan');
  assert.match(failedPlan.context(failedCreate.taskPath).latest.error,/질문이나 구현 계획/);assert.equal(failedPlan.head(failedCreate.taskPath).detail,'AI 처리 실패');
  failedPlan.restart();assert.equal(failedPlan.service.act(koreanBody).taskPath,failedCreate.taskPath,'a replay after restart finds the record from its history');
  assert.deepEqual(readTasks(failedPlan.root).filter(t=>t.path===failedCreate.taskPath).map(t=>[t.title,t.state]),[['보스 체력바','확인 대기']],'Korean record names are listed');
  // 같은 제목은 -2를 붙이고, 파일 이름에 못 쓰는 기호는 -로 바꾼다.
  for(const [title,name] of [['보스 체력바','보스-체력바-2.md'],['저장/복원: "좌표" 확인?','저장-복원-좌표-확인.md'],['CON','CON-작업.md'],['!!!','작업.md']]){
    assert.equal(failedPlan.service.act({...koreanBody,operationId:'create-'+(++sequence),title}).taskPath,'.agents/workflow/tasks/'+name,title);
    await settle();failedPlan.fail();await settle();
  }

  // 작업 목록은 기록에서 만들고 최근 수정 순으로 정렬한다. 상태 줄이 없는 기록은 상태가 비어 있다.
  const listing=fixture();
  fs.writeFileSync(path.join(listing.folder,'module_review_WxAI.md'),'# AI 모듈 리뷰\n\n지적 1건\n');
  fs.utimesSync(path.join(listing.folder,'module_review_WxAI.md'),new Date(Date.now()+60000),new Date(Date.now()+60000));
  assert.deepEqual(readTasks(listing.root).map(t=>[t.title,t.state,t.summary,t.next]),[['AI 모듈 리뷰','','',''],['예시 작업','확인 대기','체크리스트 1/2 통과','저장 후 복원을 확인한다.']]);

  // AI에게 주는 지시: 정하기는 읽기 전용, 나머지는 사용자 결정대로 권한 확인 없이 실행한다. 사람에게는 질문·계획·체크리스트로만 넘긴다.
  const prompt=kind=>taskPrompt({action:'x',kind,taskPath,taskHash:'h',actor:'테스터',at:'t',answers:[{id:'Q1',answer:'A안'}],message:'추가 요청 원문',checks:[{item:'저장 후 복원',result:'실패',note:'재현: 저장 → 종료 → 재개'}]});
  assert.match(prompt('plan'),/지금은 정하기입니다/);assert.doesNotMatch(prompt('plan'),/권한 확인 없이/);assert.doesNotMatch(prompt('plan'),/checklist는 기존 항목을 포함한/);
  assert.match(prompt('implement'),/구현 계획대로 구현/);
  assert.match(prompt('request'),/추가 요청/);assert.ok(prompt('request').includes('"message":"추가 요청 원문"'));assert.ok(prompt('plan').includes('"answers":[{"id":"Q1","answer":"A안"}]'));
  assert.match(prompt('fix'),/실패 원인을 조사/);assert.ok(prompt('fix').includes('재현: 저장 → 종료 → 재개'));
  for(const kind of ['implement','request','fix'])assert.match(prompt(kind),/권한 확인 없이 명령을 실행/,kind);
  for(const kind of ['implement','request','fix'])assert.match(prompt(kind),/checklist는 기존 항목을 포함한 전체 체크리스트/,kind);
  for(const kind of ['plan','implement','request','fix']){
    const text=prompt(kind);
    assert.match(text,/관리자 정책·CLI 설정 변경/,kind);assert.doesNotMatch(text,/한국어로|\| 문자|직접 고치지 마세요|Q번호|선택지 2개/,kind);assert.match(text,/Git 커밋·푸시/,kind);assert.doesNotMatch(text,/blockers/,kind);
    assert.match(text,/AGENTS\.md와 \.agents\/workflow\/process\/index\.md를 따르고/,kind);
  }

  // 터미널 창 실행기: 작업 파일을 넘기고 결과 파일을 기다린다. 모드와 결과 모양은 단계에서 정한다.
  const jobRoot=path.join(base,'jobs');fs.mkdirSync(jobRoot);
  let seen;
  const fakeRunner=(result,pid=process.pid)=>(root,title,argv)=>{
    const job=argv[2];seen={root,title,argv,job:JSON.parse(fs.readFileSync(path.join(job,'job.json'))),prompt:fs.readFileSync(path.join(job,'prompt.txt'),'utf8'),schema:JSON.parse(fs.readFileSync(path.join(job,'schema.json')))};
    fs.writeFileSync(path.join(job,'pid.txt'),String(pid));if(result)fs.writeFileSync(path.join(job,'result.json'),JSON.stringify(result));
  };
  const jobRequest=(kind,extra={})=>({action:'x',kind,provider:'codex',operationId:'job-'+(++sequence),taskPath,title:'제목 "따옴표" & 기호',...extra});
  const values={plan:{...research,plan:'계획'},implement:fixReport([passedRows[0]]),request:{summary:'설명',evidence:['읽음'],changes:[],questions:[],plan:'',checklist:[]},fix:fixReport([passedRows[0]])};
  for(const [kind,mode] of [['plan','plan'],['implement','work'],['request','work'],['fix','work']]){
    const value=await runJob({root:jobRoot,command:{file:'codex'},request:jobRequest(kind),open:fakeRunner({ok:true,value:{...values[kind],blockers:['버릴 칸']}}),wait:5});
    assert.equal(value.summary,values[kind].summary);assert.ok(!('blockers' in value),'fields outside the step are dropped');assert.equal(seen.job.mode,mode,kind);assert.equal(seen.job.repo,jobRoot);
    assert.deepEqual(seen.schema.required,schemaFor(kind).required,kind);assert.equal(seen.argv[0],process.execPath);assert.match(seen.argv[1],/Workflow-Runner\.cjs$/);
    assert.ok(!/["%!]/.test(seen.title)&&seen.title.includes('&'),'terminal titles drop only quote-breaking characters');assert.ok(!fs.existsSync(seen.argv[2]),'job files are removed');
  }
  await assert.rejects(runJob({root:jobRoot,command:{file:'codex'},request:jobRequest('fix'),open:fakeRunner({ok:false,error:'로그인 필요'}),wait:5}),/로그인 필요/);
  await assert.rejects(runJob({root:jobRoot,command:{file:'codex'},request:jobRequest('fix'),open:fakeRunner(null,999999),wait:5}),/결과 없이 닫혔습니다/);
  await assert.rejects(runJob({root:jobRoot,command:{file:'codex'},request:jobRequest('plan'),open:fakeRunner({ok:true,value:research}),wait:5}),/질문이나 구현 계획/);
  await assert.rejects(runJob({root:jobRoot,command:null,request:jobRequest('plan',{provider:'claude'})}),/Claude Code/);
  // 실제 실행기와 가짜 Codex로 끝까지 확인한다: 정하기는 read-only, 구현은 danger-full-access 샌드박스다.
  const fakeCodex=path.join(base,'fake-codex.cjs');
  fs.writeFileSync(fakeCodex,`const fs=require('fs');const a=process.argv.slice(2);let input='';process.stdin.on('data',d=>input+=d).on('end',()=>{console.log('fake codex progress');const report=${JSON.stringify({summary:'',evidence:['가짜'],changes:[],questions:[],plan:'계획',checklist:passedRows})};report.summary='sandbox='+a[a.indexOf('--sandbox')+1]+' plan='+input.includes('지금은 정하기');fs.writeFileSync(a[a.indexOf('--output-last-message')+1],JSON.stringify(report));process.exit(a.includes('--fail')?1:0);});`);
  const runner=(_root,_title,argv)=>{spawn(argv[0],argv.slice(1),{stdio:'ignore',env:{...process.env,WX_RUNNER_CLOSE_SECONDS:'0'}});};
  assert.equal((await runJob({root:jobRoot,command:{file:process.execPath,args:[fakeCodex]},request:jobRequest('plan'),open:runner,wait:20})).summary,'sandbox=read-only plan=true');
  assert.equal((await runJob({root:jobRoot,command:{file:process.execPath,args:[fakeCodex]},request:jobRequest('implement'),open:runner,wait:20})).summary,'sandbox=danger-full-access plan=false');
  await assert.rejects(runJob({root:jobRoot,command:{file:process.execPath,args:[fakeCodex,'--fail']},request:jobRequest('implement'),open:runner,wait:20}),/Codex 처리에 실패/);
  assert.deepEqual(fs.readdirSync(path.join(jobRoot,'Saved/Workflow/jobs')),[],'no job folders remain');

  // 터미널 창은 cmd start로 연다. 따옴표를 깨거나 변수로 펼쳐지는 문자가 있으면 열지 않는다.
  let started;
  const start=(file,args,options)=>{started={file,args,options};return {unref(){}};};
  openTerminal('C:\\Wx','Wx AI · a"b&c',['C:\\Program Files\\nodejs\\node.exe','C:\\Wx\\run.cjs'],start);
  assert.equal(started.file,'cmd.exe');assert.deepEqual(started.args,['/d','/c','start "Wx AI · a b&c" /D "C:\\Wx" "C:\\Program Files\\nodejs\\node.exe" "C:\\Wx\\run.cjs"']);
  assert.deepEqual([started.options.detached,started.options.windowsVerbatimArguments,started.options.windowsHide,started.options.stdio],[true,true,true,'ignore']);
  for(const bad of ['50%','a"b','a!b','x\ny'])assert.throws(()=>openTerminal('C:\\Wx','t',[bad],start),/쓸 수 없는 문자/,bad);
  openTerminal('C:\\R&D (x)','t',['C:\\R&D\\a^b|c.exe'],start);assert.match(started.args[2],/"C:\\R&D\\a\^b\|c\.exe"/,'quoted cmd characters are allowed');
  let session;
  const capture=(root,title,argv)=>{session={root,title,argv};};
  const sessionPath='.agents/workflow/tasks/보스-체력바.md';
  openSession({root:'C:\\Wx',command:{file:'claude.exe',args:[]},provider:'claude',taskPath:sessionPath,title:'보스',open:capture});
  assert.equal(session.title,'Wx AI · 보스');assert.equal(session.argv.length,2);assert.equal(session.argv[0],'claude.exe');
  assert.equal(session.argv[1],`Continue the Wx task recorded in ${sessionPath}. Follow AGENTS.md and .agents/workflow/process/index.md.`);
  openSession({root:'C:\\Wx',command:{file:'node.exe',args:['gemini.js']},provider:'gemini',taskPath:sessionPath,title:'보스',open:capture});assert.deepEqual(session.argv.slice(0,3),['node.exe','gemini.js','-i']);
  assert.throws(()=>openSession({root:'C:\\Wx',command:null,provider:'codex',taskPath:sessionPath,title:'보스',open:capture}),/Codex CLI/);

  // Wiki 갱신: 준비물(WSL, claude-obsidian)을 확인·설치하고 origin/main의 sparse 작업 트리에서 고른 AI를 돌린다. claude-obsidian 명령은 래퍼가 WSL에서 실행한다.
  const wikiRoot=path.join(base,'wiki-update'),tree=path.join(wikiRoot,'Saved/Workflow/wiki-update-tree'),pluginDir=path.join(wikiRoot,'Saved/Workflow/claude-obsidian/v9.9.9');
  const wikiCli=path.join(__dirname,'Wiki-Obsidian.cjs');
  let calls=[],wslState='nowsl',engineFails=false,engineCwd='',ran=null,runResult={summary:'원자료 1건 수집, lint 0, 커밋 abc1234',evidence:['lint 0']};const launched=[];
  const exec=async(file,args,options={})=>{
    calls.push([file,...args].join(' '));
    if(file==='wsl.exe'&&args.includes('python3')){if(wslState!=='ready')throw Error('no python');return 'Python 3.14.4';}
    if(file==='wsl.exe'&&args[0]==='-l'){if(wslState==='nowsl')throw Error('WSL is not installed');return {broken:'Ubuntu-24.04\r\nUbuntu',nodistro:'Ubuntu-24.04'}[wslState]||'';}
    if(file==='reg.exe'){if(wslState!=='reboot')throw Error('key not found');return 'RebootPending';}
    if(file===process.execPath){engineCwd=options.cwd;if(engineFails)throw Object.assign(Error('Command failed: node Wiki-Obsidian.cjs --version'),{stderr:'python3: note\nmount: permission denied\n'});return '2.2.0';}
    if(file==='git'&&args[0]==='worktree'&&args[1]==='add'){fs.mkdirSync(path.join(tree,'Wiki'),{recursive:true});fs.writeFileSync(path.join(tree,'.git'),'gitdir: x');fs.writeFileSync(path.join(tree,'Wiki/README.md'),"claude plugin marketplace add 'AgriciDaniel/claude-obsidian#v9.9.9'\n");}
    if(file==='git'&&args.includes('clone')){const dest=args.at(-1);fs.mkdirSync(path.join(dest,'scripts'),{recursive:true});fs.writeFileSync(path.join(dest,'scripts/claude-obsidian.py'),'');}
    return '';
  };
  const run=async options=>{ran=options;if(runResult instanceof Error)throw runResult;return runResult;};
  // 설치 안내 창은 실제 터미널 열기 검사를 거친다(창 인자에 쓸 수 없는 문자가 있으면 여기서 실패).
  const update=createWikiUpdate({root:wikiRoot,providers:['codex','claude'],commands:{codex:{file:'codex'},claude:{file:'claude'}},exec,open:(root,title,argv)=>{openTerminal(root,title,argv,()=>({unref(){}}));launched.push([title,...argv].join(' '));},run,now:()=>'2026-09-26T00:00:00Z'});
  const settleUpdate=async()=>{for(let i=0;i<100&&update.isBusy();i++)await new Promise(resolve=>setImmediate(resolve));};
  const updateState=()=>update.act({action:'status'});
  assert.deepEqual(updateState(),{status:'idle'});
  assert.throws(()=>update.act({action:'fire'}),/형식 오류/);
  assert.throws(()=>update.act({action:'start',provider:'gemini'}),/사용할 수 없습니다/,'only connected AIs can update the Wiki');
  // WSL이 없으면 관리자 승인 창으로 WSL과 Ubuntu를 설치한다. --no-launch라 Linux 사용자 만들기 창이 없고, Git은 건드리지 않는다.
  assert.equal(update.act({action:'start',provider:'claude'}).status,'preparing');await settleUpdate();
  assert.equal(updateState().status,'setup');assert.match(updateState().message,/관리자 승인/);assert.doesNotMatch(updateState().message,/Linux 사용자/);
  assert.equal(launched.length,1);assert.match(launched[0],/^Wx · WSL 설치 powershell\.exe -NoExit -NoProfile -Command /,'a visible window stays open with the instructions');
  assert.match(launched[0],/Start-Process -Verb RunAs -FilePath wsl\.exe -ArgumentList '--install','Ubuntu','--no-launch' -Wait/);assert.match(launched[0],/설치를 시작하지 못했습니다/,'a declined or failed elevation is shown in the window');
  assert.ok(!calls.some(c=>c.startsWith('git ')));
  // 재부팅이 대기 중이면(WSL을 막 설치한 뒤) 설치 창을 다시 열지 않고 재부팅을 안내한다.
  wslState='reboot';update.act({action:'start',provider:'claude'});await settleUpdate();
  assert.deepEqual([updateState().status,launched.length],['setup',1]);assert.match(updateState().message,/다시 시작해야/);
  // WSL은 있는데 Ubuntu가 없으면(다른 배포판만 있어도) 관리자 승인 없이 Ubuntu만 설치한다.
  wslState='nodistro';update.act({action:'start',provider:'claude'});await settleUpdate();
  assert.deepEqual([updateState().status,launched.length],['setup',2]);assert.match(updateState().message,/Ubuntu 설치 창/);
  assert.match(launched[1],/-Command wsl\.exe --install Ubuntu --no-launch;/);assert.doesNotMatch(launched[1],/RunAs/);
  // Ubuntu는 있는데 python3를 못 부르면 설치 창 없이 확인할 곳을 알린다.
  wslState='broken';update.act({action:'start',provider:'claude'});await settleUpdate();
  assert.deepEqual([updateState().status,launched.length],['failed',2]);assert.match(updateState().error,/wsl -l -v/);
  // 준비되면 작업 트리를 LF sparse로 만들고, README의 태그로 claude-obsidian을 받아, 래퍼로 WSL 실행을 확인한 뒤 고른 AI를 그 작업 트리에서 돌린다.
  wslState='ready';calls=[];
  update.act({action:'start',provider:'claude'});
  assert.throws(()=>update.act({action:'start',provider:'codex'}),/이미 진행 중/,'one Wiki update at a time');
  await settleUpdate();
  assert.deepEqual([updateState().status,updateState().provider,updateState().summary],['complete','claude',runResult.summary]);
  assert.deepEqual(calls.filter(c=>c.startsWith('git ')),['git fetch --quiet origin main','git worktree prune','git config extensions.worktreeConfig true',`git worktree add --quiet --no-checkout --detach ${tree} origin/main`,`git -C ${tree} config --worktree core.autocrlf false`,
    `git -C ${tree} sparse-checkout set --no-cone /Wiki/ /Docs/**/*.md /.agents/workflow/tasks/ /AGENTS.md /.gitattributes`,`git -C ${tree} reset --quiet --hard origin/main`,`git -C ${tree} clean -q -fdx`,
    `git -c core.autocrlf=false clone --quiet --depth 1 --branch v9.9.9 https://github.com/AgriciDaniel/claude-obsidian ${pluginDir}.download`]);
  assert.ok(fs.existsSync(path.join(pluginDir,'scripts/claude-obsidian.py'))&&!fs.existsSync(pluginDir+'.download'));
  assert.ok(calls.includes([process.execPath,wikiCli,'--version'].join(' '))&&engineCwd===tree,'the wrapper runs claude-obsidian in WSL from the work tree before the AI starts');
  assert.deepEqual([ran.repo,ran.mode,ran.provider,ran.command.file,ran.title],[tree,'work','claude','claude','Wiki 갱신']);
  assert.match(ran.prompt,/Wiki\/README\.md의 절차대로/);assert.doesNotMatch(ran.prompt,/한국어로|작업 트리 밖/);assert.deepEqual(ran.schema.required,['summary','evidence']);
  const pcInfo=JSON.parse(ran.prompt.match(/이 PC 정보\(JSON\): (.*)/)[1]);
  assert.deepEqual(pcInfo,{worktree:tree,claudeObsidian:{tag:'v9.9.9',path:pluginDir},command:`node "${wikiCli}"`});
  // 두 번째부터는 작업 트리를 origin/main으로 다시 맞추기만 하고, 받아 둔 claude-obsidian은 다시 받지 않는다. AI 실패는 이유를 남긴다.
  calls=[];runResult=Error('Claude Code 처리에 실패했습니다.');
  update.act({action:'start',provider:'codex'});await settleUpdate();
  assert.deepEqual(calls.filter(c=>c.startsWith('git ')),['git fetch --quiet origin main',`git -C ${tree} reset --quiet --hard origin/main`,`git -C ${tree} clean -q -fdx`]);
  assert.deepEqual([updateState().status,updateState().provider,updateState().error],['failed','codex','Claude Code 처리에 실패했습니다.']);
  // WSL에서 claude-obsidian을 못 부르면 AI를 돌리지 않고 첫 줄 이유를 알린다.
  ran=null;engineFails=true;runResult={summary:'x',evidence:[]};
  update.act({action:'start',provider:'codex'});await settleUpdate();
  assert.deepEqual([updateState().status,updateState().error,ran],['failed','WSL에서 claude-obsidian을 실행하지 못했습니다. mount: permission denied',null]);
  engineFails=false;
  // 작업 트리 자리에 Git 작업 트리가 아닌 폴더가 있으면 지우지 않고 알린다.
  fs.rmSync(path.join(tree,'.git'));
  update.act({action:'start',provider:'codex'});await settleUpdate();
  assert.match(updateState().error,/Git 작업 트리가 아닙니다/);assert.ok(fs.existsSync(path.join(tree,'Wiki/README.md')));
  // 래퍼: 저장소 드라이브를 metadata로 붙인 경로로 현재 폴더와 Windows 경로 인자를 옮기고, WSL의 Ubuntu에서 root로 실행한다.
  assert.equal(toWsl('C:\\Wx\\a b\\c.md','C'),'/mnt/wx-c/Wx/a b/c.md');assert.equal(toWsl('D:\\','C'),'/mnt/d');assert.equal(toWsl('D:\\x','D'),'/mnt/wx-d/x');
  const drive=path.resolve(wikiRoot)[0],argv=wslArgs(['capture','apply','--vault','Wiki','C:\\x\\b.json','--out=C:\\x\\y.json','Wiki\\wiki\\a.md'],path.join(tree,'Wiki'),wikiRoot);
  assert.deepEqual(argv.slice(0,7),['-d','Ubuntu','-u','root','-e','sh','-c']);assert.match(argv[7],/mountpoint -q "\$m" \|\| .*mount -t drvfs "\$src" "\$m" -o metadata/);
  assert.deepEqual(argv.slice(8),['sh','/mnt/wx-'+drive.toLowerCase(),drive+':\\',toWsl(path.join(tree,'Wiki')),toWsl(path.join(pluginDir,'scripts/claude-obsidian.py')),'capture','apply','--vault','Wiki',toWsl('C:\\x\\b.json',drive),'--out='+toWsl('C:\\x\\y.json',drive),'Wiki/wiki/a.md']);

  const http=fixture(['codex','claude','gemini']),token='feedback-test',port=18746;
  server=createServer({token,port,testFeedback:http.service,wikiUpdate:update});await new Promise(resolve=>server.listen(port,'127.0.0.1',resolve));
  const post=(route,body,headers={})=>fetch('http://127.0.0.1:'+port+route,{method:'POST',headers:{'Content-Type':'application/json','X-Wx-Token':token,Origin:'null',...headers},body:JSON.stringify(body)});
  assert.equal((await post('/test-feedback',{action:'list'},{'X-Wx-Token':'bad'})).status,403);
  assert.equal((await post('/test-feedback',{action:'list'},{Origin:'https://example.com'})).status,403);
  assert.equal((await post('/test-feedback',http.request({provider:'unknown'}))).status,400);
  const response=await post('/test-feedback',http.request({provider:'claude',checks:[{index:1,result:'실패',note:'문제'}]}));assert.equal(response.status,200);await settle();assert.deepEqual([http.input.provider,http.input.kind],['claude','fix']);
  assert.equal((await post('/test-feedback',{action:'list'})).status,200);
  assert.equal((await post('/analyze',{})).status,404,'removed web task routes must not answer');
  assert.equal((await post('/execution',{})).status,404);
  const health=await (await fetch('http://127.0.0.1:'+port+'/health')).json();
  assert.equal(health.busy,true);assert.deepEqual(Object.keys(health).sort(),['busy','identity']);
  http.finish(fixReport([passedRows[0],row('저장 후 복원','사람','대기','재확인')]));await settle();assert.equal(http.context().latest.status,'retest');
  const createdByHttp=await (await post('/test-feedback',{...createBody,operationId:'http-create',provider:'gemini'})).json();
  assert.equal(createdByHttp.latest.status,'running');await settle();assert.deepEqual([http.input.provider,http.input.kind],['gemini','plan']);
  assert.deepEqual(await (await post('/test-feedback',{action:'terminal',taskPath,provider:'codex'})).json(),{opened:true});
  // Wiki 갱신 경로는 접속 토큰을 거친다. 진행 중이면 서버가 바쁘다고 알려 그동안 서버를 다시 띄우지 않는다.
  assert.equal((await post('/wiki-update',{action:'status'},{'X-Wx-Token':'bad'})).status,403);
  assert.equal((await (await post('/wiki-update',{action:'status'})).json()).status,'failed');
  assert.equal((await post('/wiki-update',{action:'start',provider:'gemini'})).status,400);
  fs.writeFileSync(path.join(tree,'.git'),'gitdir: x');runResult=new Promise(()=>{});
  const wikiServer=createServer({token,port:port+1,wikiUpdate:update});await new Promise(resolve=>wikiServer.listen(port+1,'127.0.0.1',resolve));
  try{
    assert.equal((await (await fetch('http://127.0.0.1:'+(port+1)+'/health')).json()).busy,false);
    update.act({action:'start',provider:'codex'});
    assert.equal((await (await fetch('http://127.0.0.1:'+(port+1)+'/health')).json()).busy,true,'a running Wiki update keeps the server from being replaced');
  }finally{wikiServer.closeAllConnections();await new Promise(resolve=>wikiServer.close(resolve));}
  console.log('PASS record state lines, checklist format, per-step result fields, instant completion without AI, record-only partial results, fixes on failures, hand-over of unrunnable AI items, conflicts, new task flow, Korean record names, retry/restart, worker lock, prompts per step, terminal runner end to end, terminal launch, HTTP routing and the local Wiki update (prerequisites, LF sparse worktree, pinned claude-obsidian, WSL wrapper, one run at a time)');
}finally{
  if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
  const resolved=path.resolve(base);assert.equal(path.dirname(resolved),path.resolve(os.tmpdir()));assert.ok(path.basename(resolved).startsWith('wx-test-feedback-'));fs.rmSync(resolved,{recursive:true,force:true});
}})().catch(error=>{console.error(error);process.exitCode=1;});
