// Copyright Woogle. All Rights Reserved.
let feedbackRecords=Object.create(null),feedbackSelected=null,feedbackContext=null;
let feedbackLoaded=false,feedbackLoading=false,feedbackSubmitting=false,feedbackPoll=null;
const feedbackDrafts=Object.create(null);
let feedbackAvailableProviders=null;
function feedbackProviders(){return feedbackAvailableProviders||[{id:'codex',label:'Codex'}];}
function feedbackProviderLabel(id){return ({codex:'Codex',claude:'Claude Code',gemini:'Gemini CLI'})[id]||id;}
function feedbackProviderChoice(){return feedbackDraft(feedbackSelected.path).provider||feedbackRecords[feedbackSelected.path]?.latest?.provider||feedbackProviders()[0]?.id||'codex';}
function renderFeedbackProvider(){
  const select=$('test-feedback-provider'),selected=feedbackProviderChoice(),providers=feedbackProviders();select.replaceChildren();
  for(const provider of providers){const option=el('option',provider.label);option.value=provider.id;select.append(option);}
  if(!providers.some(provider=>provider.id===selected)){const option=el('option',feedbackProviderLabel(selected)+' · 연결 없음');option.value=selected;option.disabled=true;select.append(option);}
  select.value=selected;
}
function feedbackKey(taskPath,suffix='draft'){return workflowKey+':test-feedback:'+suffix+':'+taskPath;}
function feedbackStatusText(status){return ({running:'AI 처리 중',complete:'테스트 반영·정리 완료',retest:'수정 결과 재확인 필요',blocked:'AI 확인 요청',failed:'AI 처리 실패',interrupted:'AI 처리 중단'})[status]||'아직 전달한 결과 없음';}
function feedbackDraft(taskPath){
  if(!feedbackDrafts[taskPath]){
    try{feedbackDrafts[taskPath]=JSON.parse(localStorage.getItem(feedbackKey(taskPath))||'null');}catch{}
    feedbackDrafts[taskPath]||={result:'',actor:'',scope:'',notes:''};
  }
  return feedbackDrafts[taskPath];
}
function rememberFeedback(){
  try{localStorage.setItem(feedbackKey(feedbackSelected.path),JSON.stringify(feedbackDraft(feedbackSelected.path)));}
  catch{$('test-feedback-message').textContent='브라우저에 입력을 보존하지 못했습니다. 이 화면을 닫기 전에 결과를 전달하세요.';}
}
async function loadTestFeedback(){
  if(feedbackLoading||typeof fetch==='undefined'||!data.ai)return;
  feedbackLoading=true;
  try{
    const result=await workflowRequest('/test-feedback',{action:'list'});
    feedbackAvailableProviders=result.providers||null;
    feedbackRecords=result.records;feedbackLoaded=true;
    const index=data.documents.find(d=>d.path==='.agents/workflow/tasks/index.md');if(index&&typeof result.indexText==='string')index.text=result.indexText;
    if(!location.hash||location.hash==='#')renderTaskRecords();
    renderFeedbackStatus();
    if(feedbackSelected&&feedbackRecords[feedbackSelected.path]?.latest?.status!=='running'&&feedbackContext?.revision!==feedbackRecords[feedbackSelected.path]?.revision)await refreshFeedbackContext();
  }catch(error){if(feedbackSelected)$('test-feedback-message').textContent=error.message;}
  finally{feedbackLoading=false;watchTestFeedback();}
}
function watchTestFeedback(){
  if(typeof setTimeout==='undefined'||feedbackPoll)return;
  if(!Object.values(feedbackRecords).some(record=>record.latest?.status==='running'))return;
  feedbackPoll=setTimeout(()=>{feedbackPoll=null;loadTestFeedback();},3000);
}
function renderFeedbackStatus(){
  if(!feedbackSelected)return;
  const panel=$('test-feedback-result');panel.replaceChildren();
  const latest=feedbackRecords[feedbackSelected.path]?.latest;
  renderFeedbackProvider();
  panel.append(el('h3',latest?feedbackStatusText(latest.status):'테스트 결과 전달'));
  if(latest){
    panel.append(el('p',(latest.result==='passed'?'이상 없음':'이상 있음')+' · '+latest.actor+' · '+latest.at,'notice'));
    panel.append(el('p','처리 AI: '+feedbackProviderLabel(latest.provider||'codex'),'notice'));
    panel.append(el('p','확인한 항목: '+latest.scope));
    if(latest.notes)panel.append(el('pre',latest.notes,'plan-preview'));
    if(latest.error)panel.append(el('p',latest.error,'notice'));
    if(latest.report){
      panel.append(el('p',latest.report.summary));
      for(const change of latest.report.changes)panel.append(el('p',change));
      for(const check of latest.report.checks)panel.append(el('p',({passed:'통과',failed:'실패',not_run:'미실행'})[check.status]+' · '+check.name),el('pre',check.evidence,'plan-preview'));
      for(const item of [...latest.report.humanChecks,...latest.report.blockers])panel.append(el('p',item,'notice'));
    }
    if(['failed','interrupted'].includes(latest.status))panel.append(workflowButton('저장된 결과로 AI 다시 시도',()=>sendTestFeedback('retry')));
  }
  let pending=false;try{pending=!!localStorage.getItem(feedbackKey(feedbackSelected.path,'pending'));}catch{}
  $('test-feedback-fields').disabled=feedbackSubmitting||latest?.status==='running'||pending;
  $('test-feedback-submit').disabled=feedbackSubmitting||latest?.status==='running'||(!feedbackContext&&!pending);
}
async function openTestFeedback(item){
  if(feedbackSubmitting)return;
  feedbackSelected=item;feedbackContext=null;
  const panel=$('test-feedback-panel');panel.hidden=false;panel.replaceChildren();
  panel.append(el('h2',item.title+' · 테스트 결과'));
  const status=el('div');status.id='test-feedback-result';panel.append(status);
  const fields=el('fieldset');fields.id='test-feedback-fields';fields.append(el('legend','직접 확인한 결과'));
  const draft=feedbackDraft(item.path);if(!draft.scope)draft.scope=item.next;
  const providerRow=el('label',undefined,'feedback-field'),providerSelect=el('select');providerSelect.id='test-feedback-provider';providerSelect.setAttribute('aria-label','처리할 AI');
  providerRow.append(el('span','처리할 AI'),providerSelect);fields.append(providerRow);
  providerSelect.onchange=()=>{draft.provider=providerSelect.value;rememberFeedback();};
  function field(label,value,rows){
    const row=el('label',undefined,'feedback-field'),input=el(rows?'textarea':'input');
    row.append(el('span',label));input.setAttribute('aria-label',label);input.value=value;
    if(rows)input.rows=rows;else input.type='text';row.append(input);fields.append(row);return {row,input};
  }
  const actor=field('확인자',draft.actor).input;actor.maxLength=100;actor.oninput=()=>{draft.actor=actor.value;rememberFeedback();};
  const scope=field('확인한 항목',draft.scope,2).input;scope.maxLength=4000;scope.oninput=()=>{draft.scope=scope.value;rememberFeedback();};
  const options=el('div',undefined,'feedback-options');fields.append(options);
  const notes=field('문제 상황과 재현 방법',draft.notes,4);notes.input.maxLength=10000;notes.input.placeholder='어디서 무엇을 했는지, 기대한 동작과 실제 문제를 적어주세요.';
  notes.input.oninput=()=>{draft.notes=notes.input.value;rememberFeedback();};notes.row.hidden=draft.result!=='issues';
  const choices=[];
  for(const [value,label] of [['passed','이상 없음'],['issues','이상 있음']]){
    const row=el('label'),input=el('input');input.type='radio';input.name='test-feedback-result';input.value=value;input.checked=draft.result===value;input.setAttribute('aria-label',label);
    input.onchange=()=>{draft.result=value;for(const choice of choices)choice.checked=choice.value===value;notes.row.hidden=value!=='issues';rememberFeedback();};choices.push(input);row.append(input,el('span',label));options.append(row);
  }
  panel.append(fields);
  const message=el('p','','notice');message.id='test-feedback-message';message.setAttribute('role','status');panel.append(message);
  const submit=workflowButton('AI에게 전달',()=>sendTestFeedback('submit'));submit.id='test-feedback-submit';panel.append(submit);
  panel.append(workflowButton('최신 상태 불러오기',()=>refreshFeedbackContext()),workflowButton('닫기',()=>{feedbackSelected=null;panel.hidden=true;}));
  panel.append(el('p','이상 없음은 확인한 범위의 결과를 정리합니다. 이상 있음은 AI가 수정·재검증한 뒤 다시 확인할 항목을 알려줍니다.','notice'));
  renderFeedbackStatus();panel.scrollIntoView?.({block:'start',behavior:'smooth'});await refreshFeedbackContext();
}
async function refreshFeedbackContext(){
  if(!feedbackSelected)return;
  const taskPath=feedbackSelected.path;
  feedbackContext=null;renderFeedbackStatus();
  try{
    const context=await workflowRequest('/test-feedback',{action:'read',taskPath});
    if(feedbackSelected?.path!==taskPath)return;
    feedbackContext=context;feedbackAvailableProviders=context.providers||null;feedbackRecords[taskPath]=context;$('test-feedback-message').textContent='현재 작업 상태를 확인했습니다. 입력한 확인 범위와 결과를 전달하세요.';
    if(localStorage.getItem(feedbackKey(taskPath,'pending')))$('test-feedback-message').textContent='이전 전송의 응답을 확인하지 못했습니다. AI에게 전달을 다시 누르면 저장된 요청의 접수 여부를 확인합니다.';
    renderFeedbackStatus();watchTestFeedback();
  }catch(error){if(feedbackSelected?.path===taskPath)$('test-feedback-message').textContent=error.message;}
}
async function sendTestFeedback(action){
  if(feedbackSubmitting||!feedbackSelected)return;
  const item=feedbackSelected,draft=feedbackDraft(item.path),key=feedbackKey(item.path,'pending');
  const message=$('test-feedback-message');
  feedbackSubmitting=true;renderFeedbackStatus();
  try{
    let request=JSON.parse(localStorage.getItem(key)||'null');
    if(!request){
      if(action==='retry')await refreshFeedbackContext();
      if(!feedbackContext)throw Error('최신 상태를 먼저 불러오세요.');
      if(action==='submit'){
        if(!draft.result)throw Error('이상 없음 또는 이상 있음 중 하나를 선택하세요.');
        if(!draft.actor.trim())throw Error('확인자 이름을 입력하세요.');
        if(!draft.scope.trim())throw Error('확인한 항목을 입력하세요.');
        if(draft.result==='issues'&&!draft.notes.trim())throw Error('문제 상황과 재현 방법을 기록하세요.');
      }
      const provider=feedbackProviderChoice();
      if(!feedbackProviders().some(p=>p.id===provider))throw Error('선택한 AI가 연결되어 있지 않습니다. 다른 AI를 선택하거나 OpenWorkflow.bat을 다시 실행하세요.');
      request={action,provider,taskPath:item.path,operationId:requestId(),expectedRevision:feedbackContext.revision,taskHash:feedbackContext.taskHash,codeVersion:feedbackContext.codeVersion};
      if(action==='submit')Object.assign(request,{result:draft.result,actor:draft.actor,scope:draft.scope,notes:draft.result==='issues'?draft.notes:''});
      localStorage.setItem(key,JSON.stringify(request));
    }
    const record=await workflowRequest('/test-feedback',request);
    localStorage.removeItem(key);feedbackRecords[item.path]=record;
    message.textContent='AI가 테스트 결과를 접수했습니다. 이 화면에서 처리 상태를 확인할 수 있습니다.';
    feedbackContext=null;
    if(request.action==='submit'){
      draft.result='';localStorage.setItem(feedbackKey(item.path),JSON.stringify(draft));
      for(const input of $('test-feedback-fields').querySelectorAll('input[type="radio"]'))input.checked=false;
      for(const input of $('test-feedback-fields').querySelectorAll('textarea'))if(input.getAttribute('aria-label')==='문제 상황과 재현 방법')input.parentElement.hidden=true;
    }
    renderFeedbackStatus();await loadTestFeedback();
  }catch(error){
    if(error.responded)localStorage.removeItem(key);
    message.textContent=error.message+(error.responded?'':' 입력과 전송 요청은 보존됩니다. 다시 전달하면 중복 처리하지 않습니다.');
  }finally{feedbackSubmitting=false;renderFeedbackStatus();watchTestFeedback();}
}
