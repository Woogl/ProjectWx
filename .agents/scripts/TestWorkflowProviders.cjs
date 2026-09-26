// Copyright Woogle. All Rights Reserved.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const { EventEmitter } = require('node:events');
const { invocation, parseResponse, runProvider } = require('./Workflow-Providers.cjs');
const sample = { summary:'처리', evidence:['exit 0'], changes:[], questions:[], checklist:[] };
(async () => {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'wx-providers-'));
  try {
    const output = path.join(dir, 'result.json'), schema = path.join(dir, 'schema.json');
    fs.writeFileSync(schema, JSON.stringify({ type:'object' }));
    assert.throws(() => invocation('unknown', schema, output));
    assert.throws(() => invocation('codex', schema, output, 'other'), /모드/);
    assert.throws(() => parseResponse('claude', '{"is_error":true}', output));
    assert.throws(() => parseResponse('claude', '{"result":"text"}', output));
    assert.throws(() => parseResponse('gemini', '{"response":"invalid"}', output));
    assert.deepEqual(parseResponse('gemini', JSON.stringify({response:'```json\n'+JSON.stringify(sample)+'\n```'}), output), sample);
    // 정하기(plan)는 읽기 전용, 구현·수정(work)은 사용자 결정에 따라 권한 확인 없이 실행한다.
    const flag = (args, name) => args[args.indexOf(name) + 1];
    assert.equal(flag(invocation('codex', schema, output, 'plan'), '--sandbox'), 'read-only');
    assert.equal(flag(invocation('codex', schema, output, 'work'), '--sandbox'), 'danger-full-access');
    const claudePlan = invocation('claude', schema, output, 'plan'), claudeWork = invocation('claude', schema, output, 'work');
    assert.equal(flag(claudePlan, '--tools'), 'Read,Glob,Grep');assert.equal(flag(claudePlan, '--permission-mode'), 'dontAsk');assert.ok(!claudePlan.includes('bypassPermissions'));
    assert.equal(flag(claudeWork, '--permission-mode'), 'bypassPermissions');assert.ok(!claudeWork.includes('--tools'));assert.equal(flag(claudeWork, '--output-format'), 'json');
    const claudeStream = invocation('claude', schema, output, 'work', true);
    assert.equal(flag(claudeStream, '--output-format'), 'stream-json');assert.ok(claudeStream.includes('--verbose'));
    assert.equal(flag(invocation('gemini', schema, output, 'plan'), '--approval-mode'), 'default');
    assert.equal(flag(invocation('gemini', schema, output, 'work'), '--approval-mode'), 'yolo');
    // Codex plan은 샌드박스 밖에서 도는 플러그인·앱을 끄고, Codex가 알려준 설정 MCP 서버 가운데 켜진 것만 -c로 끈다. work는 그대로 둔다.
    assert.ok(!invocation('codex', schema, output, 'work').includes('--disable'));
    const codexCalls = [];
    const codexPlan = await runProvider({provider:'codex',command:{file:'codex',args:['--profile-arg']},prompt:'x',repo:dir,output,schema,mode:'plan',execute:(_file,args,options,done)=>{
      codexCalls.push(args);
      if (args.includes('mcp')) { assert.equal(options.cwd, dir); done(null, JSON.stringify([{name:'node_repl',enabled:true},{name:'unreal-mcp',enabled:true},{name:'already-off',enabled:false}])); return; }
      return { stdin:{ on() {}, end() { fs.writeFileSync(output, JSON.stringify(sample)); done(null, ''); } } };
    }});
    assert.deepEqual(codexPlan, sample);
    assert.deepEqual(codexCalls[0], ['--profile-arg', '--disable', 'plugins', 'mcp', 'list', '--json'], 'the server list comes from the same Codex and config');
    assert.equal(codexCalls[1].join(' '), `--profile-arg exec --sandbox read-only --disable plugins --disable apps -c mcp_servers.node_repl.enabled=false -c mcp_servers.unreal-mcp.enabled=false --ephemeral --output-schema ${schema} --output-last-message ${output} -`);
    let codexStarted = false;
    await assert.rejects(runProvider({provider:'codex',command:{file:'codex'},prompt:'x',repo:dir,output,schema,mode:'plan',execute:(_file,args,_options,done)=>{
      if (args.includes('mcp')) return done(Object.assign(new Error('unknown subcommand'), { code:2 }));
      codexStarted = true;
    }}), /MCP 서버 목록/);
    assert.equal(codexStarted, false, 'a plan without MCP isolation does not start');
    // 실행 제한은 plan 30분, work 60분이고, 시간이 지나면 AI 프로세스 트리를 끝낸다.
    const realSetTimeout = global.setTimeout, limits = [];
    global.setTimeout = (fn, ms, ...rest) => { limits.push(ms); return realSetTimeout(fn, ms, ...rest); };
    try {
      for (const mode of ['plan', 'work']) await runProvider({provider:'claude',command:{file:'claude'},prompt:'x',repo:dir,output,schema,mode,execute:(_file,_args,_options,done)=>({stdin:{on(){},end(){done(null,JSON.stringify({structured_output:sample}));}}})});
    } finally { global.setTimeout = realSetTimeout; }
    assert.deepEqual(limits, [30 * 60 * 1000, 60 * 60 * 1000]);
    let stopped;
    await assert.rejects(runProvider({provider:'claude',command:{file:'claude'},prompt:'x',repo:dir,output,schema,limit:5,
      stop:child=>{stopped=child;child.finish(Object.assign(new Error('killed'), { code:1 }));},
      execute:(_file,_args,_options,done)=>({pid:77,finish:done,stdin:{on(){},end(){}}})}), /처리 시간\(0분\)이 지나 AI와 하위 프로세스를 끝냈습니다/);
    assert.equal(stopped.pid, 77, 'the time limit stops the AI process tree');
    for (const provider of ['codex', 'claude', 'gemini']) {
      let received;
      const execute = (file, args, options, done) => {
        assert.equal(file, process.execPath);
        assert.equal(options.cwd, dir);
        assert.equal(options.shell, undefined);
        assert.equal(options.windowsHide, true);
        return { stdin: { on() {}, end(prompt) {
          received = prompt;
          if (provider === 'codex') fs.writeFileSync(output, JSON.stringify(sample));
          done(null, JSON.stringify(provider === 'claude' ? { structured_output:sample } : { response:JSON.stringify(sample) }));
        } } };
      };
      const result = await runProvider({provider, command:{file:process.execPath}, prompt:'판단 보존 $() ` & 한글', repo:dir, output, schema, execute});
      assert.deepEqual(result, sample);
      assert(received.startsWith('판단 보존 $() ` & 한글'));
      assert(!fs.existsSync(output));
      assert(!fs.existsSync(output+'.settings.json'));
    }
    assert.throws(() => runProvider({provider:'claude',command:null,prompt:'x',repo:dir,output,schema}), /Claude Code/);
    const denied = await runProvider({provider:'claude',command:{file:'claude'},prompt:'x',repo:dir,output,schema,
      execute:(_file,_args,_options,done)=>({stdin:{on(){},end(){done(null,JSON.stringify({structured_output:sample,permission_denials:[{tool_name:'Bash'}]}));}}})});
    assert.match(denied.evidence.at(-1), /권한이 거부/, 'denied tools are reported as evidence');
    // Gemini는 plan에서 쓰기·셸 도구를 빼고, 관리자 설정은 그대로 둔 채 이 실행의 도구만 좁힌다.
    for (const [mode, writable] of [['plan', false], ['work', true]]) {
      await runProvider({provider:'gemini',command:{file:'gemini'},prompt:'x',repo:dir,output,schema,mode,execute:(_file,_args,options,done)=>{
        const settings = JSON.parse(fs.readFileSync(options.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH));
        for (const tool of ['write_file', 'replace', 'run_shell_command']) assert.equal(settings.tools.core.includes(tool), writable, mode + ' ' + tool);
        assert.ok(settings.tools.core.includes('read_file'));assert.equal(settings.hooksConfig.enabled, false);
        return {stdin:{on(){},end(){done(null,JSON.stringify({response:JSON.stringify(sample)}));}}};
      }});
    }
    const settingsPath = path.join(dir, 'admin-settings.json'), previousSettings = process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH;
    fs.writeFileSync(settingsPath, JSON.stringify({tools:{core:['read_file'],exclude:['run_shell_command']},security:{auth:{selectedType:'test-login'}},customPolicy:'preserved'}));
    process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH = settingsPath;
    try {
      await runProvider({provider:'gemini',command:{file:'gemini'},prompt:'x',repo:dir,output,schema,execute:(_file,_args,options,done)=>{
        const settings = JSON.parse(fs.readFileSync(options.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH));
        assert.deepEqual(settings.tools.core,['read_file']);assert.deepEqual(settings.tools.exclude,['run_shell_command']);assert.equal(settings.security.auth.selectedType,'test-login');assert.equal(settings.customPolicy,'preserved');
        return {stdin:{on(){},end(){done(null,JSON.stringify({response:JSON.stringify(sample)}));}}};
      }});
    } finally { if (previousSettings === undefined) delete process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH; else process.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH = previousSettings; }
    // 터미널 창 실행: Codex는 출력을 그대로 보여주고, Claude Code는 진행 이벤트를 한 줄씩 보여준 뒤 결과 이벤트를 쓴다.
    const fakeChild = (onEnd, stdout = null) => { const handlers = {}; const child = { pid:4321, stdout, stdin:{ on() {}, end(prompt) { onEnd(prompt, child); } }, on(name, fn) { handlers[name] = fn; }, kill() {}, close(code) { handlers.close(code); } }; return child; };
    const visibleCodex = await runProvider({provider:'codex',command:{file:'codex'},prompt:'x',repo:dir,output,schema,visible:true,launch:(file,args,options)=>{
      assert.deepEqual(options.stdio, ['pipe', 'inherit', 'inherit']);assert.equal(options.windowsHide, false);assert.equal(flag(args, '--sandbox'), 'danger-full-access');
      return fakeChild((_prompt, child) => { fs.writeFileSync(output, JSON.stringify(sample)); queueMicrotask(() => child.close(0)); });
    }});
    assert.deepEqual(visibleCodex, sample);assert(!fs.existsSync(output));
    const printed = [], lines = [
      JSON.stringify({type:'system',subtype:'init'}),
      JSON.stringify({type:'assistant',message:{content:[{type:'text',text:'체력바 코드를 읽습니다.'},{type:'tool_use',name:'Read',input:{file_path:'Source/WxUI/Boss.cpp'}}]}}),
      JSON.stringify({type:'user',message:{content:[{type:'tool_result',content:'...'}]}}),
      JSON.stringify({type:'assistant',message:{content:[{type:'tool_use',name:'Bash',input:{command:'git status\n--short'}}]}}),
      JSON.stringify({type:'result',structured_output:sample,permission_denials:[]})].join('\n');
    const streamed = await runProvider({provider:'claude',command:{file:'claude'},prompt:'x',repo:dir,output,schema,visible:true,print:line=>printed.push(line),launch:(file,args,options)=>{
      assert.deepEqual(options.stdio, ['pipe', 'pipe', 'inherit']);assert.equal(flag(args, '--output-format'), 'stream-json');
      const stdout = new EventEmitter();stdout.setEncoding = encoding => assert.equal(encoding, 'utf8');
      return fakeChild((_prompt, child) => { stdout.emit('data', lines.slice(0, 25)); stdout.emit('data', lines.slice(25)); queueMicrotask(() => child.close(0)); }, stdout);
    }});
    assert.deepEqual(streamed, sample);assert.deepEqual(printed, ['체력바 코드를 읽습니다.', '→ Read Source/WxUI/Boss.cpp', '→ Bash git status --short']);
    await assert.rejects(runProvider({provider:'claude',command:{file:'claude'},prompt:'x',repo:dir,output,schema,visible:true,print(){},launch:()=>{
      const stdout = new EventEmitter();stdout.setEncoding = () => {};
      return fakeChild((_prompt, child) => { stdout.emit('data', lines.split('\n')[1] + '\n'); queueMicrotask(() => child.close(0)); }, stdout);
    }}), /구조화된 응답이 없습니다/, 'a stream without a result event is a failure');
    await assert.rejects(runProvider({provider:'codex',command:{file:'codex'},prompt:'x',repo:dir,output,schema,visible:true,launch:()=>fakeChild((_prompt, child) => queueMicrotask(() => child.close(1)))}), /Codex 처리에 실패/);
    console.log('PASS provider routing, plan/work permission modes, Codex plan MCP isolation, time limits with process-tree stop, stdin isolation, response parsing, Gemini tool narrowing, visible Codex and streamed Claude progress, errors and cleanup');
  } finally {
    assert.equal(path.dirname(dir), path.resolve(os.tmpdir()));
    assert(path.basename(dir).startsWith('wx-providers-'));
    fs.rmSync(dir, { recursive:true, force:true });
  }
})().catch(error => { console.error(error); process.exitCode=1; });
