// Copyright Woogle. All Rights Reserved.
// 작업 진행 패널: 새 작업 요청, 질문 답변, 구현 승인, 사람 항목 테스트 결과, 추가 요청을 로컬 서버를 거쳐 AI에게 전달한다.
let taskJobs=Object.create(null),taskSelected=null,taskContext=null,newTaskOpen=false;
let taskJobsLoaded=false,taskJobsLoading=false,taskSending=false,terminalOpening=false,taskPoll=null,taskProviders=null,providerChoice='',taskListSignature='';
const taskDrafts=Object.create(null);
function availableProviders(){return taskProviders||[{id:'codex',label:'Codex'}];}
function providerLabel(id){return ({codex:'Codex',claude:'Claude Code',gemini:'Gemini CLI'})[id]||id;}
function taskKey(suffix,taskPath=''){return workflowKey+':test-feedback:'+suffix+(taskPath?':'+taskPath:'');}
// 브라우저 저장소를 쓰지 못해도 이 화면이 열려 있는 동안은 메모리 값으로 이어간다.
const storageMemory=new Map();
function storageGet(key){if(storageMemory.has(key))return storageMemory.get(key);try{return JSON.parse(localStorage.getItem(key)||'null');}catch{return null;}}
function storageSet(key,value){storageMemory.set(key,value);try{localStorage.setItem(key,JSON.stringify(value));return true;}catch{return false;}}
function storageRemove(key){storageMemory.delete(key);try{localStorage.removeItem(key);}catch{}}
function localTime(value){const date=new Date(value);return Number.isNaN(date.getTime())?String(value||'-'):date.toLocaleString('ko-KR',{dateStyle:'short',timeStyle:'short'});}
function taskStatusText(status){return ({running:'AI 처리 중',questions:'질문 답변 필요',approval:'구현 승인 필요',issues:'실패 확인 필요',retest:'사람 확인 필요',recorded:'테스트 결과 기록',complete:'완료',empty:'처리 결과 확인',conflict:'기록 충돌',failed:'AI 처리 실패',interrupted:'AI 처리 중단'})[status]||'아직 AI에게 맡긴 일 없음';}
function showTaskMessage(text){$('test-feedback-message').textContent=text;}
// 입력 중인 선택·답변·요청은 항목 이름과 질문 ID로 보존해 기록이 갱신돼도 다른 칸에 옮겨 붙지 않는다.
function taskDraft(taskPath){
  if(!taskDrafts[taskPath]){
    const draft=storageGet(taskKey('draft',taskPath))||{},object=value=>value&&typeof value==='object'?value:{};
    taskDrafts[taskPath]={checks:object(draft.checks),answers:object(draft.answers),message:typeof draft.message==='string'?draft.message:''};
  }
  return taskDrafts[taskPath];
}
function rememberDraft(taskPath){if(!storageSet(taskKey('draft',taskPath),taskDraft(taskPath)))showTaskMessage('브라우저에 입력을 보존하지 못했습니다. 이 화면을 닫기 전에 전달하세요.');}
// 이름은 작업마다 다시 적지 않도록 하나만 기억한다.
function taskActor(){const actor=storageGet(taskKey('actor'));return typeof actor==='string'?actor:'';}
function taskField(label,control){const row=el('label',undefined,'feedback-field');row.append(el('span',label),control);return row;}
function taskInput(tag,label,value,maxLength,onInput){const input=el(tag);if(tag==='input')input.type='text';input.maxLength=maxLength;input.value=value||'';input.setAttribute('aria-label',label);input.oninput=()=>onInput(input.value);return input;}
function actorField(){return taskField('이름',taskInput('input','이름',taskActor(),100,value=>storageSet(taskKey('actor'),value)));}
// 연결되지 않은 AI가 골라져 있으면 다른 AI로 몰래 바꾸지 않고 연결 없음으로 보여준다.
function fillProviders(select,selected){
  select.replaceChildren();
  for(const provider of availableProviders()){const option=el('option',provider.label);option.value=provider.id;select.append(option);}
  if(!availableProviders().some(provider=>provider.id===selected)){const option=el('option',providerLabel(selected)+' · 연결 없음');option.value=selected;option.disabled=true;select.append(option);}
  select.value=selected;
}
// 처리할 AI는 왼쪽 메뉴 아래에서 한 번 고르고, 새 작업·작업 진행·Wiki 갱신이 모두 따른다.
function selectedProvider(){
  if(!providerChoice){const saved=storageGet(taskKey('provider'));providerChoice=typeof saved==='string'?saved:'';}
  return providerChoice||availableProviders()[0]?.id||'codex';
}
function renderProviderChoice(){
  const select=$('ai-provider');fillProviders(select,selectedProvider());select.disabled=!data.ai;
  select.onchange=()=>{providerChoice=select.value;storageSet(taskKey('provider'),select.value);};
}
const providerMissing='왼쪽 메뉴 아래에서 고른 AI가 연결되어 있지 않습니다. 다른 AI를 고르거나 OpenWorkflow.bat을 다시 실행하세요.';
async function loadTaskJobs(){
  if(taskJobsLoading||typeof fetch==='undefined'||!data.ai)return;
  taskJobsLoading=true;
  try{
    const result=await workflowRequest('/test-feedback',{action:'list'}),before=taskSelected&&taskJobs[taskSelected.path]?.latest?.status;
    taskProviders=result.providers||null;renderProviderChoice();taskJobs=result.records;taskJobsLoaded=true;if(Array.isArray(result.tasks))liveTaskRecords=result.tasks;
    // 목록은 기록이나 AI 처리 상태가 바뀔 때만 다시 그려 포커스와 입력을 지킨다.
    const signature=JSON.stringify([liveTaskRecords,Object.values(taskJobs).map(job=>[job.taskPath,job.revision,job.latest?.status])]);
    if(signature!==taskListSignature&&(!location.hash||location.hash==='#'))renderTaskRecords();
    taskListSignature=signature;
    if(taskSelected){
      const job=taskJobs[taskSelected.path];
      if((taskContext?.revision??0)!==(job?.revision??0))await refreshTaskContext(false);
      else renderTaskStatus();
      if(before==='running'&&job?.latest?.status!=='running')showTaskMessage('AI 처리가 끝났습니다 · '+taskStatusText(job?.latest?.status));
    }
  }catch(error){if(taskSelected||newTaskOpen)showTaskMessage(error.message);}
  finally{taskJobsLoading=false;watchTaskJobs();}
}
function watchTaskJobs(){
  if(typeof setTimeout==='undefined'||taskPoll)return;
  if(!Object.values(taskJobs).some(job=>job.latest?.status==='running'))return;
  taskPoll=setTimeout(()=>{taskPoll=null;loadTaskJobs();},3000);
}
// 패널을 열면 제목으로 포커스를 옮겨 키보드·화면 읽기 사용자가 바로 이어간다.
function openTaskShell(title){
  const panel=$('test-feedback-panel'),heading=el('h2',title);heading.tabIndex=-1;panel.hidden=false;panel.replaceChildren(heading);
  const message=el('p','','notice');message.id='test-feedback-message';message.setAttribute('role','status');
  return {panel,heading,message};
}
function showTaskShell(panel,heading){panel.scrollIntoView?.({block:'start',behavior:'smooth'});heading.focus?.({preventScroll:true});}
function openNewTask(){
  if(taskSending)return;
  taskSelected=null;taskContext=null;newTaskOpen=true;
  const {panel,heading,message}=openTaskShell('새 작업'),draft=storageGet(taskKey('new'))||{};
  const remember=patch=>{Object.assign(draft,patch);storageSet(taskKey('new'),draft);};
  const fields=el('fieldset');fields.id='new-task-fields';fields.setAttribute('aria-label','새 작업 입력');
  fields.append(
    taskField('제목',taskInput('input','제목',draft.title,80,value=>remember({title:value}))),
    taskField('요청',Object.assign(taskInput('textarea','요청',draft.request,20000,value=>remember({request:value})),{rows:8,placeholder:'무엇을 왜 바꾸고 싶은지, 알고 있는 제약이나 참고할 기록을 적어주세요.'})),
    actorField());
  const submit=workflowButton('AI에게 전달',()=>sendNewTask());submit.id='new-task-submit';
  panel.append(el('p','요청을 적어 전달하면 왼쪽 메뉴 아래에서 고른 AI가 코드와 Wiki를 읽기 전용으로 조사한 뒤 질문이나 구현 계획을 돌려줍니다. 진행 과정은 새 터미널 창에 보입니다.','notice'),fields,message,submit,
    workflowButton('닫기',()=>{newTaskOpen=false;panel.hidden=true;}));
  renderSendState();showTaskShell(panel,heading);
}
function renderSendState(){
  if(!newTaskOpen)return renderTaskStatus();
  $('new-task-fields').disabled=taskSending;$('new-task-submit').disabled=taskSending;
}
// 새 작업은 기록을 만든 뒤 바로 작업 진행 패널로 넘어간다. 전달하지 못하면 입력을 그대로 두고 같은 버튼으로 다시 전달한다.
async function sendNewTask(){
  if(taskSending)return;
  taskSending=true;renderSendState();
  try{
    const draft=storageGet(taskKey('new'))||{},title=(draft.title||'').trim(),text=(draft.request||'').trim(),actor=taskActor().trim(),provider=selectedProvider();
    if(!title)throw Error('제목을 입력하세요.');
    if(!text)throw Error('요청을 입력하세요.');
    if(!actor)throw Error('이름을 입력하세요.');
    if(!availableProviders().some(p=>p.id===provider))throw Error(providerMissing);
    const record=await workflowRequest('/test-feedback',{action:'create',operationId:requestId(),title,request:text,actor,provider});
    storageRemove(taskKey('new'));
    taskJobs[record.taskPath]={taskPath:record.taskPath,revision:record.revision,latest:record.latest};
    taskRecordFilter='active';
    taskSending=false;await openTaskPanel({title,path:record.taskPath});
    showTaskMessage('새 작업을 만들었습니다. AI가 조사를 마치면 질문이나 구현 계획이 이 화면에 나타납니다.');
    await loadTaskJobs();
  }catch(error){showTaskMessage(error.message);}
  finally{taskSending=false;renderSendState();}
}
async function openTaskPanel(item){
  if(taskSending)return;
  taskSelected=item;taskContext=null;newTaskOpen=false;
  const {panel,heading,message}=openTaskShell(item.title),draft=taskDraft(item.path);
  const next=el('p','','record-next');next.id='task-next';
  const status=el('div');status.id='test-feedback-result';
  const origin=el('details');origin.id='task-origin';origin.hidden=true;
  const plan=el('details');plan.id='task-plan';plan.hidden=true;
  const fields=el('fieldset');fields.id='test-feedback-fields';fields.setAttribute('aria-label','작업 입력');
  const phase=el('div');phase.id='task-phase';
  fields.append(phase,actorField());
  const submit=workflowButton('',()=>sendTaskAction(({questions:'answer',approval:'approve'})[taskPhase()]||'submit'));submit.id='test-feedback-submit';
  const extra=el('details');extra.id='task-request';
  const note=Object.assign(taskInput('textarea','추가 요청',draft.message,10000,value=>{draft.message=value;rememberDraft(item.path);}),{rows:4});
  const hint=el('p','','notice');hint.id='task-request-hint';
  const send=workflowButton('추가 요청 전달',()=>sendTaskAction('request'));send.id='task-request-send';
  extra.append(el('summary','AI에게 추가 요청'),hint,taskField('요청',note),send);
  const terminal=workflowButton('터미널에서 이어하기',()=>openTaskTerminal());terminal.id='task-terminal';
  panel.append(next,status,origin,plan,fields,message,submit,terminal,workflowButton('최신 상태 불러오기',()=>refreshTaskContext()),workflowButton('닫기',()=>{taskSelected=null;panel.hidden=true;}),extra);
  renderTaskPanel();showTaskShell(panel,heading);await refreshTaskContext();
}
// 지금 사람이 할 일: 질문 답변 → 구현 승인 → 사람 항목 테스트. AI가 처리 중이면 기다린다.
function taskPhase(){
  const latest=taskJobs[taskSelected.path]?.latest;
  if(latest?.status==='running')return 'running';
  if(!taskContext)return 'loading';
  if(taskContext.checklistError)return 'error';
  if(taskContext.questions.some(q=>!q.answer))return 'questions';
  if(taskContext.plan.text&&!taskContext.plan.approval)return 'approval';
  if(taskContext.checklist.length&&taskContext.checklist.every(row=>row.result==='통과'))return 'complete';
  return taskContext.checklist.length?'checklist':'empty';
}
function renderTaskPanel(){
  if(!taskSelected)return;
  const next=$('task-next');next.textContent=taskContext?.next?'다음 행동 · '+taskContext.next:'';next.hidden=!taskContext?.next;
  const origin=$('task-origin');origin.replaceChildren(el('summary','요청 원문'),el('pre',taskContext?.request||'','plan-preview'));origin.hidden=!taskContext?.request;
  // 승인된 계획은 접어 두고, 승인 전 계획은 승인 단계에서 펼쳐 보인다.
  const plan=$('task-plan');plan.replaceChildren(el('summary','승인된 구현 계획 · '+(taskContext?.plan.approval||'')),el('pre',taskContext?.plan.text||'','plan-preview'));plan.hidden=!taskContext?.plan.approval;
  renderTaskPhase();renderTaskStatus();
}
function renderTaskPhase(){
  const box=$('task-phase'),phase=taskPhase(),submit=$('test-feedback-submit');box.replaceChildren();
  submit.hidden=!['questions','approval','checklist'].includes(phase);submit.textContent=({questions:'답변 전달',approval:'구현 승인',checklist:'테스트 결과 전달'})[phase]||'';
  // 완료된 작업은 추가 요청을 받지 않는다. 구현 승인 전의 추가 요청은 읽기 전용 조사다.
  $('task-request').hidden=phase==='complete';
  $('task-request-hint').textContent=taskContext?.plan.approval||taskContext?.checklist.length
    ?'승인된 범위 안의 수정은 AI가 바로 고치고, 범위를 바꾸는 요청은 질문이나 새 계획으로 돌려줍니다. 권한 확인 없이 모든 명령을 허용해 처리합니다.'
    :'구현 승인 전이라 AI가 읽기 전용으로 조사해 질문이나 구현 계획으로 답합니다.';
  if(phase==='running')return box.append(el('p','AI가 처리하는 동안에는 입력할 수 없습니다. 진행 과정은 터미널 창에서 볼 수 있고, 끝나면 이 화면이 갱신됩니다.','notice'));
  if(phase==='loading')return box.append(el('p','작업 기록을 불러오는 중입니다.','notice'));
  if(phase==='error')return box.append(el('p',taskContext.checklistError+' 작업 기록의 표를 고친 뒤 최신 상태를 불러오세요.','notice'));
  if(phase==='questions')return renderQuestions(box);
  if(phase==='approval')return renderPlan(box);
  if(phase==='checklist')return renderChecklist(box);
  if(phase==='complete')return box.append(el('p','모든 항목이 통과해 완료된 작업입니다. 재사용할 지식은 매일 정기 갱신이 Wiki에 반영합니다. 새 문제는 새 작업으로 요청하세요.','notice'));
  box.append(el('p','지금 답하거나 확인할 항목이 없습니다. 아래 AI에게 추가 요청으로 이어서 맡기세요.','notice'));
}
// 질문마다 선택지를 고르거나 직접 적는다. 추천은 보여주기만 하고 미리 고르지 않으며, 고른 선택지는 선택 안 함으로 되돌릴 수 있다.
function renderQuestions(box){
  const draft=taskDraft(taskSelected.path),open=taskContext.questions.filter(q=>!q.answer),answered=taskContext.questions.filter(q=>q.answer);
  box.append(el('h3','AI 질문 · '+open.length),el('p','모두 답하면 AI가 다시 조사해 추가 질문이나 구현 계획을 돌려줍니다.','notice'));
  for(const question of open){
    const item=el('div',undefined,'check-item'),entry=draft.answers[question.id]||{choice:'',note:''},options=el('div',undefined,'feedback-options'),choices=[];
    const update=patch=>{draft.answers[question.id]={...(draft.answers[question.id]||{choice:'',note:''}),...patch};rememberDraft(taskSelected.path);};
    item.append(el('strong',question.id+'. '+question.question));
    if(question.recommendation)item.append(el('span','추천 · '+question.recommendation,'check-state'));
    if(question.options.length)for(const [value,label] of [['','선택 안 함'],...question.options.map(option=>[option,option])]){
      const choice=el('label'),radio=el('input');radio.type='radio';radio.name='task-question-'+question.id;radio.value=value;radio.checked=(entry.choice||'')===value;radio.setAttribute('aria-label',question.id+' '+label);
      radio.onchange=()=>{for(const other of choices)other.checked=other===radio;update({choice:value});};
      choices.push(radio);choice.append(radio,el('span',label));options.append(choice);
    }
    const note=Object.assign(taskInput('textarea',question.id+' 직접 답변',entry.note,1000,value=>update({note:value})),{rows:2,placeholder:'선택지에 덧붙이거나 다르게 답할 내용'});
    item.append(options,taskField(question.options.length?'직접 답변(선택)':'답변',note));box.append(item);
  }
  if(!answered.length)return;
  const done=el('details');done.append(el('summary','답한 질문 · '+answered.length));
  for(const question of answered)done.append(el('p',question.id+'. '+question.question+' → '+question.answer));
  box.append(done);
}
function renderPlan(box){
  box.append(el('h3','구현 계획'),el('pre',taskContext.plan.text,'plan-preview'));
  box.append(el('p','승인하면 AI가 이 계획대로 구현하고 테스트 체크리스트를 만듭니다. 구현은 사용자 결정에 따라 권한 확인 없이 모든 명령을 실행하며 진행 과정은 터미널 창에 보입니다. 계획을 바꾸려면 승인하지 말고 AI에게 추가 요청에 적으세요.','notice'));
}
// 항목 왼쪽 체크 칸은 고른 결과(없으면 기록된 결과)를 보여준다. 결과는 글자로도 있으므로 읽기 도구에는 숨긴다.
const checkMarks={'통과':['pass','✓'],'실패':['fail','✕'],'미실행':['skip','–']};
function paintMark(mark,result){const [state,text]=checkMarks[result]||['wait',''];mark.className='check-mark '+state;mark.textContent=text;}
function checkMark(result){const mark=el('span');mark.setAttribute('aria-hidden','true');paintMark(mark,result);return mark;}
// 사람이 할 일을 먼저 보여주고 AI가 확인한 항목은 결과만 짧게 붙인다. 항목마다 박스 하나다.
function renderChecklist(box){
  const draft=taskDraft(taskSelected.path),rows=taskContext.checklist.map((row,index)=>({row,index}));
  const human=rows.filter(({row})=>row.owner==='사람'),ai=rows.filter(({row})=>row.owner!=='사람');
  if(human.every(({row})=>row.result==='통과')&&ai.some(({row})=>row.result!=='통과'))box.append(el('p','사람 항목은 모두 통과했지만 AI 항목이 남아 완료되지 않았습니다. 아래 AI 항목의 근거를 보고 AI에게 추가 요청으로 다시 확인하거나 고치게 하세요.','notice'));
  box.append(el('h3','사람이 확인할 항목 · '+human.length));
  if(!human.length)box.append(el('p','사람이 확인할 항목이 없습니다.','notice'));
  for(const {row,index} of human){
    const item=el('div',undefined,'check-item marked'),entry=draft.checks[row.item]||{result:'',note:''},options=el('div',undefined,'feedback-options'),choices=[],mark=checkMark(entry.result||row.result);
    item.append(mark,el('strong',row.item),el('span','현재 '+row.result+(row.evidence?' · '+row.evidence:''),'check-state'));
    if(row.method)item.append(el('p',row.method));
    const update=patch=>{draft.checks[row.item]={...(draft.checks[row.item]||{result:'',note:''}),...patch};rememberDraft(taskSelected.path);};
    const input=Object.assign(taskInput('textarea',row.item+' 문제 상황과 재현 방법',entry.note,2000,value=>update({note:value})),{rows:3,placeholder:'어디서 무엇을 했는지, 기대한 동작과 실제 문제를 적어주세요.'});
    const note=taskField('문제 상황과 재현 방법',input);note.hidden=entry.result!=='실패';
    for(const [value,label] of [['','이번에 확인 안 함'],['통과','통과'],['실패','실패']]){
      const choice=el('label'),radio=el('input');radio.type='radio';radio.name='test-feedback-check-'+index;radio.value=value;radio.checked=(entry.result||'')===value;radio.setAttribute('aria-label',row.item+' '+label);
      radio.onchange=()=>{for(const other of choices)other.checked=other===radio;note.hidden=value!=='실패';paintMark(mark,value||row.result);update({result:value});};
      choices.push(radio);choice.append(radio,el('span',label));options.append(choice);
    }
    item.append(options,note);box.append(item);
  }
  if(!ai.length)return;
  box.append(el('h3','AI가 확인한 항목 · '+ai.length));
  for(const {row} of ai){const item=el('div',undefined,'check-item marked ai');item.append(checkMark(row.result),el('strong',row.item),el('span',row.result+(row.evidence?' · '+row.evidence:''),'check-state'));box.append(item);}
}
function renderTaskStatus(){
  if(!taskSelected)return;
  const panel=$('test-feedback-result'),latest=taskJobs[taskSelected.path]?.latest;panel.replaceChildren();
  panel.append(el('h3',latest?taskStatusText(latest.status):'아직 AI에게 맡긴 일 없음'));
  if(latest?.status==='running'){
    const minutes=Math.max(0,Math.floor((Date.now()-Date.parse(latest.startedAt||latest.at))/60000));
    panel.append(el('p',`${providerLabel(latest.provider)} 처리 중 · ${minutes}분 경과 · 터미널 창에서 진행 과정을 볼 수 있습니다.`,'notice'));
  }else if(latest){
    panel.append(el('p',`최근 전달 · ${latest.actor||'-'} · ${localTime(latest.at)} · 처리 AI ${providerLabel(latest.provider)}`,'notice'));
    if(latest.checks)for(const check of latest.checks)panel.append(el('p',check.result+' · '+check.item+(check.note?': '+check.note:'')));
    if(latest.answers)for(const answer of latest.answers)panel.append(el('p',answer.id+' 답변 · '+answer.answer));
    if(latest.message)panel.append(el('p','추가 요청 · '+latest.message));
    if(latest.error)panel.append(el('p',latest.error,'notice'));
    if(latest.report){
      const changes=latest.report.changes||[],evidence=latest.report.evidence||[];
      panel.append(el('p',latest.report.summary));
      const detail=el('details');detail.append(el('summary','AI 처리 근거 · 변경 '+changes.length+' · 근거 '+evidence.length));
      for(const change of changes)detail.append(el('p',change));
      for(const item of evidence)detail.append(el('pre',item,'plan-preview'));
      panel.append(detail);
    }
    if(['failed','interrupted'].includes(latest.status))panel.append(workflowButton('저장된 요청으로 AI 다시 시도',()=>sendTaskAction('retry')));
  }
  const busy=taskSending||latest?.status==='running';
  $('test-feedback-fields').disabled=busy;$('test-feedback-submit').disabled=busy||!taskContext;
  $('task-request-send').disabled=busy;$('task-terminal').disabled=busy||terminalOpening;
}
// 자동으로 다시 읽을 때는 방금 보여준 안내를 덮지 않는다.
async function refreshTaskContext(announce=true){
  if(!taskSelected)return;
  const taskPath=taskSelected.path;
  try{
    const context=await workflowRequest('/test-feedback',{action:'read',taskPath});
    if(taskSelected?.path!==taskPath)return;
    taskContext=context;taskProviders=context.providers||null;renderProviderChoice();taskJobs[taskPath]={taskPath,revision:context.revision,latest:context.latest};
    if(announce)showTaskMessage('최신 작업 기록을 불러왔습니다.');
    renderTaskPanel();watchTaskJobs();
  }catch(error){if(taskSelected?.path===taskPath)showTaskMessage(error.message);}
}
// 동작마다 필요한 입력을 모아 보낸다. 전달하지 못하면 입력을 그대로 두고, 같은 일이 두 번 처리되는 것은 서버가 기록 해시와 작업 중 잠금으로 막는다.
async function sendTaskAction(action){
  if(taskSending||!taskSelected)return;
  const item=taskSelected,draft=taskDraft(item.path);
  taskSending=true;renderTaskStatus();
  try{
    if(action==='retry')await refreshTaskContext(false);
    if(!taskContext)throw Error('최신 상태를 먼저 불러오세요.');
    const request={action,provider:selectedProvider(),taskPath:item.path,operationId:requestId(),taskHash:taskContext.taskHash};
    if(action==='submit'){
      const checks=[];
      taskContext.checklist.forEach((row,index)=>{const entry=draft.checks[row.item];if(row.owner==='사람'&&entry?.result)checks.push({index,result:entry.result,note:entry.result==='실패'?entry.note||'':''});});
      if(!checks.length)throw Error('확인한 항목의 결과를 하나 이상 선택하세요.');
      if(checks.some(check=>check.result==='실패'&&!check.note.trim()))throw Error('실패한 항목에는 문제 상황과 재현 방법을 적으세요.');
      request.checks=checks;
    }
    if(action==='answer'){
      request.answers=taskContext.questions.filter(q=>!q.answer).map(q=>{const entry=draft.answers[q.id]||{};return {id:q.id,answer:[entry.choice,(entry.note||'').trim()].filter(Boolean).join(' — ')};});
      const missing=request.answers.filter(answer=>!answer.answer).map(answer=>answer.id);
      if(missing.length)throw Error('답하지 않은 질문이 있습니다: '+missing.join(', '));
    }
    if(action==='request'){request.message=draft.message.trim();if(!request.message)throw Error('추가 요청을 입력하세요.');}
    if(action!=='retry'){request.actor=taskActor().trim();if(!request.actor)throw Error('이름을 입력하세요.');}
    if(!availableProviders().some(p=>p.id===request.provider))throw Error(providerMissing);
    const record=await workflowRequest('/test-feedback',request);
    taskJobs[item.path]=record;
    if(action==='submit')draft.checks={};
    if(action==='answer')draft.answers={};
    if(action==='request')draft.message='';
    rememberDraft(item.path);
    showTaskMessage(record.latest?.status==='complete'?'모든 항목이 통과해 완료했습니다.'
      :record.latest?.status==='recorded'?'테스트 결과를 기록했습니다. 남은 항목을 확인하면 다시 전달하세요.'
      :action==='approve'?'구현을 승인했습니다. AI가 구현하는 과정은 터미널 창에서 볼 수 있습니다.':'AI에게 전달했습니다. 처리 과정은 터미널 창에서 볼 수 있고, 끝나면 이 화면이 갱신됩니다.');
    if(taskSelected?.path===item.path)renderTaskPanel();
    await loadTaskJobs();
  }catch(error){showTaskMessage(error.message);}
  finally{taskSending=false;renderTaskStatus();watchTaskJobs();}
}
// 사람이 AI와 직접 대화하며 이어갈 터미널 창을 연다. 권한 확인은 평소 CLI 설정을 따른다.
async function openTaskTerminal(){
  if(!taskSelected||terminalOpening)return;
  const provider=selectedProvider();
  terminalOpening=true;renderTaskStatus();
  try{
    if(!availableProviders().some(p=>p.id===provider))throw Error(providerMissing);
    await workflowRequest('/test-feedback',{action:'terminal',taskPath:taskSelected.path,provider});
    showTaskMessage(providerLabel(provider)+' 터미널 창을 열었습니다. 창에서 AI와 대화하며 이어가고, 끝나면 최신 상태 불러오기를 누르세요.');
  }catch(error){showTaskMessage(error.message);}
  finally{terminalOpening=false;renderTaskStatus();}
}
