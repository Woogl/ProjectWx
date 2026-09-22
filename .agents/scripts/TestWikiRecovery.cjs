// Copyright Woogle. All Rights Reserved.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),vm=require('node:vm'),assert=require('node:assert/strict');
const {listTasks,saveTask}=require('./Wiki-Tasks.cjs');
const {saveHandoff,revokeHandoff,startChange,renameTask,readCurrent}=require('./Wiki-AI.cjs');
const scripts=['workflow-model.js','workflow.js','execution.js'].map(file=>fs.readFileSync(path.join(__dirname,'wiki-viewer',file),'utf8')).join('\n');
const fixture=fs.mkdtempSync(path.join(os.tmpdir(),'wx-recovery-'));
const storage=new Map(),panel={};let loseResponse=false,failAfterCommit=false,requests=0;
const localStorage={getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)};
const result={summary:'검토',draft:'회피 비용 20',facts:[],evidence:[],blockers:[],items:[],questions:[]};
async function reload(){
  const context=vm.createContext({location:{pathname:'recovery'},localStorage,data:{ai:{url:'http://127.0.0.1/analyze',token:'test'}},$:()=>panel});
  context.fetch=async(url,options)=>{
    requests++;
    const body=JSON.parse(options.body);
    const receipt=url.endsWith('/tasks')?listTasks(fixture):url.endsWith('/task')?saveTask(fixture,body):url.endsWith('/handoff')?saveHandoff(body,fixture):url.endsWith('/change')?startChange(body,fixture):url.endsWith('/rename')?renameTask(body,fixture):revokeHandoff(body,fixture);
    if(failAfterCommit&&url.endsWith('/handoff')){failAfterCommit=false;localStorage.setItem=()=>{throw new Error('quota after commit');};}
    if(loseResponse&&!url.endsWith('/tasks')&&!url.endsWith('/task')){loseResponse=false;throw new Error('response lost after commit');}
    return {ok:true,json:async()=>receipt};
  };
  vm.runInContext(scripts,context);await vm.runInContext('initializeSharedTasks()',context);return context;
}
const run=(context,code)=>vm.runInContext(code,context);
function prepare(context){
  context.result=result;
  run(context,"if(!workflowState.taskId)workflowState=newTask('회피 테스트');workflowState.source='원본';workflowState.planning.result=result;workflowState.planning.reviewBasis=loopBasis('planning');saveWorkflow()");
}
function confirm(context){
  return run(context,`commitWorkflow('/handoff',{taskId:workflowState.taskId,expectedRevision:workflowState.serverRevision,stage:'planning',source:workflowState.source,notes:workflowState.planning.notes,decisions:workflowState.planning.decisions,result:workflowState.planning.result,confirmedAt:'2026-09-20T00:00:00Z'},workflowState,loopBasis('planning'))`);
}
(async()=>{try{
  let c=await reload();prepare(c);loseResponse=true;
  await assert.rejects(confirm(c),/response lost/);
  const journal=run(c,'JSON.stringify(pendingSave.body)');
  assert.equal(run(c,'workflowState.serverRevision'),0);
  const files=fs.readdirSync(path.join(fixture,'.agents/workflow/tasks')).sort();
  c=await reload();assert.equal(run(c,'JSON.stringify(pendingSave.body)'),journal);
  await run(c,'syncSharedTasks()');
  assert.equal(run(c,"loopConfirmed('planning')"),true);
  assert.deepEqual(fs.readdirSync(path.join(fixture,'.agents/workflow/tasks')).sort(),files);
  assert.equal(run(c,'workflowState.serverRevision'),1);
  c.result=result;run(c,"workflowState.implementation.result=result;workflowState.implementation.reviewBasis=loopBasis('implementation');saveWorkflow()");
  loseResponse=true;await assert.rejects(run(c,"beginChange('planning',undefined,'비용 변경')"),/response lost/);
  c=await reload();await run(c,'finishPendingSave()');
  assert.equal(run(c,'workflowState.planning.confirmation'),null);
  assert.equal(run(c,'workflowState.serverRevision'),0);
  assert.equal(run(c,'pendingSave'),null);
  assert.equal(panel.inert,false);
  console.log('PASS committed handoff/change response loss, reload, exact replay and no duplicate snapshots');

  prepare(c);failAfterCommit=true;await assert.rejects(confirm(c),/저장하지 못했습니다/);
  localStorage.setItem=(k,v)=>storage.set(k,v);
  c=await reload();await run(c,'finishPendingSave()');
  assert.equal(run(c,"loopConfirmed('planning')"),true);
  assert.equal(run(c,"loopReady('implementation')"),false,'same handoff path and draft with new planning revision requires design review');
  const count=requests;localStorage.setItem=()=>{throw new Error('quota before request');};
  await assert.rejects(run(c,"beginChange('planning',undefined,'비용 변경')"),/저장하지 못했습니다/);
  assert.equal(requests,count,'no mutation without durable retry journal');
  localStorage.setItem=(k,v)=>storage.set(k,v);
  console.log('PASS quota before request and after server commit preserve recoverable state');

  run(c,`workflowState.implementation.decisions=['Q1','Q2','D1'].map((id,index)=>({id,kind:index<2?'planning':'design',title:id,scope:id,requirement:'기획',options:[],recommendation:'',impact:'',reopenReason:'',answer:index===0?'확정 답':'검토 중 답',confirmed:index===0,history:[]}));saveWorkflow()`);
  loseResponse=true;await assert.rejects(run(c,"transferPlanningQuestions('Q1')"),/response lost/);
  c=await reload();await run(c,'finishPendingSave()');
  assert.equal(run(c,'workflowState.planning.decisions.length'),2);
  assert.equal(run(c,'workflowState.planning.decisions[0].answer'),'확정 답');
  assert.equal(run(c,'workflowState.planning.decisions[1].confirmed'),false);
  assert.equal(run(c,'workflowState.planning.decisions[1].answer'),'검토 중 답');
  assert.equal(run(c,'workflowState.implementation.decisions[1].status'),'transferred');
  assert.equal(run(c,'workflowState.implementation.decisions[2].status'),undefined);
  assert.equal(run(c,'workflowState.planning.confirmation'),null);
  const state=JSON.parse(run(c,'JSON.stringify(workflowState)'));
  assert.equal(readCurrent({taskId:state.taskId,expectedRevision:state.serverRevision},fixture).planning,null);
  console.log('PASS all planning questions transfer together, preserve answers/status, and recover after response loss');
  run(c,'workflowState.planning.decisions.forEach(q=>q.confirmed=true)');prepare(c);await confirm(c);
  const previous=run(c,'workflowState.taskId');
  const priorPath=run(c,'workflowState.planning.confirmation.path');
  await run(c,"beginChange('planning',undefined,'추가 변경')");
  assert.notEqual(run(c,'workflowState.taskId'),previous);
  assert.equal(fs.existsSync(path.join(fixture,priorPath)),true);
  assert.equal(run(c,`otherTasks[${JSON.stringify(previous)}].planning.confirmation.path`),priorPath);
  assert.equal(run(c,'pendingSave'),null);
  const oldTitle=run(c,'workflowState.taskId');
  loseResponse=true;
  await assert.rejects(run(c,`commitWorkflow('/rename',{taskId:workflowState.taskId,title:'디버깅용 변경 작업',expectedRevision:workflowState.serverRevision},WxWorkflowModel.remapTask(workflowState,workflowState.taskId,'디버깅용 변경 작업'))`),/response lost/);
  c=await reload();await run(c,'finishPendingSave()');
  assert.equal(run(c,'workflowState.taskId'),'디버깅용 변경 작업');
  assert.equal(run(c,`otherTasks[${JSON.stringify(previous)}].changeTo`),'디버깅용 변경 작업');
  assert.equal(run(c,`Object.hasOwn(otherTasks,${JSON.stringify(oldTitle)})`),false);
  console.log('PASS change creation preserves server snapshots and browser parent state');

}finally{
  const resolved=path.resolve(fixture);assert.equal(path.dirname(resolved),path.resolve(os.tmpdir()));assert(path.basename(resolved).startsWith('wx-recovery-'));
  fs.rmSync(resolved,{recursive:true,force:true});
}})().catch(error=>{console.error(error);process.exitCode=1;});
