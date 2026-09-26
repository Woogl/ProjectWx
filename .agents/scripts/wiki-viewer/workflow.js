// Copyright Woogle. All Rights Reserved.
const workflowKey = 'wx-wiki-workflow-v1:' + location.pathname;
const requestId = () => 'request-' + (typeof crypto !== 'undefined' ? crypto.randomUUID() : Date.now().toString(36)+'-'+Math.random().toString(36).slice(2));
function workflowButton(text, action) {
  const b = el('button', text, 'back'); b.type = 'button'; b.onclick = action; return b;
}
async function workflowRequest(endpoint, body) {
  if (!data.ai) throw new Error('OpenWorkflow.bat을 다시 실행하여 AI 연결을 시작하세요.');
  let response, result;
  try {
    response = await fetch(data.ai.url + endpoint, { method:'POST', headers:{'Content-Type':'application/json','X-Wx-Token':data.ai.token}, body:JSON.stringify(body) });
    result = await response.json();
  } catch { throw new Error('로컬 AI 서버에 연결하지 못했습니다. 입력은 그대로 있습니다. OpenWorkflow.bat을 다시 실행해 새로 열린 화면에서 이어가세요.'); }
  if (!response.ok) throw new Error(result.error||'요청에 실패했습니다.');
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
// Wiki 갱신: 고른 AI가 이 PC에서 Wiki/README.md 절차로 갱신한다. 상태는 서버에 있고 진행 중에만 다시 읽는다.
const wikiUpdate={loaded:false,status:'idle',provider:'',message:'',summary:'',error:''};
let wikiUpdatePoll=null,wikiUpdateStarting=false;
const wikiUpdateRunning=()=>['preparing','running'].includes(wikiUpdate.status);
async function loadWikiUpdate() {
  wikiUpdate.loaded=true;
  try{Object.assign(wikiUpdate,await workflowRequest('/wiki-update',{action:'status'}));}catch{}
  showWikiUpdate();
}
// 진행은 목록을 다시 그리지 않고 제목 아래 한 줄과 버튼만 고친다.
function showWikiUpdate() {
  const line=$('wiki-update-status'),button=$('wiki-update'),text=wikiUpdateText();
  if(line){if(line.textContent!==text)line.textContent=text;line.hidden=!text;}
  if(button){
    button.disabled=!data.ai||wikiUpdateStarting||wikiUpdateRunning();
    button.title=!data.ai?'OpenWorkflow.bat을 다시 실행해 AI 연결을 시작하세요.':button.disabled?'Wiki를 갱신하는 중입니다.':'왼쪽 메뉴 아래에서 고른 AI가 이 PC에서 Wiki를 갱신하고 main에 푸시합니다. WSL과 claude-obsidian이 없으면 설치를 시작합니다.';
  }
  if(wikiUpdateRunning()&&!wikiUpdatePoll&&typeof setTimeout!=='undefined')wikiUpdatePoll=setTimeout(()=>{wikiUpdatePoll=null;loadWikiUpdate();},3000);
}
function wikiUpdateText() {
  const who=providerLabel(wikiUpdate.provider||'');
  if(wikiUpdateStarting)return 'Wiki 갱신을 시작하는 중입니다.';
  if(wikiUpdateRunning())return `Wiki 갱신 중 · ${who} · ${wikiUpdate.message}`;
  if(wikiUpdate.status==='setup')return wikiUpdate.message;
  if(wikiUpdate.status==='complete')return `Wiki 갱신을 마쳤습니다(${who}). ${wikiUpdate.summary} main에 올라간 결과는 pull하면 Obsidian에서 볼 수 있습니다.`;
  if(wikiUpdate.status==='failed')return 'Wiki 갱신을 하지 못했습니다. '+wikiUpdate.error;
  return '';
}
// 버튼을 누르면 바로 시작하고, 진행과 결과는 작업 기록 제목 아래 한 줄로 보여준다. 응답을 기다리는 동안 다시 누를 수 없다.
async function startWikiUpdate() {
  if(wikiUpdateStarting||wikiUpdateRunning())return;
  const provider=selectedProvider();
  Object.assign(wikiUpdate,{summary:'',error:''});
  if(!availableProviders().some(p=>p.id===provider)){Object.assign(wikiUpdate,{status:'failed',provider,error:providerMissing});return showWikiUpdate();}
  wikiUpdateStarting=true;showWikiUpdate();
  try{Object.assign(wikiUpdate,await workflowRequest('/wiki-update',{action:'start',provider}));}
  catch(error){Object.assign(wikiUpdate,{status:'failed',error:error.message});}
  finally{wikiUpdateStarting=false;}
  showWikiUpdate();
}
function renderTaskRecords() {
  const panel=$('task-records');panel.replaceChildren();
  const groups=taskRecordGroups(),total=groups.reduce((n,g)=>n+g.items.length,0);
  const heading=el('div',undefined,'record-heading'),start=workflowButton('새 작업',()=>{location.hash='#work!new';});start.id='new-task-open';
  const update=workflowButton('Wiki 갱신',()=>startWikiUpdate());update.id='wiki-update';
  const progress=el('p','','notice');progress.id='wiki-update-status';progress.setAttribute('role','status');
  heading.append(el('h2','확인할 일과 작업 기록'),start,update);panel.append(heading,progress);showWikiUpdate();
  if(!total){panel.append(el('p','작업 기록이 없습니다. 새 작업으로 시작하세요.','notice'));return;}
  panel.append(el('p',total+'개 기록. 최근에 바뀐 기록부터 보여줍니다. 이어서 작업을 누르면 작업 탭에서 질문 답변·구현 승인·테스트 결과 전달을 하고, 새 일은 새 작업으로 시작하세요.','notice'));
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
  // 확인 대기·진행 중은 작업 탭에서 이어가며 AI 처리 상태를 붙이고, 완료·리뷰·참고는 기록을 읽기만 한다. 행마다 같은 버튼 이름에 작업 제목을 붙여 읽는다.
  const working=group.id==='waiting'||group.id==='active';
  for(const item of group.items){
    const link=el('div',undefined,'record-item');
    const summary=el('div');summary.append(el('strong',item.title),el('span',item.evidence||'','record-evidence'));
    const next=el('div',undefined,'record-next');next.append(el('span',group.id==='complete'?'참고할 때':'다음 행동','record-label'),el('span',item.next||'-'));
    const actions=el('div',undefined,'record-actions');
    if(working){const open=workflowButton('이어서 작업',()=>{location.hash=route('work',item.path);});open.setAttribute('aria-label','이어서 작업: '+item.title);actions.append(open);}
    else if(data.documents.some(d=>d.path===item.path)){const open=el('a','기록 열기 →');open.href=route(item.path);open.setAttribute('aria-label','기록 열기: '+item.title);actions.append(open);}
    else actions.append(el('span','새 기록 · OpenWorkflow.bat을 다시 실행하면 열립니다.','notice'));
    const latest=taskJobs[item.path]?.latest;if(working&&latest)actions.append(el('span',taskStatusText(latest.status),'notice'));
    link.append(summary,next,actions);list.append(link);
  }
  panel.append(list);
}
function renderWorkSummary() {
  renderTaskRecords();
  if(!taskJobsLoaded)loadTaskJobs();
  if(data.ai&&!wikiUpdate.loaded)loadWikiUpdate();
}
