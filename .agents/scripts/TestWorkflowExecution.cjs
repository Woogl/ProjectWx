// Copyright Woogle. All Rights Reserved.
const assert=require('node:assert/strict'),fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const {createExecutionService,readExecution,runExecution}=require('./Workflow-Execution.cjs');
const root=fs.mkdtempSync(path.join(os.tmpdir(),'wx-execution-'));
const report={summary:'완료 기준 구현',changes:['Source/Foo.cpp 변경'],checks:[{name:'회귀 검사',status:'passed',evidence:'node test.cjs: exit 0'}],humanChecks:['회피 후 조작감 확인'],blockers:[]};
const current={taskId:'작업',planning:'p.md',implementation:'d.md',pending:null};
let version='v1',finish,runs=0;
const service=()=>createExecutionService({root,resolveCurrent:()=>current,fingerprint:()=>version,diff:()=>'+ actual change',run:()=>{runs++;return new Promise(resolve=>{finish=resolve;});}});
let runner=service(),sequence=0;
const action=(type,extra={})=>runner.act({taskId:'작업',action:type,expectedRevision:readExecution(root,'작업')?.revision||0,operationId:'op-'+(++sequence),...extra});
const settle=async()=>{await new Promise(resolve=>setImmediate(resolve));};
(async()=>{try{
  current.implementation=null;assert.throws(()=>action('start'),/확정/);current.implementation='d.md';
  const request={taskId:'작업',action:'start',expectedRevision:0,operationId:'start'};
  assert.equal(runner.act(request).status,'running');await settle();
  assert.equal(runner.act(request).status,'running');assert.equal(runs,1,'response loss must not execute twice');
  assert.throws(()=>action('accept'),/진행 중/);
  finish(report);await settle();assert.equal(readExecution(root,'작업').status,'review');
  version='v2';assert.equal(action('approve').status,'blocked');assert.equal(readExecution(root,'작업').decisions.length,0);
  action('retry');await settle();finish(report);await settle();
  action('approve');await settle();assert.equal(readExecution(root,'작업').phase,'verify');
  finish(report);await settle();assert.equal(readExecution(root,'작업').status,'acceptance');
  assert.throws(()=>action('accept'),/확인할 항목/);
  action('accept',{confirmedChecks:[0]});assert.equal(readExecution(root,'작업').status,'complete');
  assert.equal(readExecution(root,'작업').decisions.length,2,'only humans approve review and acceptance');
  assert.throws(()=>action('retry'),/다시 실행/);

  current.taskId='중단';const start={taskId:'중단',action:'start',expectedRevision:0,operationId:'interrupted'};
  runner.act(start);await settle();runner=service();assert.equal(readExecution(root,'중단').status,'interrupted');
  // 이전 프로세스의 콜백은 재시작한 서버에 존재하지 않는 상황을 모사한다.
  runner.act({...start,action:'retry',expectedRevision:readExecution(root,'중단').revision,operationId:'resume'});await settle();
  finish({...report,checks:[{name:'게임 실행',status:'not_run',evidence:'에디터 실행 불가'}]});await settle();
  const decide=(type,extra={})=>runner.act({taskId:'중단',action:type,expectedRevision:readExecution(root,'중단').revision,operationId:'more-'+(++sequence),...extra});
  decide('approve');await settle();version='v3';finish(report);await settle();assert.equal(readExecution(root,'중단').status,'review','verification changes require code review again');
  decide('approve');await settle();finish({...report,checks:[]});await settle();assert.throws(()=>decide('accept',{confirmedChecks:[0]}),/미검증/);
  decide('accept',{confirmedChecks:[0],feedback:'실행 환경 한계를 알고 이 범위를 수용'});assert.equal(readExecution(root,'중단').status,'complete');
  current.taskId='수정 재시도';const feedbacks=[];
  runner=createExecutionService({root,resolveCurrent:()=>current,fingerprint:()=>version,diff:()=>'',run:async record=>{
    feedbacks.push(record.feedback);if(feedbacks.length===2)throw Error('일시적 실패');return report;
  }});
  const reviseAction=(action,feedback)=>runner.act({taskId:current.taskId,action,feedback,expectedRevision:readExecution(root,current.taskId)?.revision||0,operationId:'feedback-'+(++sequence)});
  reviseAction('start');await settle();reviseAction('revise','취소 시 비용 반환');await settle();
  assert.equal(readExecution(root,current.taskId).status,'blocked');reviseAction('retry');await settle();
  assert.deepEqual(feedbacks.slice(1),['취소 시 비용 반환','취소 시 비용 반환']);
  assert.equal(readExecution(root,current.taskId).decisions[0].feedback,'취소 시 비용 반환');
  const child=require('node:child_process').spawn(process.execPath,['-e','setInterval(()=>{},1000)'],{windowsHide:true,stdio:'ignore'});
  await new Promise((resolve,reject)=>{child.once('spawn',resolve);child.once('error',reject);});
  const stopped=new Promise(resolve=>child.once('exit',resolve));
  try{
    const file=path.join(root,'.agents/workflow/tasks/workflow_남은 실행_execution.json');
    fs.writeFileSync(file,JSON.stringify({taskId:'남은 실행',status:'running',revision:1,workerPid:child.pid}));
    runner=service();assert.equal(runner.isBusy(),true,'surviving worker blocks the entire repository');
    current.taskId='새 작업';assert.throws(()=>runner.act({taskId:current.taskId,action:'start',expectedRevision:0,operationId:'other-task'}),/진행 중/);
  }finally{child.kill();await stopped;}
  assert.equal(runner.isBusy(),false,'execution unlocks after the old worker exits');
  runner.act({taskId:current.taskId,action:'start',expectedRevision:0,operationId:'after-exit'});await settle();finish(report);await settle();
  let received;
  await runExecution({root,command:{file:'codex.exe'},record:{phase:'implement'},execute:(_file,args,options,done)=>{
    assert.equal(args[args.indexOf('--sandbox')+1],'workspace-write');assert(!args.some(a=>a.includes('bypass')));assert.equal(options.windowsHide,true);
    return {stdin:{on(){},end(prompt){received=prompt;fs.writeFileSync(args[args.indexOf('--output-last-message')+1],JSON.stringify(report));done(null);}}};
  }});assert.match(received,/사람은 판단/);
  console.log('PASS durable execution, replay, restart, code-bound approvals, verification changes, explicit acceptance and sandboxed worker');
}finally{assert.equal(path.dirname(root),path.resolve(os.tmpdir()));assert(path.basename(root).startsWith('wx-execution-'));fs.rmSync(root,{recursive:true,force:true});}})().catch(error=>{console.error(error);process.exitCode=1;});
