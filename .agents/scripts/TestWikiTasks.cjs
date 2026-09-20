// Copyright Woogle. All Rights Reserved.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),vm=require('node:vm'),assert=require('node:assert/strict');
const {listTasks,saveTask}=require('./Wiki-Tasks.cjs');
const {saveHandoff,renameTask}=require('./Wiki-AI.cjs');
const root=fs.mkdtempSync(path.join(os.tmpdir(),'wx-tasks-'));
const scripts=['workflow-model.js','workflow.js','execution.js'].map(f=>fs.readFileSync(path.join(__dirname,'wiki-viewer',f),'utf8')).join('\n');
const panel={};
async function browser(storage=new Map()){
  const c=vm.createContext({location:{pathname:'/workflow'},data:{ai:{url:'http://127.0.0.1/analyze',token:'test'}},localStorage:{getItem:k=>storage.get(k)||null,setItem:(k,v)=>storage.set(k,v),removeItem:k=>storage.delete(k)},$:()=>panel});
  c.fetch=async(url,options)=>{try{const b=JSON.parse(options.body);const receipt=url.endsWith('/tasks')?listTasks(root):url.endsWith('/task')?saveTask(root,b):url.endsWith('/handoff')?saveHandoff(b,root):renameTask(b,root);return {ok:true,json:async()=>receipt};}catch(e){return {ok:false,json:async()=>({error:e.message,current:e.current})};}};
  vm.runInContext(scripts,c);await vm.runInContext('initializeSharedTasks()',c);return c;
}
const run=(c,s)=>vm.runInContext(s,c);
(async()=>{try{
  const a=await browser();run(a,"workflowState=newTask('공유 기획');workflowState.source='원본';saveWorkflow()");await run(a,'flushSharedTasks()');
  const b=await browser();assert.equal(run(b,"otherTasks['공유 기획'].source"),'원본');
  run(b,"workflowState=otherTasks['공유 기획'];workflowState.source='다른 사람의 수정';saveWorkflow()");await run(b,'flushSharedTasks()');
  run(a,"workflowState.source='오래 열린 화면의 입력';saveWorkflow()");await assert.rejects(run(a,'flushSharedTasks()'));
  assert.equal(listTasks(root).tasks['공유 기획'].task.source,'다른 사람의 수정');
  assert.equal(run(a,"draftRecovery['공유 기획'].task.source"),'오래 열린 화면의 입력');
  // 응답 유실 시 같은 저장 요청을 재전송해도 버전은 한 번만 증가한다.
  const rec=listTasks(root).tasks['공유 기획'];const body={task:rec.task,expectedVersion:rec.revision,operationId:'retry'};
  assert.deepEqual(saveTask(root,body),saveTask(root,body));
  assert.throws(()=>saveTask(root,{...body,task:{...body.task,taskId:'../escape'}}));
  const key='wx-wiki-workflow-v1:/workflow';
  const legacy=JSON.parse(run(b,'JSON.stringify(workflowState)'));legacy.taskId=legacy.title='브라우저 작업';legacy.serverRevision=0;
  const local=new Map([[key+':tasks',JSON.stringify({[legacy.taskId]:legacy})]]);
  await browser(local);assert(!listTasks(root).tasks['브라우저 작업'],'legacy local lists must not create shared tasks');assert(local.has(key+':tasks'),'unread legacy data remains untouched');
  saveTask(root,{task:legacy,expectedVersion:0,operationId:'create-from-file'});
  const n=Object.keys(listTasks(root).tasks).length;await browser(local);assert.equal(Object.keys(listTasks(root).tasks).length,n);
  const c=await browser();run(c,"workflowState=otherTasks['브라우저 작업']");
  await run(c,"commitWorkflow('/rename',{taskId:workflowState.taskId,title:'새 제목',expectedRevision:workflowState.serverRevision},WxWorkflowModel.remapTask(workflowState,workflowState.taskId,'새 제목'))");
  assert(!listTasks(root).tasks['브라우저 작업']);assert(listTasks(root).tasks['새 제목']);
  const d=await browser();assert(run(d,"Object.hasOwn(otherTasks,'새 제목')"));
  run(d,"workflowState=otherTasks['새 제목'];workflowState.progress={stage:'설계',note:'인터페이스 검토 중'};saveWorkflow()");await run(d,'flushSharedTasks()');
  const observer=await browser();assert.equal(run(observer,"otherTasks['새 제목'].progress.note"),'인터페이스 검토 중');
  run(d,"workflowState.planning.result={summary:'검토',draft:'확정 기획',facts:[],evidence:[],blockers:[],items:[],questions:[]};workflowState.planning.reviewBasis=loopBasis('planning')");
  await run(d,"commitWorkflow('/handoff',{taskId:workflowState.taskId,expectedRevision:workflowState.serverRevision,stage:'planning',source:workflowState.source,notes:'',decisions:[],result:workflowState.planning.result,confirmedAt:'2026-09-21T00:00:00Z'},workflowState,loopBasis('planning'))");
  const confirmed=await browser();run(confirmed,"workflowState=otherTasks['새 제목']");assert.equal(run(confirmed,"loopConfirmed('planning')"),true);
  const duplicate=new Map([[key+':tasks',JSON.stringify({'새 제목':{...legacy,taskId:'새 제목',title:'새 제목',source:'다른 브라우저의 미확정 원본'}})]]);
  await browser(duplicate);assert.equal(listTasks(root).tasks['새 제목'].task.planning.result.draft,'확정 기획');
  assert(!Object.values(listTasks(root).tasks).some(r=>r.task.source==='다른 브라우저의 미확정 원본'));
  fs.unlinkSync(path.join(root,'.agents/in-progress/workflow_공유 기획_task.json'));
  const e=await browser();assert(!run(e,"Object.hasOwn(otherTasks,'공유 기획')"));
  console.log('PASS shared files across browsers, stale-write rejection, input preservation, idempotent save, legacy isolation, rename and file deletion');
}finally{assert.equal(path.dirname(path.resolve(root)),path.resolve(os.tmpdir()));assert(path.basename(root).startsWith('wx-tasks-'));fs.rmSync(root,{recursive:true,force:true});}})().catch(e=>{console.error(e);process.exitCode=1;});
