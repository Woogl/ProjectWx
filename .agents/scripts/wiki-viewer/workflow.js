// Copyright Woogle. All Rights Reserved.
const workflowKey = 'wx-wiki-workflow-v1:' + location.pathname;
const requestId = () => 'request-' + (typeof crypto !== 'undefined' ? crypto.randomUUID() : Date.now().toString(36)+'-'+Math.random().toString(36).slice(2));
function workflowButton(text, action) {
  const b = el('button', text, 'back'); b.type = 'button'; b.onclick = action; return b;
}
async function workflowRequest(endpoint, body) {
  if (!data.ai) throw new Error('OpenWorkflow.bat을 다시 실행하여 AI 연결을 시작하세요.');
  const response = await fetch(data.ai.url + endpoint, { method:'POST', headers:{'Content-Type':'application/json','X-Wx-Token':data.ai.token}, body:JSON.stringify(body) });
  const result = await response.json();
  if (!response.ok) {const error=new Error(result.error||'요청에 실패했습니다.');error.responded=true;throw error;}
  return result;
}
// 작업 현황은 기록 파일의 상태 줄에서 만든다. 서버가 연결되면 최신 목록으로 바꾼다.
let taskRecordFilter='waiting',liveTaskRecords=null;
function taskRecordGroups() {
  const groups=[['waiting','확인 대기'],['active','진행 중'],['complete','완료'],['reference','리뷰·참고']].map(([id,title])=>({id,title,items:[]}));
  const records=liveTaskRecords||(data.documents||[]).filter(d=>/^\.agents\/workflow\/tasks\/[^/]+\.md$/.test(d.path))
    .map(d=>WxTaskRecords.readTaskRecord(d.path,d.text,d.modified||'')).sort((a,b)=>b.modified.localeCompare(a.modified));
  for(const record of records)(groups.find(g=>g.title===record.state)||groups[3]).items.push({title:record.title,path:record.path,evidence:record.summary,next:record.next});
  return groups;
}
// Wiki 갱신: 정기 갱신 Routine을 지금 실행한다. 토큰이 없으면 꺼 두고, 요청 중에는 다시 누를 수 없다.
const wikiUpdate={loaded:false,configured:false,sending:false,message:'',url:''};
async function loadWikiUpdate() {
  wikiUpdate.loaded=true;
  try{wikiUpdate.configured=!!(await workflowRequest('/wiki-update',{action:'status'})).configured;}catch{wikiUpdate.configured=false;}
  renderTaskRecords();
}
async function fireWikiUpdate() {
  if(wikiUpdate.sending)return;
  wikiUpdate.sending=true;wikiUpdate.message='Wiki 갱신을 요청하는 중입니다.';wikiUpdate.url='';renderTaskRecords();
  try{const result=await workflowRequest('/wiki-update',{action:'fire'});wikiUpdate.message='Wiki 갱신을 시작했습니다. 결과가 main에 푸시된 뒤 pull하면 Obsidian에서 볼 수 있습니다.';wikiUpdate.url=result.sessionUrl||'';}
  catch(error){wikiUpdate.message=error.message;}
  finally{wikiUpdate.sending=false;renderTaskRecords();}
}
function renderTaskRecords() {
  const panel=$('task-records');panel.replaceChildren();
  const groups=taskRecordGroups(),total=groups.reduce((n,g)=>n+g.items.length,0);
  const heading=el('div',undefined,'record-heading'),start=workflowButton('새 작업',()=>openNewTask());start.id='new-task-open';
  const update=workflowButton('Wiki 갱신',()=>fireWikiUpdate());update.id='wiki-update';update.disabled=!wikiUpdate.configured||wikiUpdate.sending;
  if(!wikiUpdate.configured)update.title=data.ai?'Saved/Wiki/wiki-routine.json에 Routine의 trigger와 token을 넣으면 켜집니다.':'OpenWorkflow.bat을 다시 실행해 AI 연결을 시작하세요.';
  heading.append(el('h2','확인할 일과 작업 기록'),start,update);panel.append(heading);
  if(wikiUpdate.message){const note=el('p',wikiUpdate.message,'notice');if(wikiUpdate.url){const link=el('a','Routine 세션 열기');link.href=wikiUpdate.url;link.target='_blank';link.rel='noopener noreferrer';note.append(' ',link);}panel.append(note);}
  if(!total){panel.append(el('p','작업 기록이 없습니다. 새 작업으로 시작하세요.','notice'));return;}
  panel.append(el('p',total+'개 기록. 최근에 바뀐 기록부터 보여줍니다. 작업 진행에서 질문 답변·구현 승인·테스트 결과 전달을 하고, 새 일은 새 작업으로 시작하세요.','notice'));
  const filters=el('div',undefined,'record-filters');filters.setAttribute('role','group');filters.setAttribute('aria-label','작업 상태');
  for(const group of groups){
    const button=workflowButton(group.title+' · '+group.items.length,()=>{taskRecordFilter=group.id;renderTaskRecords();$('task-record-list').focus();});
    button.setAttribute('aria-pressed',String(taskRecordFilter===group.id));button.setAttribute('aria-controls','task-record-list');filters.append(button);
  }
  panel.append(filters);
  const group=groups.find(g=>g.id===taskRecordFilter)||groups[0],list=el('section',undefined,'record-list');
  list.id='task-record-list';list.setAttribute('tabindex','-1');list.setAttribute('aria-label',group.title);
  list.append(el('h3',group.title+' · '+group.items.length));
  if(group.id==='complete')list.append(el('p','기록 당시 완료·사용자 확인 범위입니다. 남은 제약은 상세 기록에 보존되어 있습니다. 새 문제는 새 작업으로 요청하세요.','notice'));
  if(group.id==='reference')list.append(el('p','상태 줄이 없는 기록입니다. 모듈 리뷰의 지적을 고치려면 새 작업으로 요청하세요.','notice'));
  if(!group.items.length)list.append(el('p','이 상태의 작업이 없습니다.','notice'));
  // 확인 대기·진행 중은 작업 진행에서 이어가며 AI 처리 상태를 붙이고, 완료·리뷰·참고는 기록을 읽기만 한다.
  const working=group.id==='waiting'||group.id==='active';
  for(const item of group.items){
    const link=el('div',undefined,'record-item');
    const summary=el('div');summary.append(el('strong',item.title),el('span',item.evidence||'','record-evidence'));
    const next=el('div',undefined,'record-next');next.append(el('span',group.id==='complete'?'참고할 때':'다음 행동','record-label'),el('span',item.next||'-'));
    const actions=el('div',undefined,'record-actions');
    if(working)actions.append(workflowButton('작업 진행',()=>openTaskPanel(item)));
    else if(data.documents.some(d=>d.path===item.path)){const open=el('a','기록 열기 →');open.href=route(item.path);actions.append(open);}
    else actions.append(el('span','새 기록 · OpenWorkflow.bat을 다시 실행하면 열립니다.','notice'));
    const latest=taskJobs[item.path]?.latest;if(working&&latest)actions.append(el('span',taskStatusText(latest.status),'notice'));
    link.append(summary,next,actions);list.append(link);
  }
  panel.append(list);
}
function renderWorkSummary() {
  renderTaskRecords();
  $('test-feedback-panel').hidden=!taskSelected&&!newTaskOpen;
  if(!taskJobsLoaded)loadTaskJobs();
  if(data.ai&&!wikiUpdate.loaded)loadWikiUpdate();
}
