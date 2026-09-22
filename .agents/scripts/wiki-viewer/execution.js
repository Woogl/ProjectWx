// Copyright Woogle. All Rights Reserved.
function executionStatus(){
  const execution=workflowState.execution;
  if(!loopConfirmed('implementation'))return '설계 판단 대기';
  if(!execution)return 'AI 구현 준비';
  return ({running:execution.phase==='verify'?'AI 검증 중':'AI 구현 중',review:'코드 리뷰 필요',acceptance:'결과 확인 필요',cleanup:'정리 대기',complete:'완료',blocked:'AI 작업 확인 필요',interrupted:'AI 작업 중단'})[execution.status]||'확인 필요';
}
function currentWorkStage(){
  if(!loopConfirmed('planning'))return 'planning';
  if(['cleanup','complete'].includes(workflowState.execution?.status))return 'completion';
  if(workflowState.execution?.status==='acceptance'||workflowState.execution?.phase==='verify'&&workflowState.execution?.status==='running')return 'testing';
  return 'implementation';
}
function openWorkStage(){
  const stage=currentWorkStage();
  const path=`.agents/workflow/process/${stage==='planning'?'design_review':stage}.md`;
  if(typeof history!=='undefined')history.replaceState(null,'',route(path));
  else location.hash=route(path);
  if(typeof readRoute==='function')readRoute();
  renderWorkflow(path);
}
let executionPoll=null,executionRequestBusy=false;
function workflowGuideDocument(doc){
  if(!isWorkflow||!workflowState.taskId||!/^\.agents\/workflow\/process\/(design_review|implementation|testing|completion)\.md$/.test(doc.path))return doc;
  const stage=currentWorkStage();
  return docs.find(item=>item.path===`.agents/workflow/process/${stage==='planning'?'design_review':stage}.md`)||doc;
}
async function recoverExecutionRequests(){
  if(executionRequestBusy)return;
  for(const title of Object.keys(otherTasks)){
    const key=workflowKey+':execution:'+title,request=JSON.parse(localStorage.getItem(key)||'null');
    if(!request)continue;
    try{otherTasks[title].execution=await workflowRequest('/execution',request);localStorage.removeItem(key);}
    catch(error){if(error.responded)localStorage.removeItem(key);else throw error;sharedStatus(error.message);}
  }
}
function watchExecution(){
  if(typeof setTimeout==='undefined'||executionPoll)return;
  if(!Object.values(otherTasks).some(task=>task.execution?.status==='running'||localStorage.getItem(workflowKey+':execution:'+task.taskId))&&workflowState.execution?.status!=='running')return;
  executionPoll=setTimeout(async()=>{executionPoll=null;await syncSharedTasks();watchExecution();},3000);
}
async function executeAction(action,feedback='',confirmedChecks=[],actor='',closure){
  if(executionRequestBusy)return;
  executionRequestBusy=true;
  const taskId=workflowState.taskId;
  try{
    await flushSharedTasks();
    // 응답 유실 뒤 같은 요청을 재전송해도 구현은 중복 실행하지 않는다.
    const key=workflowKey+':execution:'+taskId;
    let request=JSON.parse(localStorage.getItem(key)||'null');
    if(!request){request={taskId,action,feedback,confirmedChecks,actor,closure,expectedRevision:workflowState.execution?.revision||0,operationId:requestId()};localStorage.setItem(key,JSON.stringify(request));}
    const result=await workflowRequest('/execution',request);
    localStorage.removeItem(key);
    if(workflowState.taskId===taskId){workflowState.execution=result;otherTasks[taskId]=workflowState;openWorkStage();}
    watchExecution();
  }catch(error){
    if(error.responded)localStorage.removeItem(workflowKey+':execution:'+taskId);
    sharedStatus(error.message);
  }finally{executionRequestBusy=false;watchExecution();}
}
function renderExecution(panel){
  panel.append(el('h3',executionStatus()));
  if(!loopConfirmed('implementation')){
    panel.append(el('p','먼저 기획·설계의 질문에 답해주세요.'));
    panel.append(workflowButton('판단할 내용 보기',()=>openWorkStage(loopConfirmed('planning')?'implementation':'planning')));return;
  }
  const execution=workflowState.execution;
  if(!execution){
    panel.append(el('p','AI가 확정 설계에 따라 구현·검증하고 리뷰할 결과를 준비합니다.'));
    panel.append(workflowButton('AI 구현 시작',()=>executeAction('start')));return;
  }
  if(execution.status==='running'){
    panel.append(el('p','완료되면 판단할 결과를 표시합니다. 다른 작업을 보거나 창을 닫아도 실행은 계속됩니다.'));
    watchExecution();return;
  }
  if(execution.error)panel.append(el('p',execution.error,'notice'));
  const report=execution.report;
  if(report){
    panel.append(el('p',report.summary));
    for(const change of report.changes)panel.append(el('p',change));
    if(report.checks.length){panel.append(el('h4','AI 검증 결과'));for(const check of report.checks)panel.append(el('p',({passed:'통과',failed:'실패',not_run:'미검증'})[check.status]+' · '+check.name),el('pre',check.evidence,'plan-preview'));}
    for(const blocker of report.blockers)panel.append(el('p',blocker,'notice'));
  }
  if(execution.diff){const diff=el('details');diff.append(el('summary','코드 변경 확인'),el('pre',execution.diff,'plan-preview'));panel.append(diff);}
  for(const decision of execution.decisions||[])panel.append(el('p',({approve:'코드 리뷰 승인',accept:'테스트 수용',revise:'수정 요청'})[decision.action]+' · '+(decision.actor||'승인자 미기록')+' · '+decision.at+(decision.feedback?' · '+decision.feedback:''),'notice'));
  if(execution.status==='complete'){
    panel.append(el('p','테스트 수용과 지식·자료 정리가 완료되었습니다.'));
    if(execution.closure){const c=execution.closure;panel.append(el('p','정리 확인 · '+c.actor+' · '+c.at),el('p',(c.wikiStatus==='reflected'?'Wiki 반영: ':'Wiki 생략: ')+c.wikiEvidence),el('p','자료 정리: '+c.cleanupEvidence));}
    return;
  }
  const judgment=workflowState.executionJudgment?.revision===execution.revision?workflowState.executionJudgment:{revision:execution.revision,feedback:'',checks:[],actor:'',wikiStatus:'reflected',wikiEvidence:'',cleanupEvidence:''};
  const remember=()=>{workflowState.executionJudgment=judgment;saveWorkflow();};
  const actor=el('input');actor.type='text';actor.maxLength=100;actor.setAttribute('aria-label','승인자 이름');actor.placeholder='승인자 이름';actor.value=judgment.actor||'';actor.oninput=()=>{judgment.actor=actor.value;remember();};
  if(['review','acceptance','cleanup'].includes(execution.status))panel.append(el('label','승인자 이름 (직접 입력)'),actor);
  if(execution.status==='cleanup'){
    panel.append(el('p','AI가 Wiki 반영·자료 정리를 수행한 결과를 기록하세요. 반영할 지식이 없으면 생략 사유를 남기세요.'));
    const wikiStatus=el('select');wikiStatus.setAttribute('aria-label','Wiki 처리');
    for(const [value,text] of [['reflected','Wiki 반영 완료'],['skipped','반영할 지식 없음']]){const option=el('option',text);option.value=value;wikiStatus.append(option);}
    wikiStatus.value=judgment.wikiStatus||'reflected';wikiStatus.onchange=()=>{judgment.wikiStatus=wikiStatus.value;remember();};panel.append(wikiStatus);
    for(const [key,label] of [['wikiEvidence','Wiki 반영 근거 또는 생략 사유'],['cleanupEvidence','자료 정리 결과']]){const field=el('textarea');field.setAttribute('aria-label',label);field.placeholder=label;field.value=judgment[key]||'';field.oninput=()=>{judgment[key]=field.value;remember();};panel.append(el('label',label),field);}
    panel.append(workflowButton('정리 완료',()=>executeAction('finish','',[],actor.value,{wikiStatus:judgment.wikiStatus||'reflected',wikiEvidence:judgment.wikiEvidence||'',cleanupEvidence:judgment.cleanupEvidence||''})));return;
  }
  const confirmed=judgment.checks;
  if(execution.status==='acceptance'&&report){
    if(report.humanChecks.length)panel.append(el('h4','직접 확인할 사항'));
    report.humanChecks.forEach((text,index)=>{const row=el('label',undefined,'check-row'),check=el('input');check.type='checkbox';check.checked=confirmed.includes(index);check.onchange=()=>{const at=confirmed.indexOf(index);if(check.checked&&at<0)confirmed.push(index);else if(!check.checked&&at>=0)confirmed.splice(at,1);remember();};row.append(check,el('span',text));panel.append(row);});
  }
  const feedback=el('textarea');feedback.rows=2;feedback.setAttribute('aria-label','판단 의견');feedback.placeholder=execution.status==='blocked'?'AI가 확인을 요청한 내용에 답해주세요.':'수정할 내용이 있으면 적어주세요.';
  feedback.value=judgment.feedback;feedback.oninput=()=>{judgment.feedback=feedback.value;remember();};
  panel.append(feedback);
  if(execution.status==='review')panel.append(workflowButton('코드 리뷰 승인',()=>executeAction('approve',feedback.value,[],actor.value)));
  if(execution.status==='acceptance'){
    if(!report.checks.length||report.checks.some(c=>c.status==='not_run'))panel.append(el('p','미검증 항목까지 수용하려면 의견에 이유를 적어주세요.','notice'));
    panel.append(workflowButton('테스트 완료',()=>executeAction('accept',feedback.value,confirmed,actor.value)));
  }
  const stopped=execution.status==='blocked'||execution.status==='interrupted';
  panel.append(workflowButton(stopped?'AI 이어서 진행':'수정 요청',()=>executeAction(stopped&&!feedback.value.trim()?'retry':'revise',feedback.value)));
}
