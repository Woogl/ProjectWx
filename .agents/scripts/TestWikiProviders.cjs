// Copyright Woogle. All Rights Reserved.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const { EventEmitter } = require('node:events');
const { invocation, parseResponse, runProvider } = require('./Wiki-AI-Providers.cjs');
const sample = { summary:'처리', changes:[], checks:[{ name:'검사', status:'passed', evidence:'exit 0' }], humanChecks:[], blockers:[] };
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
    await assert.rejects(runProvider({provider:'claude',command:{file:process.execPath},prompt:'x',repo:dir,output,schema,
      execute:(_file,_args,_options,done)=>({stdin:{on(){},end(){done({killed:true});}}})}), /초과/);
    assert.throws(() => runProvider({provider:'claude',command:null,prompt:'x',repo:dir,output,schema}), /Claude Code/);
    const denied = await runProvider({provider:'claude',command:{file:'claude'},prompt:'x',repo:dir,output,schema,
      execute:(_file,_args,_options,done)=>({stdin:{on(){},end(){done(null,JSON.stringify({structured_output:{...sample,blockers:[]},permission_denials:[{tool_name:'Bash'}]}));}}})});
    assert.match(denied.blockers[0], /권한이 거부/, 'denied tools cannot produce completion');
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
    let spawned;
    const visibleCodex = await runProvider({provider:'codex',command:{file:'codex'},prompt:'x',repo:dir,output,schema,visible:true,onSpawn:pid=>{spawned=pid;},launch:(file,args,options)=>{
      assert.deepEqual(options.stdio, ['pipe', 'inherit', 'inherit']);assert.equal(options.windowsHide, false);assert.equal(flag(args, '--sandbox'), 'danger-full-access');
      return fakeChild((_prompt, child) => { fs.writeFileSync(output, JSON.stringify(sample)); queueMicrotask(() => child.close(0)); });
    }});
    assert.deepEqual(visibleCodex, sample);assert.equal(spawned, 4321);assert(!fs.existsSync(output));
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
    console.log('PASS provider routing, plan/work permission modes, stdin isolation, response parsing, Gemini tool narrowing, visible Codex and streamed Claude progress, errors and cleanup');
  } finally {
    assert.equal(path.dirname(dir), path.resolve(os.tmpdir()));
    assert(path.basename(dir).startsWith('wx-providers-'));
    fs.rmSync(dir, { recursive:true, force:true });
  }
})().catch(error => { console.error(error); process.exitCode=1; });
