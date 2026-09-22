// Copyright Woogle. All Rights Reserved.
const assert=require('node:assert/strict'),fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const {createServer,validateResult,validateContext,buildPrompt,saveHandoff,revokeHandoff,readCurrent,startChange,renameTask,resolveCurrent,analyze}=require('./Wiki-AI.cjs');
const {mergeDecisions}=require('./wiki-viewer/workflow-model.js');
const sample={summary:'회피',draft:'회피 비용 20',facts:['비용 20'],evidence:[],blockers:[],items:[],questions:[]};
const question={id:'Q1',title:'취소 정책',requirement:'기획 미정',scope:'취소 시 반환?',options:['반환','유지'],recommendation:'유지',impact:'비용 정책',reopenReason:''};
(async()=>{
  assert.throws(()=>validateResult({items:[]}));
  assert.deepEqual(validateResult(sample),sample,'zero questions/tasks is a valid converged planning review');
  assert.throws(()=>validateResult({...sample,questions:[question,question]}));
  const decisions=[{...question,answer:'유지',confirmed:true,history:[]}];
  assert.deepEqual(mergeDecisions(decisions,[]),decisions,'omitted decisions must survive');
  const excluded=mergeDecisions(decisions,[],[{id:'Q1',reason:'취소 기능 제외',basis:'인간의 범위 제외 결정'}]);
  assert.equal(excluded[0].status,'excluded');assert.equal(excluded[0].answer,'유지');
  assert.throws(()=>mergeDecisions(decisions,[],[{id:'missing',reason:'제외',basis:'근거'}]));
  assert.throws(()=>mergeDecisions(decisions,[],[{id:'Q1',reason:'제외',basis:''}]));
  assert.equal(mergeDecisions(excluded,[{...question,reopenReason:'취소 기능 재포함'}])[0].status,'active');
  assert.throws(()=>mergeDecisions(decisions,[{...question,scope:'다른 질문'}]));
  const reopened=mergeDecisions(decisions,[{...question,reopenReason:'기획 변경'}]);
  assert.equal(reopened[0].confirmed,false);assert.equal(reopened[0].answer,'유지');assert.equal(reopened[0].history.length,1);
  assert.equal(decisions[0].confirmed,true,'merge is transactional');
  assert.throws(()=>validateContext({decisions:[...decisions,...decisions]}));
  const prompt=buildPrompt('회피','implementation',{notes:'의견',decisions,previousDraft:'초안'});
  assert(prompt.includes('AGENTS.md'));assert(prompt.includes('유지'));assert(prompt.includes('추가 판단만')||prompt.includes('반복 질문하지'));
  let calls=0,lastContext,lastMode,lastProvider;
  const server=createServer({token:'test-token',port:18744,providers:['codex','claude','gemini'].map(id=>({id,label:id})),runAnalysis:async(plan,mode,context,provider)=>{calls++;lastProvider=provider;lastContext=context;lastMode=mode;assert.equal(plan,'회피');return sample;},writeHandoff:()=>({path:'test.md'})});
  await new Promise(resolve=>server.listen(18744,'127.0.0.1',resolve));
  const send=(body={plan:'회피'},headers={},endpoint='/analyze')=>fetch('http://127.0.0.1:18744'+endpoint,{method:'POST',headers:{Origin:'null','Content-Type':'application/json','X-Wx-Token':'test-token',...headers},body:JSON.stringify(body)});
  try{
    assert.equal((await send(undefined,{Origin:'https://example.com'})).status,403);
    assert.equal((await send(undefined,{'X-Wx-Token':'invalid'})).status,403);
    assert.equal((await send({plan:''})).status,400);
    assert.equal((await send({plan:'회피',context:{decisions:'invalid'}})).status,400);
    assert.equal(calls,0);
    assert.equal((await send()).status,200);assert.equal(lastMode,'implementation');assert.equal(lastProvider,'codex');
    for(const provider of ['claude','gemini']){assert.equal((await send({plan:'회피',provider})).status,200);assert.equal(lastProvider,provider);}
    const before=calls;assert.equal((await send({plan:'회피',provider:'unknown'})).status,400);assert.equal(calls,before);
    const response=await send({plan:'회피',mode:'planning',context:{decisions,notes:'보존'}});
    assert.deepEqual(await response.json(),sample);assert.equal(lastMode,'planning');assert.equal(lastContext.decisions[0].answer,'유지');
    assert.equal((await send({plan:'회피',mode:'bad'})).status,400);
    assert.equal((await send({}, {'X-Wx-Token':'invalid'},'/handoff')).status,403);
    assert.equal((await send({}, {'X-Wx-Token':'invalid'},'/revoke')).status,403);
    assert.equal((await send({}, {'X-Wx-Token':'invalid'},'/execution')).status,403);
    assert.equal((await send({}, {Origin:'https://example.com'},'/execution')).status,403);
    assert.equal((await send({}, {},'/execution')).status,400,'execution without a configured worker cannot silently succeed');
    assert.equal((await send({}, {},'/revoke')).status,400);
    assert.equal((await send({}, {},'/handoff')).status,200);
    const health=await(await fetch('http://127.0.0.1:18744/health')).json();assert.equal(health.protocol,3);assert.equal(health.revision.split(':').length,11);
  }finally{await new Promise(resolve=>server.close(resolve));}
  const fixture=fs.mkdtempSync(path.join(os.tmpdir(),'wx-workflow-'));
  try{
    const body={operationId:'first-save',title:'회피 시스템 개선',taskId:'회피 시스템 개선',expectedRevision:0,stage:'planning',source:'원본',notes:'의견',decisions,result:sample,confirmedAt:new Date().toISOString()};
    const record=saveHandoff(body,fixture);
    assert.ok(fs.readFileSync(path.join(fixture,record.path),'utf8').startsWith('# 회피 시스템 개선 · 기획 확정 인계'));
    assert.ok(fs.readFileSync(path.join(fixture,'.agents/workflow/tasks/index.md'),'utf8').includes('[회피 시스템 개선 · 기획 확정 인계]'));
    assert.equal(readCurrent({...body,expectedRevision:record.revision},fixture).title,body.title);
    assert.deepEqual(saveHandoff(body,fixture),record,'lost response retry returns original receipt');
    assert.equal(fs.readdirSync(path.join(fixture,'.agents/workflow/tasks')).filter(f=>f.endsWith('.md')).length,2,'retry creates no duplicate handoff');
    assert.throws(()=>saveHandoff({...body,source:'changed'},fixture),/같은 요청 식별자/);
    assert(fs.readFileSync(path.join(fixture,record.path),'utf8').includes('확정 답변: 유지'));
    assert.deepEqual(JSON.parse(fs.readFileSync(path.join(fixture,record.dataPath),'utf8')).decisions,decisions);
    assert(fs.readFileSync(path.join(fixture,'.agents/workflow/tasks/index.md'),'utf8').includes(encodeURIComponent(path.basename(record.path))));
    assert.throws(()=>saveHandoff({...body,operationId:undefined,expectedRevision:record.revision,result:{...sample,blockers:['미검증']}},fixture),/미확정 판단 또는 미해결 자료/);
    assert.throws(()=>saveHandoff({...body,operationId:undefined,expectedRevision:record.revision,decisions:[{...decisions[0],confirmed:false}]},fixture),/미확정 판단 또는 미해결 자료/);
    assert.throws(()=>saveHandoff({...body,operationId:undefined,expectedRevision:record.revision,stage:'implementation'},fixture),/확정 기획 인계 자료/);
    assert.throws(()=>saveHandoff({...body,operationId:undefined,expectedRevision:record.revision,stage:'implementation',source:'낡은 기획',upstream:{path:record.path,draft:'낡은 기획'}},fixture),/기획이 개정되었습니다/);
    const second=saveHandoff({...body,operationId:undefined,expectedRevision:record.revision,stage:'implementation',source:sample.draft,upstream:{path:record.path,draft:sample.draft}},fixture);
    assert.equal(readCurrent({...body,operationId:undefined,expectedRevision:second.revision},fixture).implementation,second.path);
    const original=fs.readFileSync(path.join(fixture,record.dataPath),'utf8');
    assert.throws(()=>saveHandoff({...body,operationId:'overwrite',expectedRevision:second.revision},fixture),/이미 확정/);
    assert.equal(revokeHandoff({...body,operationId:'legacy',expectedRevision:second.revision},fixture).preserved,true);
    assert.equal(fs.readFileSync(path.join(fixture,record.dataPath),'utf8'),original);
    const change={taskId:body.taskId,newTaskId:'change-one',operationId:'change-start',expectedRevision:second.revision,stage:'planning',reason:'비용 변경',scope:'회피 비용 및 회귀 검사'};
    const rename=fs.renameSync;
    try {
      fs.renameSync=(from,to)=>{if(to.endsWith('workflow_회피 시스템 개선_current.json'))throw new Error('simulated disk failure');return rename(from,to);};
      assert.throws(()=>startChange(change,fixture),/disk failure/);
    }finally{fs.renameSync=rename;}
    assert.equal(resolveCurrent(body.taskId,fixture).implementation,second.path,'partial change creation does not replace current criteria');
    const started=startChange(change,fixture);
    assert.deepEqual(startChange(change,fixture),started,'lost response reuses one child');
    assert.equal(resolveCurrent(body.taskId,fixture).pending.scope,change.scope);
    assert.equal(resolveCurrent('change-one',fixture).planning,record.path);
    assert.equal(resolveCurrent(body.taskId,fixture).implementation,second.path,'review retains old criteria outside affected scope');
    assert.throws(()=>startChange({...change,operationId:'second-child',expectedRevision:started.revision,newTaskId:'other'},fixture),/최신 작업/);
    assert.throws(()=>saveHandoff({...body,operationId:'stale-parent',expectedRevision:started.revision},fixture),/후속 변경/);
    const newBody={...body,taskId:'change-one',operationId:'new-plan',expectedRevision:0,result:{...sample,draft:'회피 비용 30'}};
    assert.throws(()=>saveHandoff({...newBody,decisions:[],result:{...newBody.result,questions:[question]}},fixture),/AI 질문과/);
    const newPlan=saveHandoff(newBody,fixture);
    assert.equal(resolveCurrent(body.taskId,fixture).planning,newPlan.path);
    assert.equal(resolveCurrent(body.taskId,fixture).implementation,null,'new planning cannot use old design');
    const newDesign=saveHandoff({...newBody,operationId:'new-design',expectedRevision:newPlan.revision,stage:'implementation',source:newBody.result.draft,upstream:{path:newPlan.path,draft:newBody.result.draft}},fixture);
    assert.equal(resolveCurrent(body.taskId,fixture).implementation,newDesign.path);
    assert.equal(resolveCurrent(body.taskId,fixture).pending,null);
    assert.equal(fs.readFileSync(path.join(fixture,record.dataPath),'utf8'),original,'old decisions remain byte-identical');
    const designChange=startChange({...change,taskId:'change-one',newTaskId:'change-two',operationId:'design-change',expectedRevision:newDesign.revision,stage:'implementation'},fixture);
    const final=saveHandoff({...newBody,taskId:'change-two',operationId:'design-only',expectedRevision:0,stage:'implementation',source:newBody.result.draft,upstream:{path:newPlan.path,draft:newBody.result.draft}},fixture);
    assert.equal(resolveCurrent(body.taskId,fixture).planning,newPlan.path,'design change inherits approved planning without reconfirmation');
    assert.equal(resolveCurrent(body.taskId,fixture).implementation,final.path);
    assert.equal(designChange.changeFrom.planning,newPlan.path);
    const renamed=renameTask({taskId:'change-one',title:'회피 비용 변경',expectedRevision:designChange.revision,operationId:'rename-change'},fixture);
    assert.equal(renamed.taskId,'회피 비용 변경');
    assert.equal(fs.existsSync(path.join(fixture,newPlan.path)),false);
    assert.equal(resolveCurrent(body.taskId,fixture).planning,'.agents/workflow/tasks/workflow_회피 비용 변경_planning.md');
    assert.equal(resolveCurrent('change-two',fixture).implementation,final.path);
    assert.deepEqual(renameTask({taskId:'change-one',title:'회피 비용 변경',expectedRevision:designChange.revision,operationId:'rename-change'},fixture),renamed);
    const json=JSON.parse(fs.readFileSync(path.join(fixture,'.agents/workflow/tasks/workflow_회피 비용 변경_planning.json'),'utf8'));
    assert.deepEqual(json.decisions,newBody.decisions,'rename preserves human decisions');
    const renameBody={taskId:'change-two',title:'후속 설계 (검증)',expectedRevision:resolveCurrent(body.taskId,fixture).revision,operationId:'rename-recovery'};
    try{
      fs.renameSync=(from,to)=>{if(to.endsWith('workflow_후속 설계 (검증)_current.json'))throw new Error('rename interrupted');return rename(from,to);};
      assert.throws(()=>renameTask(renameBody,fixture),/rename interrupted/);
      assert.throws(()=>resolveCurrent(body.taskId,fixture),/제목 변경 저장/);
    }finally{fs.renameSync=rename;}
    const recovered=renameTask(renameBody,fixture);
    assert.deepEqual(renameTask(renameBody,fixture),recovered);
    assert.equal(resolveCurrent(body.taskId,fixture).taskId,'후속 설계 (검증)');
    assert.equal(fs.existsSync(path.join(fixture,final.path)),false);
    assert(fs.readFileSync(path.join(fixture,'.agents/workflow/tasks/index.md'),'utf8').includes('%28'));
    assert.throws(()=>renameTask({...renameBody,taskId:renameBody.title,title:body.taskId,expectedRevision:recovered.revision,operationId:'duplicate-title'},fixture),/이미 사용/);
    for(const title of ['../escape','bad/name','bad:name','bad.','bad?'])assert.throws(()=>renameTask({taskId:'change-two',title,expectedRevision:final.revision,operationId:'invalid'},fixture));

  }finally{
    const resolved=path.resolve(fixture),temp=path.resolve(os.tmpdir());
    assert.equal(path.dirname(resolved),temp);assert(path.basename(resolved).startsWith('wx-workflow-'));
    fs.rmSync(resolved,{recursive:true,force:true});
  }
  console.log('PASS decision preservation, reopen reasons, convergence, contextual requests, HTTP validation, protocol revision and durable handoff');
  if(['--live','--live-planning'].includes(process.argv[2])){
    const mode=process.argv[2]==='--live-planning'?'planning':'implementation';
    const result=await analyze('플레이어 회피는 스태미나 20을 소모한다. 취소 시 비용 반환은 미정이다.',process.argv[3],mode);
    validateResult(result);console.log(`PASS live ${mode}: ${result.questions.length} decisions`);
  }
})().catch(error=>{console.error(error);process.exitCode=1;});
