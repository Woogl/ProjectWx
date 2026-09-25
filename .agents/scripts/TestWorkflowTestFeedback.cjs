// Copyright Woogle. All Rights Reserved.
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const {execFileSync}=require('node:child_process');
const {createFeedbackService,runFeedback}=require('./Workflow-TestFeedback.cjs');
const {codeVersion}=require('./Workflow-Execution.cjs');
const {createServer}=require('./Wiki-AI.cjs');
const base=fs.mkdtempSync(path.join(os.tmpdir(),'wx-test-feedback-'));
const taskPath='.agents/workflow/tasks/example.md';
const indexText='# 작업 기록\n\n'+['플레이 확인','에디터 확인','개선 판단','완료 기록'].map((group,i)=>`## ${group}\n\n| 작업 | 확인된 범위 | 다음 행동 |\n| --- | --- | --- |\n${i===0?'| [예시 작업](example.md) | 구현·빌드 | 저장 후 복원을 확인한다. |\n':''}`).join('\n');
const report={summary:'테스트 결과 반영',changes:['근거 대조'],checks:[{name:'정리',status:'passed',evidence:'기존 결정·남은 항목 대조, 추가 Wiki 지식 없음'}],humanChecks:[],blockers:[]};
const settle=()=>new Promise(resolve=>setImmediate(resolve));
let sequence=0,server;
function fixture(providers=['codex']){
  const root=path.join(base,'case-'+(++sequence)),folder=path.join(root,'.agents/workflow/tasks');fs.mkdirSync(folder,{recursive:true});
  fs.writeFileSync(path.join(folder,'index.md'),indexText);fs.writeFileSync(path.join(root,taskPath),'# 예시 작업\n\n원본 결정과 검증 근거\n');
  let version='v1',busy=false,finish,reject,runs=0,lastInput;
  const options={root,providers,fingerprint:()=>version,externalBusy:()=>busy,run:input=>{lastInput=input;runs++;return new Promise((a,b)=>{finish=a;reject=b;});}};
  let service=createFeedbackService(options);
  const context=()=>service.act({action:'read',taskPath});
  const request=(extra={})=>({action:'submit',taskPath,operationId:'op-'+(++sequence),expectedRevision:context().revision,taskHash:context().taskHash,codeVersion:context().codeVersion,result:'passed',actor:'테스터',scope:'저장 후 복원',notes:'',...extra});
  return {root,folder,context,request,get service(){return service;},get runs(){return runs;},get input(){return lastInput;},set version(v){version=v;},set busy(v){busy=v;},finish:value=>finish(value),fail:()=>reject(Error('일시적 CLI 실패')),restart:()=>{service=createFeedbackService(options);},index:()=>fs.readFileSync(path.join(folder,'index.md'),'utf8'),task:()=>fs.readFileSync(path.join(root,taskPath),'utf8')};
}
(async()=>{try{
  const f=fixture();
  assert.throws(()=>f.service.act(f.request({provider:'claude'})),/선택한 AI/);
  assert.throws(()=>f.service.act(f.request({provider:'unknown'})),/선택한 AI/);
  assert.throws(()=>f.service.act(f.request({result:'issues'})),/문제 상황/);
  assert.throws(()=>f.service.act(f.request({result:'both'})),/하나/);
  assert.throws(()=>f.service.act(f.request({actor:'\n사용자'})),/확인자/);
  assert.throws(()=>f.service.act(f.request({scope:' '})),/확인한 항목/);
  assert.throws(()=>f.service.act({action:'read',taskPath:'.agents/workflow/tasks/../process/index.md'}),/경로/);
  const outdated=f.request();f.version='v2';assert.throws(()=>f.service.act(outdated),/바뀌었습니다/);
  const oldTask=f.request();fs.appendFileSync(path.join(f.root,taskPath),'\n새 결정\n');assert.throws(()=>f.service.act(oldTask),/바뀌었습니다/);
  f.busy=true;assert.throws(()=>f.service.act(f.request()),/다른 AI/);f.busy=false;
  const sent=f.request();assert.equal(f.service.act(sent).latest.status,'running');await settle();
  assert.equal(f.service.act(sent).latest.status,'running');assert.equal(f.runs,1);
  assert.equal(f.input.provider,'codex','old requests retain Codex behavior');
  assert.throws(()=>f.service.act({...sent,notes:'내용 변경'}),/같은 접수/);
  assert.throws(()=>f.service.act(f.request()),/다른 AI/);
  assert.equal(f.service.act({action:'list'}).records[taskPath].latest.actor,'테스터');
  f.finish(report);await settle();assert.equal(f.context().latest.status,'complete');assert.equal(f.service.isBusy(),false);
  assert.match(f.task(),/원본 결정과 검증 근거/);assert.match(f.task(),/제출 결과: 이상 없음/);assert.match(f.task(),/> 저장 후 복원/);
  assert.match(f.index().split('## 완료 기록')[1],/예시 작업/);assert.equal(f.index().split('[예시 작업]').length,2);
  const recorded=f.task();f.service.act(sent);await settle();assert.equal(f.runs,1);assert.equal(f.task(),recorded);
  f.restart();assert.equal(f.context().latest.status,'complete');

  const multi=fixture(['claude','gemini']);
  assert.deepEqual(multi.context().providers.map(p=>p.id),['claude','gemini']);
  const claude=multi.request({provider:'claude',result:'issues',notes:'문제 원문'});
  multi.service.act(claude);await settle();assert.equal(multi.input.provider,'claude');multi.fail();await settle();
  multi.restart();assert.equal(multi.context().latest.provider,'claude');
  const switchAI=multi.request({action:'retry',provider:'gemini'});multi.service.act(switchAI);await settle();
  assert.equal(multi.input.provider,'gemini');assert.equal(multi.input.notes,'문제 원문');assert.equal(multi.input.attempts[0].provider,'claude');
  multi.service.act(switchAI);assert.equal(multi.runs,2);multi.fail();await settle();
  multi.service.act(multi.request({action:'retry'}));await settle();assert.equal(multi.input.provider,'gemini','retry without a selection preserves its provider');
  multi.finish(report);await settle();assert.match(multi.task(),/처리 AI: Gemini CLI/);
  assert.throws(()=>multi.service.act({...claude,provider:'gemini'}),/같은 접수/);

  // 완료 기록에도 새 문제가 발생하면 확인할 일로 돌아간다.
  const issue=f.request({result:'issues',notes:'복원 직후 좌표가 (0, 0, 0)입니다.\n재현: 저장 → 종료 → 재개'});
  f.service.act(issue);await settle();f.finish({...report,humanChecks:['복원 좌표 재확인']});await settle();
  assert.equal(f.context().latest.status,'retest');assert.match(f.task(),/> 재현: 저장 → 종료 → 재개/);
  assert.match(f.index().split('## 플레이 확인')[1].split('## 에디터 확인')[0],/예시 작업/);
  assert.doesNotMatch(f.index().split('## 완료 기록')[1],/예시 작업/);
  f.service.act(f.request());await settle();f.finish(report);await settle();assert.equal(f.context().latest.status,'complete');
  for(const [label,result,expected] of [
    ['human',{...report,humanChecks:['미확인 플레이']},'retest'],
    ['not-run',{...report,checks:[{name:'플레이',status:'not_run',evidence:'에디터 연결 없음'}]},'retest'],
    ['empty',{...report,checks:[]},'retest'],
    ['failed',{...report,checks:[{name:'회귀',status:'failed',evidence:'assert 실패'}]},'blocked'],
    ['blocker',{...report,blockers:['기존 코드 리뷰 확인 필요']},'blocked']
  ]){const c=fixture();c.service.act(c.request());await settle();c.finish(result);await settle();assert.equal(c.context().latest.status,expected,label);}
  const changed=fixture();changed.service.act(changed.request());await settle();changed.version='v2';changed.finish(report);await settle();assert.equal(changed.context().latest.status,'retest');
  const revised=fixture();revised.service.act(revised.request());await settle();fs.appendFileSync(path.join(revised.root,taskPath),'\n추가 결정\n');revised.finish(report);await settle();assert.equal(revised.context().latest.status,'blocked');assert.match(revised.task(),/추가 결정/);assert.equal(revised.index(),indexText);

  const failed=fixture();failed.service.act(failed.request({result:'issues',notes:'문제 원문'}));await settle();failed.fail();await settle();
  assert.equal(failed.context().latest.status,'failed');assert.equal(failed.context().latest.notes,'문제 원문');
  failed.version='v2';assert.throws(()=>failed.service.act(failed.request({action:'retry'})),/새 테스트/);failed.version='v1';
  const retry=failed.request({action:'retry'});failed.service.act(retry);await settle();failed.service.act(retry);assert.equal(failed.runs,2);
  failed.finish(report);await settle();assert.equal(failed.context().latest.status,'retest');
  const interrupted=fixture();interrupted.service.act(interrupted.request());await settle();interrupted.restart();assert.equal(interrupted.context().latest.status,'interrupted');
  interrupted.service.act(interrupted.request({action:'retry'}));await settle();interrupted.finish(report);await settle();assert.equal(interrupted.context().latest.status,'complete');
  const orphan=fixture();orphan.service.act(orphan.request());await settle();
  const stateFile=fs.readdirSync(orphan.folder).find(name=>name.startsWith('test_feedback_'));
  const state=JSON.parse(fs.readFileSync(path.join(orphan.folder,stateFile)));state.requests[0].workerPid=process.pid;fs.writeFileSync(path.join(orphan.folder,stateFile),JSON.stringify(state));
  orphan.restart();assert.equal(orphan.service.isBusy(),true);assert.throws(()=>orphan.service.act(orphan.request({action:'retry'})),/다른 AI/);

  // 문서·접수 기록 정리만으로 코드 버전을 바꾸지 않으며 실제 구현 변경은 감지한다.
  const gitRoot=path.join(base,'git');fs.mkdirSync(gitRoot);
  const git=(...args)=>execFileSync('git',['-c','safe.directory='+gitRoot.replace(/\\/g,'/'),...args],{cwd:gitRoot,windowsHide:true,stdio:'pipe'});
  git('init');fs.writeFileSync(path.join(gitRoot,'code.cpp'),'int value=1;');fs.writeFileSync(path.join(gitRoot,'README.md'),'old');git('add','.');git('-c','user.name=Test','-c','user.email=test@example.invalid','commit','-m','fixture');
  const before=codeVersion(gitRoot,true);fs.writeFileSync(path.join(gitRoot,'README.md'),'new');fs.mkdirSync(path.join(gitRoot,'.wiki'));fs.writeFileSync(path.join(gitRoot,'.wiki','note.md'),'note');
  assert.equal(codeVersion(gitRoot,true),before);assert.notEqual(codeVersion(gitRoot),before);
  fs.writeFileSync(path.join(gitRoot,'code.cpp'),'int value=2;');assert.notEqual(codeVersion(gitRoot,true),before);

  let prompt,args;
  const output=await runFeedback({root:f.root,command:{file:'codex'},request:issue,execute:(file,argv,options,callback)=>{
    args=argv;assert.equal(options.cwd,f.root);assert.equal(options.windowsHide,true);
    const outputFile=argv[argv.indexOf('--output-last-message')+1];
    return {stdin:{on(){},end(value){prompt=value;fs.writeFileSync(outputFile,JSON.stringify(report));queueMicrotask(()=>callback(null));}}};
  }});
  assert.equal(output.summary,report.summary);assert.match(prompt,/問題|문제 원인|원인을 조사/);assert.ok(prompt.includes(issue.notes.replace(/\n/g,'\\n')));
  assert.match(prompt,/일부 항목의 이상 없음을 작업 전체 완료로 확대하지 마세요/);assert.equal(args[args.indexOf('--sandbox')+1],'workspace-write');

  for(const provider of ['codex','claude','gemini']){
    let received,spawned;
    const value=await runFeedback({root:f.root,command:{file:provider,args:['prefix']},request:{...issue,provider},onSpawn:pid=>{spawned=pid;},execute:(file,argv,options,callback)=>{
      assert.equal(file,provider);assert.equal(argv[0],'prefix');assert.equal(options.cwd,f.root);assert.equal(options.shell,undefined);assert.equal(options.windowsHide,true);
      assert.equal(options.timeout,30*60*1000);assert.ok(!argv.some(value=>/yolo|bypass|skip-permissions/.test(value)));
      if(provider==='codex')assert.equal(argv[argv.indexOf('--sandbox')+1],'workspace-write');
      if(provider==='claude'){
        assert.equal(argv[argv.indexOf('--permission-mode')+1],'acceptEdits');assert.equal(argv[argv.indexOf('--permission-prompts')+1],'none');
        assert.match(argv[argv.indexOf('--tools')+1],/Edit,Write,Bash/);assert.equal(argv[argv.indexOf('--allowedTools')+1],'Read,Glob,Grep');
        assert.ok(JSON.parse(argv[argv.indexOf('--json-schema')+1]).required.includes('checks'));
      }
      if(provider==='gemini'){
        assert.equal(argv[argv.indexOf('--approval-mode')+1],'auto_edit');assert.ok(!argv.includes('--policy'));
        const settings=JSON.parse(fs.readFileSync(options.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH));
        assert.ok(settings.tools.core.includes('write_file'));assert.ok(settings.tools.core.includes('replace'));assert.ok(settings.tools.core.includes('run_shell_command'));assert.equal(settings.hooksConfig.enabled,false);
      }
      return {pid:12345,stdin:{on(){},end(prompt){received=prompt;
        if(provider==='codex')fs.writeFileSync(argv[argv.indexOf('--output-last-message')+1],JSON.stringify(report));
        queueMicrotask(()=>callback(null,JSON.stringify(provider==='claude'?{structured_output:report}:{response:JSON.stringify(report)})));
      }}};
    }});
    assert.equal(value.summary,report.summary);assert.equal(spawned,12345);assert.ok(received.includes(issue.notes.replace(/\n/g,'\\n')));assert.match(received,/권한.*우회하거나 변경하지/);
    assert.equal(fs.readdirSync(path.join(f.root,'Saved/Wiki')).length,0,'execution schema/output/settings must be removed');
  }
  const denied=await runFeedback({root:f.root,command:{file:'claude'},request:{...issue,provider:'claude'},execute:(_file,_args,_options,callback)=>({stdin:{on(){},end(){callback(null,JSON.stringify({structured_output:report,permission_denials:[{tool_name:'Bash'}]}));}}})});
  assert.match(denied.blockers[0],/권한이 거부/,'denied tools cannot produce completion');
  await assert.rejects(runFeedback({root:f.root,command:{file:'gemini'},request:{...issue,provider:'gemini'},execute:(_file,_args,_options,callback)=>({stdin:{on(){},end(){callback(null,JSON.stringify({response:'not JSON'}));}}})}));
  await assert.rejects(runFeedback({root:f.root,command:null,request:{...issue,provider:'claude'}}),/Claude Code/);
  const settingsPath=path.join(base,'admin-settings.json'),previousSettings=process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH;
  fs.writeFileSync(settingsPath,JSON.stringify({tools:{core:['read_file'],exclude:['run_shell_command']},security:{auth:{selectedType:'test-login'}},customPolicy:'preserved'}));
  process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH=settingsPath;
  try{
    await runFeedback({root:f.root,command:{file:'gemini'},request:{...issue,provider:'gemini'},execute:(_file,_args,options,callback)=>{
      const settings=JSON.parse(fs.readFileSync(options.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH));
      assert.deepEqual(settings.tools.core,['read_file']);assert.deepEqual(settings.tools.exclude,['run_shell_command']);assert.equal(settings.security.auth.selectedType,'test-login');assert.equal(settings.customPolicy,'preserved');
      return {stdin:{on(){},end(){callback(null,JSON.stringify({response:JSON.stringify(report)}));}}};
    }});
  }finally{if(previousSettings===undefined)delete process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH;else process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH=previousSettings;}

  const http=fixture(['codex','claude','gemini']),token='feedback-test',port=18746;
  server=createServer({token,port,testFeedback:http.service,runAnalysis:async()=>({})});await new Promise(resolve=>server.listen(port,'127.0.0.1',resolve));
  const post=(route,body,headers={})=>fetch('http://127.0.0.1:'+port+route,{method:'POST',headers:{'Content-Type':'application/json','X-Wx-Token':token,Origin:'null',...headers},body:JSON.stringify(body)});
  assert.equal((await post('/test-feedback',{action:'list'},{'X-Wx-Token':'bad'})).status,403);
  assert.equal((await post('/test-feedback',{action:'list'},{Origin:'https://example.com'})).status,403);
  assert.equal((await post('/test-feedback',http.request({provider:'unknown'}))).status,400);
  const response=await post('/test-feedback',http.request({provider:'claude'}));assert.equal(response.status,200);await settle();assert.equal(http.input.provider,'claude');
  assert.equal((await post('/test-feedback',{action:'list'})).status,200);
  assert.equal((await post('/execution',{})).status,409);
  assert.equal((await post('/analyze',{})).status,409);
  assert.equal((await (await fetch('http://127.0.0.1:'+port+'/health')).json()).busy,true);
  http.finish(report);await settle();assert.equal(http.context().latest.status,'complete');
  console.log('PASS feedback validation, durable submission/replay, result classification, task/index updates, version checks, retry/restart, worker lock, CLI prompt and HTTP authorization/busy routing');
}finally{
  if(server){server.closeAllConnections();await new Promise(resolve=>server.close(resolve));}
  const resolved=path.resolve(base);assert.equal(path.dirname(resolved),path.resolve(os.tmpdir()));assert.ok(path.basename(resolved).startsWith('wx-test-feedback-'));fs.rmSync(resolved,{recursive:true,force:true});
}})().catch(error=>{console.error(error);process.exitCode=1;});
