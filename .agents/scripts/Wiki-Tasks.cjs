// Copyright Woogle. All Rights Reserved.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const { taskName, basis } = require('./wiki-viewer/workflow-model.js');
const loop = () => ({ notes:'', decisions:[], result:null, reviewBasis:'', confirmation:null, archive:[] });
function folder(root) { return path.join(root, '.agents/in-progress'); }
function file(root, title) { if(taskName(title)!==title)throw new Error('작업 제목 앞뒤 공백을 제거하세요.');return path.join(folder(root), `workflow_${title}_task.json`); }
function checkReady(root) { if(fs.existsSync(path.join(folder(root),'workflow_rename_pending.json')))throw new Error('제목 변경 저장을 먼저 복구하세요.'); }
function readTask(root, title) {
  const target=file(root,title);
  if(!fs.existsSync(target))return null;
  const record=JSON.parse(fs.readFileSync(target,'utf8'));
  if(record.task?.taskId!==title||!Number.isSafeInteger(record.revision))throw new Error('작업 파일 형식 오류: '+title);
  return record;
}
// 확정 인계는 초안 저장 여부와 무관하게 공용 확정 기록을 기준으로 복원한다.
function reconcile(root, task) {
  const currentPath=path.join(folder(root),`workflow_${task.taskId}_current.json`);
  if(!fs.existsSync(currentPath)){
    task.serverRevision=0;
    for(const stage of ['planning','implementation'])if(task[stage]?.confirmation){task[stage].archive||=[];task[stage].archive.push(task[stage].confirmation);task[stage].confirmation=null;}
    return task;
  }
  const current=JSON.parse(fs.readFileSync(currentPath,'utf8'));
  const unchanged=task.serverRevision===current.revision;
  task.serverRevision=current.revision;
  if(current.change)task.changeTo=current.change.taskId;
  if(current.changeFrom)task.changeFrom=current.changeFrom;
  for(const stage of ['planning','implementation']){
    if(!current[stage])continue;
    if(unchanged&&task[stage]?.confirmation?.path===current[stage])continue;
    const relative=current[stage].replace(/\.md$/,'.json');
    const target=path.resolve(root,relative);
    if(path.dirname(target)!==path.resolve(folder(root)))throw new Error('인계 경로 오류');
    const snapshot=JSON.parse(fs.readFileSync(target,'utf8'));
    const oldConfirmation=task[stage]?.confirmation;
    const l=task[stage]={...loop(),notes:snapshot.notes||'',decisions:snapshot.decisions||[],result:snapshot.result};
    if(stage==='planning'){task.source=snapshot.source;task.sourceVersions=snapshot.sourceVersions||[];}
    const upstream=task.planning.confirmation;
    const source=stage==='planning'?task.source:JSON.stringify({source:upstream.draft,upstream:upstream.path,upstreamRevision:upstream.revision,upstreamBasis:upstream.basis});
    l.reviewBasis=basis(source,l.notes,l.decisions);
    // 작업별 단계 확정은 한 번만 가능하므로 기존 확정의 개정 번호는 보존한다.
    l.confirmation={basis:l.reviewBasis,draft:l.result.draft,path:current[stage],dataPath:relative,confirmedAt:snapshot.confirmedAt,revision:oldConfirmation?.revision||current.revision};
  }
  return task;
}
function listTasks(root) {
  checkReady(root);
  const names=new Set();
  for(const name of fs.existsSync(folder(root))?fs.readdirSync(folder(root)):[]){
    const match=name.match(/^workflow_(.+)_(task|current)\.json$/);
    if(match)names.add(match[1]);
  }
  const tasks=Object.create(null);
  for(const title of names){
    const saved=readTask(root,title);
    const task=saved?.task||{version:2,taskId:title,title,source:'',sourceVersions:[],serverRevision:0,planning:loop(),implementation:loop()};
    tasks[title]={revision:saved?.revision||0,updatedAt:saved?.updatedAt||null,task:reconcile(root,task)};
  }
  return {tasks};
}
function saveTask(root, body) {
  checkReady(root);
  const task=body?.task;
  const target=file(root,task?.taskId);
  if(task.title!==task.taskId||typeof task.source!=='string'||!task.planning||!task.implementation||!Array.isArray(task.planning.decisions)||!Array.isArray(task.implementation.decisions))throw new Error('작업 내용 형식 오류');
  if(!Number.isSafeInteger(body.expectedVersion)||body.expectedVersion<0||typeof body.operationId!=='string')throw new Error('작업 저장 개정 번호가 필요합니다.');
  const previous=readTask(root,task.taskId);
  const digest=crypto.createHash('sha256').update(JSON.stringify(body)).digest('hex');
  if(previous?.operationId===body.operationId){if(previous.digest!==digest)throw new Error('같은 저장 요청의 내용이 변경되었습니다.');return {revision:previous.revision,updatedAt:previous.updatedAt};}
  if((previous?.revision||0)!==body.expectedVersion)throw new Error('다른 사람이 작업을 변경해 저장하지 못했습니다. 작성한 입력은 유지됩니다.');
  if(!previous&&fs.existsSync(folder(root))&&fs.readdirSync(folder(root)).some(name=>name.toLowerCase()===path.basename(target).toLowerCase()))throw new Error('이미 사용 중인 작업 제목입니다.');
  const currentPath=path.join(folder(root),`workflow_${task.taskId}_current.json`);
  if(fs.existsSync(currentPath)&&JSON.parse(fs.readFileSync(currentPath,'utf8')).revision!==task.serverRevision)throw new Error('확정 기록이 변경되어 저장하지 못했습니다. 작성한 입력은 유지됩니다.');
  const record={revision:(previous?.revision||0)+1,updatedAt:new Date().toISOString(),operationId:body.operationId,digest,task};
  fs.mkdirSync(folder(root),{recursive:true});
  const temporary=target+'.'+crypto.randomUUID()+'.tmp';
  try{fs.writeFileSync(temporary,JSON.stringify(record,null,2)+'\n',{flag:'wx'});fs.renameSync(temporary,target);}finally{if(fs.existsSync(temporary))fs.unlinkSync(temporary);}
  return {revision:record.revision,updatedAt:record.updatedAt};
}
module.exports={listTasks,saveTask};
