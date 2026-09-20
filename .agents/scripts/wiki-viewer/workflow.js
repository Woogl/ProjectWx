// Copyright Woogle. All Rights Reserved.
const workflowKey = 'wx-wiki-workflow-v1:' + location.pathname;
const newLoop = () => ({ notes: '', decisions: [], result: null, reviewBasis: '', confirmation: null, archive: [] });
const newTask = (title = '') => ({ version: 2, taskId: title, title, serverRevision: 0, source: '', sourceVersions: [], planning: newLoop(), implementation: newLoop() });
const storedObject = value => value !== null && typeof value === 'object' && !Array.isArray(value);
function normalizeStoredTask(task) {
  if (!storedObject(task)) return null;
  const normalized = { ...task, source: typeof task.source === 'string' ? task.source : '', sourceVersions: Array.isArray(task.sourceVersions) ? task.sourceVersions : [] };
  for (const stage of ['planning', 'implementation']) {
    const saved = storedObject(task[stage]) ? task[stage] : {};
    const loop = { ...newLoop(), ...saved };
    for (const key of ['notes', 'reviewBasis']) if (typeof loop[key] !== 'string') loop[key] = '';
    for (const key of ['decisions', 'archive']) if (!Array.isArray(loop[key])) loop[key] = [];
    loop.decisions = loop.decisions.filter(storedObject);
    // 불완전한 판단 자료를 기본값으로 보완한 경우 기존 확정으로 취급하지 않는다.
    if (Object.keys(newLoop()).some(key => JSON.stringify(saved[key]) !== JSON.stringify(loop[key]))) { loop.reviewBasis = ''; loop.confirmation = null; }
    normalized[stage] = loop;
  }
  return normalized;
}
let workflowState = newTask();
let analysisBusy = false;
if(typeof window!=='undefined')window.addEventListener('beforeunload',event=>{
  if(!analysisBusy&&!Object.keys(draftRecovery).length)return;
  event.preventDefault();event.returnValue='';
});
let workflowStorageError = '';
const requestId = () => 'request-' + (typeof crypto !== 'undefined' ? crypto.randomUUID() : Date.now().toString(36)+'-'+Math.random().toString(36).slice(2));
let otherTasks = Object.create(null);
let pendingSave = null;
try { pendingSave=JSON.parse(localStorage.getItem(workflowKey+':pending')||'null'); } catch { workflowStorageError='저장 결과를 확인하지 못했습니다.'; }
let sharedReady=false, sharedLoading=false, sharedVersions=Object.create(null), sharedSavePromise=null;
let draftRecovery=Object.create(null);
try { draftRecovery=JSON.parse(localStorage.getItem(workflowKey+':draft-recovery')||'{}'); } catch { workflowStorageError='입력 복구 기록을 읽지 못했습니다.'; }
function storeRecovery(){localStorage.setItem(workflowKey+':draft-recovery',JSON.stringify(draftRecovery));}
function sharedStatus(text){workflowStorageError=text;const node=typeof document!=='undefined'?document.getElementById('workflow-shared-status'):null;if(node)node.textContent=text;}
function saveWorkflow() {
  if(!workflowState.taskId)return true;
  try {
    otherTasks[workflowState.taskId]=workflowState;
    draftRecovery[workflowState.taskId]={task:JSON.parse(JSON.stringify(workflowState)),expectedVersion:sharedVersions[workflowState.taskId]||0,operationId:requestId()};
    storeRecovery();sharedStatus('공용 파일 저장 중…');
    if(sharedReady&&!pendingSave)flushSharedTasks().catch(error=>sharedStatus(error.message));
    return true;
  } catch {sharedStatus('입력 복구 기록을 저장하지 못했습니다. 페이지를 닫지 마세요.');return false;}
}
async function flushSharedTasks(){
  if(!sharedReady)return;
  if(sharedSavePromise)return sharedSavePromise;
  sharedSavePromise=(async()=>{
    for(const title of Object.keys(draftRecovery)){
      while(draftRecovery[title]){
        const request=draftRecovery[title];
        const receipt=await workflowRequest('/task',request);
        sharedVersions[title]=receipt.revision;
        if(draftRecovery[title]===request)delete draftRecovery[title];
        else draftRecovery[title].expectedVersion=receipt.revision;
        storeRecovery();
      }
    }
    sharedStatus('공용 파일에 저장됨');
  })();
  try{await sharedSavePromise;}finally{sharedSavePromise=null;}
}
async function loadSharedTasks(){
  const result=await workflowRequest('/tasks',{});
  otherTasks=Object.create(null);sharedVersions=Object.create(null);
  for(const [title,record] of Object.entries(result.tasks)){
    otherTasks[title]=normalizeStoredTask(record.task);sharedVersions[title]=record.revision;
  }
}
async function initializeSharedTasks(){
  if(sharedLoading)return;
  sharedLoading=true;
  try{
    await loadSharedTasks();sharedReady=true;
    if(!pendingSave){await flushSharedTasks();await loadSharedTasks();}
    sharedStatus('');
  }catch(error){sharedStatus(error.message);}finally{sharedLoading=false;if(typeof readRoute==='function')readRoute();}
}
// 화면 진입·복귀 때만 동기화한다. 편집 중인 입력은 조회 응답으로 덮어쓰지 않는다.
let sharedSyncing=false;
async function syncSharedTasks(){
  if(analysisBusy||sharedLoading||sharedSyncing)return;
  sharedSyncing=true;
  try{
    if(!sharedReady)await initializeSharedTasks();
    if(!sharedReady)return;
    if(pendingSave)await finishPendingSave();
    await flushSharedTasks();
    const before=JSON.stringify(workflowState);
    await loadSharedTasks();
    if(Object.keys(draftRecovery).length||before!==JSON.stringify(workflowState))return;
    if(workflowState.taskId)workflowState=otherTasks[workflowState.taskId]||newTask();
    sharedStatus('');
    if(typeof readRoute==='function')readRoute();
  }catch(error){sharedStatus(error.message);if(typeof readRoute==='function')readRoute();}finally{sharedSyncing=false;}
}
if(typeof window!=='undefined'){
  window.addEventListener('focus',syncSharedTasks);
  window.addEventListener('online',syncSharedTasks);
}
function workflowButton(text, action) {
  const b = el('button', text, 'back'); b.type = 'button'; b.onclick = action; return b;
}
function sourceFor(stage) { return stage === 'planning' ? workflowState.source : workflowState.planning.confirmation?.draft || ''; }
function reviewSourceFor(stage) { return stage === 'planning' ? sourceFor(stage) : JSON.stringify({ source: sourceFor(stage), upstream: workflowState.planning.confirmation?.path, upstreamRevision: workflowState.planning.confirmation?.revision, upstreamBasis: workflowState.planning.confirmation?.basis }); }
function loopBasis(stage) { const l = workflowState[stage]; return WxWorkflowModel.basis(reviewSourceFor(stage), l.notes, l.decisions); }
function loopReady(stage) { return (stage === 'planning' || (loopConfirmed('planning') && !workflowState[stage].decisions.some(q=>WxWorkflowModel.active(q)&&q.kind==='planning'))) && WxWorkflowModel.ready(workflowState[stage], reviewSourceFor(stage)); }
function handoffBlockers(stage) {
  const l=workflowState[stage],reasons=[];
  if(!sourceFor(stage).trim())reasons.push('기획서 원본이 필요합니다.');
  if(!l.result)reasons.push('AI 검토를 먼저 실행하세요.');
  const unanswered=l.decisions.filter(q=>WxWorkflowModel.active(q)&&(!q.confirmed||!q.answer.trim()));
  if(unanswered.length)reasons.push('답변 결정 필요: '+unanswered.map(q=>q.title).join(', '));
  for(const blocker of l.result?.blockers||[])reasons.push('미해결 사항: '+blocker);
  if(l.result&&l.reviewBasis!==loopBasis(stage))reasons.push('변경한 답변·원본을 AI 검토에 반영해야 합니다.');
  if(stage==='implementation'&&!loopConfirmed('planning'))reasons.push('기획을 먼저 최종 확정하세요.');
  if(stage==='implementation'&&l.decisions.some(q=>WxWorkflowModel.active(q)&&q.kind==='planning'))reasons.push('기획 변경 질문을 기획 단계로 전달해 확정하세요.');
  return reasons;
}
function announceWorkflow(text) {
  const status=el('p',text,'notice');status.setAttribute('role','status');status.setAttribute('tabindex','-1');
  $('workflow-controls').append(status);status.focus();status.scrollIntoView?.({block:'center'});
}
function loopConfirmed(stage) {
  const l = workflowState[stage];
  return !!(loopReady(stage) && l.confirmation && l.confirmation.basis === loopBasis(stage) && l.confirmation.draft === l.result.draft);
}
function invalidate(stage) {
  invalidateState(workflowState,stage);
}
function invalidateState(state,stage) {
  const l = state[stage];
  if (l.confirmation) l.archive.push(l.confirmation);
  l.confirmation = null;
  if (stage === 'planning') invalidateState(state,'implementation');
}
async function finishPendingSave() {
  if(!pendingSave)return;
  if(analysisBusy)throw new Error('진행 중인 작업이 끝난 뒤 다시 시도하세요.');
  analysisBusy=true;
  const panel=$('workflow-controls');panel.inert=true;
  $('workflow-busy-title').textContent='인계 기록을 저장하고 있습니다';
  $('workflow-busy-detail').textContent='저장 결과를 확인하고 있습니다. 완료되면 인계 경로를 표시합니다.';
  $('workflow-busy').hidden=false;$('app-shell').inert=true;$('workflow-busy').focus?.();
  try {
    const operation=pendingSave;
    const result=await workflowRequest(operation.endpoint,operation.body);
    workflowState=JSON.parse(JSON.stringify(operation.next));
    workflowState.serverRevision=result.revision;
    if(operation.endpoint==='/rename'){
      const oldName=operation.body.taskId,newName=operation.body.title;
      const renamed=Object.create(null);
      for(const [name,task] of Object.entries(otherTasks)){
        const updated=WxWorkflowModel.remapTask(task,oldName,newName);
        if(result.revisions?.[updated.taskId]!==undefined)updated.serverRevision=result.revisions[updated.taskId];
        renamed[name===oldName?newName:name]=updated;
      }
      otherTasks=renamed;
      workflowState=WxWorkflowModel.remapTask(workflowState,oldName,newName);
    }
    if(operation.endpoint==='/change'){
      const parent=JSON.parse(JSON.stringify(operation.parent));
      parent.serverRevision=result.revision;parent.changeTo=result.taskId;
      otherTasks[parent.taskId]=parent;
      workflowState.serverRevision=result.childRevision;workflowState.changeFrom=result.changeFrom;
    }
    if(operation.endpoint==='/revoke'&&result.preserved){
      for(const stage of operation.body.stage==='planning'?['planning','implementation']:['implementation']){
        const loop=workflowState[stage];if(!loop.confirmation&&loop.archive.length)loop.confirmation=loop.archive.pop();
      }
    }
    if(operation.endpoint==='/handoff'){
      const l=workflowState[operation.body.stage];
      invalidate(operation.body.stage);
      l.confirmation={basis:operation.basis,draft:operation.body.result.draft,path:result.path,dataPath:result.dataPath,confirmedAt:operation.body.confirmedAt,revision:result.revision};
    }
    if(operation.endpoint==='/rename'&&sharedReady){
      const records=await workflowRequest('/tasks',{});
      for(const [title,record] of Object.entries(records.tasks))sharedVersions[title]=record.revision;
      delete draftRecovery[operation.body.taskId];
    }
    if(operation.endpoint==='/change'&&sharedReady){
      const parent=otherTasks[operation.parent.taskId];
      draftRecovery[parent.taskId]={task:parent,expectedVersion:sharedVersions[parent.taskId]||0,operationId:requestId()};
    }
    if(!saveWorkflow())throw new Error(workflowStorageError);
    await flushSharedTasks();
    // 작업 목록에 서버 응답을 반영한 뒤 재시도 요청을 지운다.
    localStorage.removeItem(workflowKey+':pending');pendingSave=null;
  } finally {analysisBusy=false;panel.inert=false;$('workflow-busy').hidden=true;$('app-shell').inert=false;}
}
async function commitWorkflow(endpoint,body,next,basis) {
  if(pendingSave)throw new Error('이전 저장을 먼저 복구하세요.');
  await flushSharedTasks();
  const operation={endpoint,body:{...body,operationId:requestId()},next:JSON.parse(JSON.stringify(next)),basis,parent:endpoint==='/change'?JSON.parse(JSON.stringify(workflowState)):undefined};
  if(encodeURIComponent(JSON.stringify(operation.body)).replace(/%[0-9A-F]{2}/gi,'x').length>2*1024*1024)throw new Error('저장 자료가 2MB를 초과했습니다. 작업 범위를 나누세요. 입력은 보존됩니다.');
  // No request is sent unless its exact retry payload is durable in the browser.
  localStorage.setItem(workflowKey+':pending',JSON.stringify(operation));
  pendingSave=operation;
  await finishPendingSave();
}
async function beginChange(stage,next,reason,scope) {
  if(workflowState.changeTo)throw new Error('이미 연결된 변경 작업을 여세요.');
  if(!reason?.trim())throw new Error('바꾸려는 내용을 입력하세요.');
  if(!saveWorkflow())throw new Error(workflowStorageError);
  next ||= JSON.parse(JSON.stringify(workflowState));
  invalidateState(next,stage);
  const base=(taskTitle(workflowState)+' · '+reason.split(/\r?\n/)[0]).replace(/[\\/:*?"<>|\x00-\x1f]/g,' ').slice(0,72).replace(/[. ]+$/,'');
  next.title=base;let sequence=2;
  while(Object.keys(otherTasks).some(name=>name.toLowerCase()===next.title.toLowerCase()))next.title=base+' ('+(sequence++)+')';
  next.taskId=WxWorkflowModel.taskName(next.title);
  next.serverRevision=0;
  delete next.changeTo;
  next[stage].reviewBasis='';
  next[stage].notes=[next[stage].notes,'변경 요청: '+reason].filter(Boolean).join('\n\n');
  await commitWorkflow('/change',{taskId:workflowState.taskId,expectedRevision:workflowState.serverRevision,newTaskId:next.taskId,title:next.title,stage,reason,scope:scope||'변경 요청과 관련된 구현은 영향 분석·새 기준 확정까지 보류. AI가 기존 코드·판단을 대조해 영향 범위와 영향 없는 작업을 구분한다.'},next);
}
async function transferPlanningQuestions(selectedId) {
  const next=JSON.parse(JSON.stringify(workflowState));
  const questions=next.implementation.decisions.filter(q=>WxWorkflowModel.active(q)&&(q.kind==='planning'||q.id===selectedId));
  for(const q of questions){
    let id=q.returnId||('planning-'+q.id).slice(0,72),sequence=2;
    if(!q.returnId)while(next.planning.decisions.some(item=>item.id===id))id=('planning-'+q.id).slice(0,72)+'-'+sequence++;
    q.returnId=id;
    const transferred=JSON.parse(JSON.stringify({...q,id,kind:'planning',status:'active',origin:{stage:'implementation',id:q.id}}));
    const index=next.planning.decisions.findIndex(i=>i.id===id);
    if(index<0)next.planning.decisions.push(transferred);else next.planning.decisions[index]=transferred;
    q.status='transferred';q.transferredTo=id;
  }
  if(next.planning.decisions.length>200)throw new Error('기획 판단이 200개를 초과합니다. 범위를 나누세요.');
  next.planning.reviewBasis='';
  await beginChange('planning',next,questions.map(q=>q.scope+'\n'+q.answer).join('\n\n'),questions.map(q=>q.impact||q.scope).join('\n'));
}
function taskTitle(task) {
  return (typeof task.title==='string'&&task.title.trim()) || (task.source||'').split(/\r?\n/).map(line=>line.replace(/^#+\s*/, '').trim()).find(Boolean)?.slice(0,80) || '제목 없는 작업';
}
function renderTaskPicker(panel) {
  const tasks=!workflowState.taskId?{...otherTasks}:{...otherTasks,[workflowState.taskId]:workflowState};
  const used=new Set();
  const list=el('select');list.id='workflow-task-options';list.setAttribute('aria-label','기존 작업 선택');
  if(!workflowState.taskId){const placeholder=el('option','기존 작업을 선택하세요');placeholder.value='';placeholder.disabled=true;list.append(placeholder);}
  for(const [id,task] of Object.entries(tasks)){
    const base=taskTitle(task);let label=base,index=2;
    while(used.has(label))label=base+' ('+(index++)+')';
    used.add(label);
    const option=el('option',label);option.value=id;list.append(option);
  }
  list.value=!workflowState.taskId?'':workflowState.taskId;
  const input=el('input');input.type='text';input.placeholder='새 작업 제목';input.maxLength=80;input.setAttribute('aria-label','새 작업 제목');input.autocomplete='off';
  const message=el('p','','notice');message.setAttribute('role','status');
  const openTask=async(selected,title)=>{
    if(analysisBusy||pendingSave)return;
    if(!saveWorkflow()){message.textContent=workflowStorageError;list.value=workflowState.taskId;return;}
    try{await flushSharedTasks();await loadSharedTasks();}catch(error){message.textContent=error.message;return;}
    if(!selected&&Object.hasOwn(otherTasks,title)){message.textContent='이미 사용 중인 작업 제목입니다.';return;}
    const previous=workflowState;
    workflowState=selected?JSON.parse(JSON.stringify(otherTasks[selected])):newTask(title);
    if(!saveWorkflow()){
      if(!selected)delete otherTasks[workflowState.taskId];workflowState=previous;
      message.textContent=workflowStorageError;list.value=workflowState.taskId;return;
    }
    location.hash=route('.agents/wiki/workflow/planning.md');renderWorkflow('.agents/wiki/workflow/planning.md');
  };
  list.onchange=()=>{if(Object.hasOwn(tasks,list.value))return openTask(list.value);};
  const apply=workflowButton('새 작업 만들기',()=>{
    const title=input.value.trim();
    try{WxWorkflowModel.taskName(title);if(Object.keys(tasks).some(name=>name.toLowerCase()===title.toLowerCase()))throw new Error('이미 사용 중인 작업 제목입니다. 기존 작업 목록에서 선택하세요.');}catch(error){message.textContent=error.message;input.focus();return;}
    return openTask(null,title);
  });
  input.oninput=()=>{message.textContent='';};
  input.onkeydown=event=>{if(event.key==='Enter'){event.preventDefault();apply.onclick();}};
  const existingRow=el('div',undefined,'task-picker-row'),newRow=el('div',undefined,'task-picker-row');
  const existingLabel=el('label','기존 작업');existingLabel.setAttribute('for',list.id);
  input.id='workflow-new-task';const newLabel=el('label','새 작업');newLabel.setAttribute('for',input.id);
  existingRow.append(existingLabel,list);newRow.append(newLabel,input,apply);
  const status=el('p',workflowStorageError,'notice');status.id='workflow-shared-status';status.setAttribute('role','status');
  panel.append(el('h3','작업'),status,existingRow,newRow,message);
  if(!sharedReady||sharedLoading){list.disabled=true;input.disabled=true;apply.disabled=true;}
  if(!workflowState.taskId)return;
  const progress=el('select');progress.setAttribute('aria-label','현재 진행 단계');
  for(const phase of ['기획','설계','구현','코드 리뷰','테스트','완료','보류']){const option=el('option',phase);option.value=phase;progress.append(option);}
  progress.value=workflowState.progress?.stage||'기획';
  const progressNote=el('textarea');progressNote.rows=2;progressNote.setAttribute('aria-label','진행상황과 다음 할 일');progressNote.placeholder='진행상황·다음 할 일·관련 코드나 검증 자료';progressNote.value=workflowState.progress?.note||'';
  const saveProgress=()=>{workflowState.progress={stage:progress.value,note:progressNote.value};saveWorkflow();};
  progress.onchange=saveProgress;progressNote.oninput=saveProgress;
  panel.append(el('h3','공유 진행상황'),progress,progressNote,el('p','진행 단계는 공유 메모이며 기획·설계 확정이나 리뷰·테스트 승인을 대신하지 않습니다.','notice'));
  const titleInput=el('input');titleInput.type='text';titleInput.value=taskTitle(workflowState);titleInput.maxLength=80;titleInput.setAttribute('aria-label','현재 작업 제목');
  const rename=el('details');rename.append(el('summary','현재 작업 제목 변경'));
  rename.append(titleInput,workflowButton('제목 변경',async()=>{
    if(analysisBusy||pendingSave)return;
    const title=titleInput.value.trim();
    try{WxWorkflowModel.taskName(title);if(Object.keys(otherTasks).some(name=>name!==workflowState.taskId&&name.toLowerCase()===title.toLowerCase()))throw new Error('이미 사용 중인 작업 제목입니다.');}catch(error){message.textContent=error.message;return;}
    const oldName=workflowState.taskId;
    const next=WxWorkflowModel.remapTask(workflowState,oldName,title);next.title=title;
    try{
      if(sharedReady||workflowState.serverRevision||workflowState.changeFrom)await commitWorkflow('/rename',{taskId:workflowState.taskId,expectedRevision:workflowState.serverRevision,title},next);
      else{workflowState=next;delete otherTasks[oldName];if(!saveWorkflow())throw new Error(workflowStorageError);}
      renderWorkflow(decodeURIComponent(location.hash?.slice(1)||'.agents/wiki/workflow/planning.md'));
    }catch(error){message.textContent=error.message;if(pendingSave)renderWorkflow('.agents/wiki/workflow/planning.md');}
  }));
  panel.append(rename);
}
function onWikiNavigation(event) {
  if (analysisBusy) { history.replaceState(null, '', event.oldURL); return; }
  readRoute();
  syncSharedTasks();
}
function renderWorkSummary() {
  const summary = $('work-summary'); summary.replaceChildren();
  $('current-task-title').textContent=!workflowState.taskId?'새 작업을 만들거나 기존 작업을 선택하세요.':'현재 작업 · '+taskTitle(workflowState);
  $('work-summary-error').textContent=workflowStorageError;
  for (const [stage, title] of [['planning','기획'],['implementation','구현'],['testing','테스트'],['completion','완료']]) {
    const card = el('a', undefined, 'card'); card.href = route(`.agents/wiki/workflow/${stage}.md`);
    let status = stage==='testing'?'구현·코드 리뷰 후 진행':'테스트 승인 후 진행';
    if (stage === 'planning' || stage === 'implementation') {
      const l = workflowState[stage];
      const unanswered=l.decisions.filter(q => WxWorkflowModel.active(q)&&!q.confirmed).length;
      status = loopConfirmed(stage) ? '확정됨' : stage==='implementation'&&!loopConfirmed('planning') ? '기획 확정 후 진행' : !l.result ? (stage==='planning'&&!workflowState.source.trim()?'기획서 입력부터 시작':'AI 검토 시작') : unanswered ? unanswered+'개 질문에 답변 필요' : loopReady(stage) ? '최종 확정 대기' : l.result.blockers?.length ? '추가 자료 확인 필요' : '답변 반영·AI 재검토 필요';
      if(workflowState.changeTo)status='후속 변경 작업에서 확인';
    }
    card.append(el('span',title,'eyebrow'),el('h2',status)); summary.append(card);
  }

}
async function workflowRequest(endpoint, body) {
  if (!data.ai) throw new Error('OpenWorkflow.bat을 다시 실행하여 AI 연결을 시작하세요.');
  if(sharedReady&&!pendingSave&&!['/task','/tasks'].includes(endpoint))await flushSharedTasks();
  const response = await fetch(data.ai.url.replace(/\/analyze$/,endpoint), { method:'POST', headers:{'Content-Type':'application/json','X-Wx-Token':data.ai.token}, body:JSON.stringify(body) });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || '요청에 실패했습니다.');
  return result;
}
function renderDecisionLoop(panel, stage) {
  const l = workflowState[stage], title = stage === 'planning' ? '기획' : '설계';
  let refreshHandoffStatus=()=>{};
  const message = el('p',workflowStorageError,'notice'); message.id='workflow-review-status';message.setAttribute('role','status');message.setAttribute('tabindex','-1');
  const preview = el('div');
  const remember = () => { message.textContent = saveWorkflow() ? '공용 파일에 저장 중입니다. 변경한 판단은 AI 재검토 후 통합합니다.' : workflowStorageError; };
  if(workflowState.taskId!==taskTitle(workflowState)){
    panel.append(el('p','이전 식별 방식의 확정 작업입니다. 위의 제목 변경을 누르면 인계 파일과 연결을 제목 기준으로 함께 전환합니다.','notice'));return;
  }
  if(workflowState.changeFrom)panel.append(el('p','이전 작업: '+(otherTasks[workflowState.changeFrom.taskId]?taskTitle(otherTasks[workflowState.changeFrom.taskId]):workflowState.changeFrom.title||'이전 확정 작업'),'notice'),el('p','변경 이유: '+workflowState.changeFrom.reason),el('p',loopConfirmed('implementation')?'변경 설계 확정 · 영향받는 구현·리뷰·테스트를 진행하세요.':'검토 중 보류 범위: '+workflowState.changeFrom.scope));
  if(workflowState.changeTo){
    panel.append(el('p','확정본 보존 · 후속 작업: '+(otherTasks[workflowState.changeTo]?taskTitle(otherTasks[workflowState.changeTo]):'변경 작업')+'에서 최신 기준과 검토 범위를 확인하세요.','notice'));
    if(l.confirmation)panel.append(el('pre',l.confirmation.draft,'plan-preview'));
    panel.append(workflowButton('변경 작업 열기',()=>{
      const next=otherTasks[workflowState.changeTo];
      if(!next){message.textContent='현재 브라우저에 변경 작업이 없습니다. 공용 변경 기록을 확인하세요.';return;}
      workflowState=JSON.parse(JSON.stringify(next));saveWorkflow();renderWorkflow(`.agents/wiki/workflow/${stage}.md`);
    }),message);return;
  }
  if(l.confirmation){
    panel.append(el('h3',title+' 확정본'),el('pre',l.confirmation.draft,'plan-preview'),el('p','인계 문서: '+l.confirmation.path),message);
    const request=el('textarea');request.rows=3;request.setAttribute('aria-label','변경 요청');request.placeholder='바꾸려는 내용과 이유를 적으세요. 기존 자료는 자동으로 연결합니다.';
    panel.append(request,workflowButton(title+' 변경 작업 시작',async()=>{try{await beginChange(stage,undefined,request.value);renderWorkflow(`.agents/wiki/workflow/${stage}.md`);}catch(error){message.textContent=error.message;if(pendingSave)renderWorkflow(`.agents/wiki/workflow/${stage}.md`);}}));
    panel.append(el('p','확정본과 판단 기록을 보존하고 별도 작업에서 변경을 검토합니다. 새 확정본이 해당 기준을 대체합니다.'));
    return;
  }
  if (stage === 'planning') {
    const input = el('textarea'); input.rows=10; input.value=workflowState.source; input.setAttribute('aria-label','기획서 원본');
    input.oninput = () => { if(pendingSave)return;workflowState.source=input.value; invalidate('planning'); remember(); };
    const file = el('input'); file.type='file'; file.accept='.md,.txt,.docx,.pdf,.pptx';
    file.onchange = async () => {
      const selected=file.files?.[0]; if(!selected||analysisBusy||pendingSave)return;
      if(!/\.(md|txt|docx|pdf|pptx)$/i.test(selected.name)||selected.size>20*1024*1024){message.textContent='20MB 이하 .md·.txt·.docx·.pdf·.pptx 파일을 선택하세요.';return;}
      const taskId=workflowState.taskId, previous=workflowState.source;
      analysisBusy=true;panel.inert=true;message.textContent='파일을 읽는 중입니다.';
      try {
        let result;
        if(/\.(md|txt)$/i.test(selected.name))result={text:new TextDecoder('utf-8',{fatal:true}).decode(await selected.arrayBuffer()),warnings:[]};
        else {
          const content=await new Promise((resolve,reject)=>{const reader=new FileReader();reader.onload=()=>resolve(reader.result.split(',')[1]);reader.onerror=()=>reject(new Error('파일을 읽지 못했습니다.'));reader.readAsDataURL(selected);});
          result=await workflowRequest('/import',{name:selected.name,content});
        }
        if(!result.text.trim()||result.text.length>120000)throw new Error('기획서는 1~120,000자여야 합니다. 내용을 나누어 첨부하세요.');
        if(workflowState.taskId!==taskId||workflowState.source!==previous||pendingSave)throw new Error('작업 내용이 바뀌어 첨부를 취소했습니다. 다시 첨부하세요.');
        input.value=result.text;input.oninput();
        message.textContent=workflowStorageError||[selected.name+' 파일을 불러왔습니다.',...result.warnings].join(' ');
      } catch(error){message.textContent=error.message;}
      finally{file.value='';analysisBusy=false;panel.inert=false;}
    };
    panel.append(el('h3','기획서 원본'),input,file,el('p','Markdown · 텍스트 · Word(.docx) · PDF · PowerPoint(.pptx) / 최대 20MB. 문서의 텍스트를 읽으며 이미지·스캔·도표는 포함하지 않습니다.','notice'));
  } else {
    panel.append(el('h3','확정 기획과 판단 기록'),el('pre',sourceFor(stage)||'기획을 먼저 확정하세요.','plan-preview'));
    if(!loopConfirmed('planning')){panel.append(el('p','기획 확정이 필요합니다. 기존 설계 판단은 보존되며 기획 재확정 후 재검토합니다.','notice'));return;}
    panel.append(el('p','기획 인계 문서: '+workflowState.planning.confirmation.path,'notice'));
  }
  const notes=el('textarea');notes.rows=3;notes.value=l.notes;notes.setAttribute('aria-label','추가 요청·수정 의견');
  notes.oninput=()=>{if(analysisBusy||pendingSave)return;l.notes=notes.value;invalidate(stage);remember();refreshHandoffStatus();};
  panel.append(el('h3','추가 요청·수정 의견'),notes);
  const providers=data.ai?.providers || [];
  const providerSelect=el('select');providerSelect.setAttribute('aria-label','검토할 AI 서비스');
  for(const provider of providers){const option=el('option',provider.label);option.value=provider.id;providerSelect.append(option);}
  providerSelect.value=providers.some(p=>p.id===workflowState.provider)?workflowState.provider:(providers[0]?.id||'');
  providerSelect.onchange=()=>{workflowState.provider=providerSelect.value;saveWorkflow();};
  panel.append(el('h3','검토할 AI 서비스'),providerSelect,el('p',providers.length?'선택한 서비스에 검토 자료와 조사에 필요한 파일 내용이 전달됩니다. 해당 CLI의 로그인·기본 모델 설정을 사용합니다.':'사용 가능한 AI가 없습니다. Codex, Claude Code 또는 Gemini CLI 설치·로그인 후 OpenWorkflow.bat을 다시 실행하세요.','notice'));
  const generate=workflowButton(`AI ${title} 검토 · 답변 반영`,async()=>{
    if(analysisBusy)return;
    const source=sourceFor(stage);
    if(!source.trim()||source.length>120000){message.textContent='기획서를 1~120,000자로 입력하세요.';return;}
    if(stage==='implementation'&&!loopConfirmed('planning')){message.textContent='기획을 다시 확정하세요.';return;}
    if(stage==='planning' && !(workflowState.sourceVersions||[]).some(v=>v.source===source)) (workflowState.sourceVersions ||= []).push({source,recordedAt:new Date().toISOString()});
    if(!saveWorkflow()){message.textContent=workflowStorageError;return;}
    const selectedProvider=providerSelect.value;
    const basis=loopBasis(stage),reviewHash=location.hash; analysisBusy=true;generate.disabled=true;providerSelect.disabled=true;preview.replaceChildren();
    $('workflow-busy-title').textContent=`AI가 ${title}을 재검토하고 있습니다`;
    $('workflow-busy-detail').textContent='답변을 반영하고 추가 질문과 통합 초안을 준비하고 있습니다.';
    $('workflow-busy').hidden=false;$('app-shell').inert=true;$('workflow-busy').focus();
    message.textContent='기존 판단을 포함해 사실·추가 질문·통합 초안을 검토하고 있습니다…';
    try {
      const result=WxWorkflowModel.validateResult(await workflowRequest('/analyze',{plan:source,mode:stage,provider:selectedProvider,context:{notes:l.notes,decisions:l.decisions,previousDraft:l.result?.draft||'',planningRecord:stage==='implementation'?workflowState.planning.confirmation:null,changeFrom:workflowState.changeFrom||null}}));
      const applyReview=()=>{
        if(basis!==loopBasis(stage))throw new Error('분석 중 입력이 바뀌었습니다. 입력은 보존했습니다. 다시 검토하세요.');
        const merged=WxWorkflowModel.mergeDecisions(l.decisions,result.questions,result.exclusions||[]);
        invalidate(stage);l.decisions=merged;l.result=result;l.reviewProvider=selectedProvider;l.reviewBasis=loopBasis(stage);
        if(saveWorkflow()){
          renderWorkflow(`.agents/wiki/workflow/${stage}.md`);
          const remaining=l.decisions.filter(q=>WxWorkflowModel.active(q)&&!q.confirmed).length;
          $('workflow-review-status').textContent=remaining?`검토 완료 · 답변이 필요한 질문 ${remaining}개를 확인하세요.`:loopReady(stage)?'검토 완료 · 통합 초안을 확인하고 최종 확정하세요.':'검토 완료 · 확정 전 해결할 자료와 판단을 확인하세요.';
        }else message.textContent=workflowStorageError;
      };
      if(!result.exclusions?.length){applyReview();return;}
      preview.append(el('h3','검토 결과 미리보기'),el('p',result.summary),el('pre',result.draft,'plan-preview'));
      for(const q of result.questions)preview.append(el('h4',q.title),el('p',q.scope),el('p',q.reopenReason));
      for(const e of result.exclusions)preview.append(el('h4','범위 제외 확인 · '+e.id),el('p',e.reason),el('p','인간 판단 근거: '+e.basis));
      preview.append(el('p','아래 반영 버튼은 표시된 범위 제외도 함께 승인합니다. 질문과 이전 답변은 이력에 보존됩니다.','notice'));
      preview.append(workflowButton('검토 결과 반영 · 기존 판단 보존',()=>{
        try {applyReview();} catch(error){message.textContent=error.message;}
      }));
      message.textContent='추가 질문과 통합 초안을 확인하고 반영하세요. 기존 답변은 삭제하지 않습니다.';
    } catch(error){message.textContent='검토를 완료하지 못했습니다. '+error.message+' 답변은 보존되어 있습니다. AI 검토 버튼으로 다시 시도하세요.';}
    finally{
      analysisBusy=false;generate.disabled=false;providerSelect.disabled=false;
      $('workflow-busy').hidden=true;$('app-shell').inert=false;
      if(location.hash!==reviewHash)location.hash=reviewHash;
      $('workflow-review-status').focus();
      $('workflow-review-status').scrollIntoView?.({block:'center',behavior:'smooth'});
    }
  });
  generate.disabled=!providers.length;
  panel.append(generate,message,preview,el('h3',title+' 판단 체크리스트'));
  if(l.decisions.some(q=>WxWorkflowModel.active(q)&&!q.confirmed))panel.append(el('p','현재 질문의 답변을 모두 결정하면 AI가 자동으로 재검토하고 필요한 추가 질문을 제시합니다.','notice'));
  if(!l.decisions.length)panel.append(el('p','판단 항목이 없습니다. AI가 명확한 사실을 정리하고 추가 판단만 질문합니다.'));
  for(const q of l.decisions){
    const group=el('div');group.append(el('h4',`${q.id} · ${q.title}`),el('p','근거: '+q.requirement),el('p',q.scope));
    if(!WxWorkflowModel.active(q)){group.append(el('p',q.status==='excluded'?'범위 제외: '+q.exclusion.reason+' · 근거: '+q.exclusion.basis:'기획으로 전달됨: '+q.transferredTo),el('pre','이전 답변: '+q.answer,'plan-preview'));panel.append(group);continue;}
    const decisionStatus=el('span');decisionStatus.setAttribute('role','status');
    const updateDecisionStatus=()=>{
      const state=q.confirmed?'confirmed':'unanswered';
      group.className='decision-card decision-'+state;
      decisionStatus.className='decision-status';
      decisionStatus.textContent=q.confirmed?'✓ 답변 확정됨':'○ 확정 전';
    };
    updateDecisionStatus();
    group.append(decisionStatus,el('p','AI 제안: '+(q.recommendation||'없음')),el('p','영향: '+q.impact));
    if(q.reopenReason)group.append(el('p','재확인 이유: '+q.reopenReason,'notice'));
    const answer=el('textarea');answer.rows=3;answer.value=q.answer;answer.setAttribute('aria-label',q.id+' 답변');
    const confirm=workflowButton(q.confirmed?'답변 확정됨':'이 답변으로 결정',async()=>{
      if(analysisBusy||pendingSave||q.confirmed)return;
      if(!q.answer.trim()){message.textContent='판단 내용을 입력하세요. 보류·제외도 이유와 범위를 답변할 수 있습니다.';return;}
      q.confirmed=true;invalidate(stage);
      if(!saveWorkflow()){q.confirmed=false;message.textContent=workflowStorageError;return;}
      confirm.textContent='답변 확정됨';
      updateDecisionStatus();
      refreshHandoffStatus();
      if(l.decisions.every(item=>!WxWorkflowModel.active(item)||(item.confirmed&&item.answer.trim()))){
        if(!providers.length){message.textContent='답변을 저장했습니다. AI 연결 후 검토 버튼을 눌러 계속하세요.';return;}
        await generate.onclick();
      }else renderWorkflow(`.agents/wiki/workflow/${stage}.md`);
    });
    answer.oninput=()=>{
      if(pendingSave)return;
      if(q.confirmed)q.history.push({question:q.scope,answer:q.answer,confirmed:true,reason:'사람이 답변 수정'});
      q.answer=answer.value;q.confirmed=false;invalidate(stage);confirm.textContent='이 답변으로 결정';remember();
      updateDecisionStatus();
      refreshHandoffStatus();
    };
    group.append(el('p','AI 선택지를 누르거나 답변을 직접 입력하세요. 답변 칸에 추가 의견을 함께 적을 수 있습니다.','notice'));
    let selectedOption=q.options.find(option=>q.answer===option||q.answer.startsWith(option+'\n\n'))||'';
    for(const option of q.options){
      group.append(workflowButton(option,()=>{
        if(pendingSave)return;
        const extra=selectedOption&&(answer.value===selectedOption||answer.value.startsWith(selectedOption+'\n\n'))?answer.value.slice(selectedOption.length).trimStart():answer.value;
        answer.value=option+(extra?'\n\n'+extra:'');selectedOption=option;answer.oninput();answer.focus();
      }));
    }
    answer.placeholder='직접 답변하거나 선택한 내용에 추가 의견을 적으세요.';
    group.append(answer,confirm);
    if(stage==='implementation')group.append(workflowButton('기획 변경 질문 함께 전달 · 답변 보존',async()=>{
      try{
        await transferPlanningQuestions(q.id);
        location.hash=route('.agents/wiki/workflow/planning.md');renderWorkflow('.agents/wiki/workflow/planning.md');
      }catch(error){message.textContent=error.message;if(pendingSave)renderWorkflow(`.agents/wiki/workflow/${stage}.md`);}
    }));
    if(q.history.length)group.append(el('pre','이전 판단\n'+q.history.map(h=>`${h.question}\n${h.answer}\n${h.reason}`).join('\n\n'),'plan-preview'));
    panel.append(group);
  }
  if(l.result){
    if(stage==='implementation'){
      panel.append(el('h3','AI 구현 목록 · 사람의 판단 항목과 구분'));
      for(const item of l.result.items)panel.append(el('h4',item.title),el('p','요구사항: '+item.requirement),el('p','구현: '+item.scope),el('p','완료 기준: '+item.acceptance),el('p','의존: '+item.dependencies.join(', ')));
    }
    panel.append(el('h3',title+' 통합 초안'),el('pre',l.result.draft,'plan-preview'),el('p',loopReady(stage)?'추가 미확정 항목 없음 · 통합 결과를 최종 판단하세요.':'답변·원본·의견 변경 후 AI 재검토가 필요합니다.','notice'));
    const saveStatus=el('div');saveStatus.setAttribute('role','status');saveStatus.setAttribute('tabindex','-1');
    const finalize=workflowButton(`${title} 최종 확정 · 인계 문서 저장`,async()=>{
      if(analysisBusy)return;
      if(!loopReady(stage)){refreshHandoffStatus();saveStatus.focus();saveStatus.scrollIntoView?.({block:'center'});return;}
      const basis=loopBasis(stage);
      const snapshot=JSON.parse(JSON.stringify({taskId:workflowState.taskId,title:taskTitle(workflowState),expectedRevision:workflowState.serverRevision,stage,provider:l.reviewProvider||null,source:sourceFor(stage),sourceVersions:workflowState.sourceVersions||[],notes:l.notes,decisions:l.decisions,result:l.result,upstream:stage==='implementation'?workflowState.planning.confirmation:null,prior:l.archive.map(c=>c.path),confirmedAt:new Date().toISOString()}));
      finalize.disabled=true;panel.inert=true;
      try{
        await commitWorkflow('/handoff',snapshot,workflowState,basis);
        renderWorkflow(`.agents/wiki/workflow/${stage}.md`);
        announceWorkflow('저장 완료 · '+workflowState[stage].confirmation.path);
      }catch(error){
        if(pendingSave)renderWorkflow(`.agents/wiki/workflow/${stage}.md`);
        announceWorkflow('저장 완료를 확인하지 못했습니다. '+error.message+(pendingSave?' 입력은 유지되며 연결이 돌아오면 저장을 다시 확인합니다.':' 입력은 유지됩니다. 다시 시도하세요.'));
      }finally{finalize.disabled=false;panel.inert=false;}
    });
    refreshHandoffStatus=()=>{
      const reasons=handoffBlockers(stage);
      saveStatus.className=reasons.length?'decision-card decision-unanswered':'decision-card decision-confirmed';
      saveStatus.replaceChildren(el('strong',reasons.length?'아직 인계 문서를 저장할 수 없습니다':'통합 초안 확인 후 최종 확정할 수 있습니다'),...reasons.map(reason=>el('p',reason)));
      finalize.disabled=!!reasons.length;
    };
    refreshHandoffStatus();panel.append(saveStatus,finalize);
  }
  if(loopConfirmed(stage))panel.append(el('p','확정 · AI 인계 문서: '+l.confirmation.path,'notice'));
  if(l.archive.length)panel.append(el('pre','이전 확정본 (현재 작업 기준 아님)\n'+l.archive.map(c=>c.path).join('\n'),'plan-preview'));
}
function renderWorkflow(path){
  const panel=$('workflow-controls');panel.replaceChildren();
  const stage=path.match(/^\.agents\/wiki\/workflow\/(planning|implementation|testing|completion)\.md$/)?.[1];
  panel.hidden=!stage;if(!stage)return;
  panel.append(el('h2',({planning:'기획',implementation:'구현',testing:'테스트',completion:'완료'})[stage]));
  if(!sharedReady){panel.append(el('p',workflowStorageError||'작업 목록을 불러오는 중입니다.','notice'));return;}
  if(pendingSave){panel.append(el('p',workflowStorageError||'저장 결과를 확인하고 있습니다.','notice'));return;}
  const stageBody=el('div');
  renderTaskPicker(panel);panel.append(stageBody);
  if(!workflowState.taskId){stageBody.append(el('p','새 작업 제목을 입력해 시작하세요.','notice'));return;}
  if(stage==='planning'||stage==='implementation')renderDecisionLoop(stageBody,stage);
  else{
    stageBody.append(el('p',loopConfirmed('implementation')?'설계 인계 문서: '+workflowState.implementation.confirmation.path:'유효한 설계 확정이 필요합니다.','notice'));
    stageBody.append(el('p','설계 확정은 코드 구현·인간 코드 리뷰·테스트 승인이 아닙니다. AI는 인계 문서에 구현 버전·검증 근거를 연결하고 사람의 리뷰·수용 판단을 기록합니다.'));
    stageBody.append(el('p','실제 구현과 테스트는 이 문서를 읽는 작업에서 수행합니다. 화면은 게임 코드 실행이나 최종 테스트 승인을 대신하지 않습니다.'));
  }
}
