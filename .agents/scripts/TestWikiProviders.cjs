// Copyright Woogle. All Rights Reserved.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const { invocation, parseResponse, runProvider } = require('./Wiki-AI-Providers.cjs');
const { validateResult } = require('./Wiki-AI.cjs');
const schema = path.resolve(__dirname, 'wiki-checklist.schema.json');
const sample = { summary:'검토', draft:'확정 초안', facts:[], evidence:[], blockers:[], items:[], questions:[] };
(async () => {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'wx-providers-'));
  try {
    const output = path.join(dir, 'result.json');
    assert.throws(() => invocation('unknown', schema, output));
    assert.throws(() => parseResponse('claude', '{"is_error":true}', output));
    assert.throws(() => parseResponse('claude', '{"result":"text"}', output));
    assert.throws(() => parseResponse('gemini', '{"response":"invalid"}', output));
    assert.throws(() => validateResult(parseResponse('gemini', '{"response":"{}"}', output)));
    assert.deepEqual(parseResponse('gemini', JSON.stringify({response:'```json\n'+JSON.stringify(sample)+'\n```'}), output), sample);
    for (const provider of ['codex', 'claude', 'gemini']) {
      let received;
      const execute = (file, args, options, done) => {
        assert.equal(file, process.execPath);
        assert.equal(options.cwd, dir);
        assert.equal(options.shell, undefined);
        if (provider === 'codex') assert(args.includes('read-only'));
        if (provider === 'claude') {
          assert.equal(args[args.indexOf('--tools')+1], 'Read,Glob,Grep');
          assert(args.includes('--strict-mcp-config'));
          assert.equal(args[args.indexOf('--permission-mode')+1], 'dontAsk');
        }
        if (provider === 'gemini') {
          assert.equal(args[args.indexOf('--approval-mode')+1], 'plan');
          const settings = JSON.parse(fs.readFileSync(options.env.GEMINI_CLI_SYSTEM_SETTINGS_PATH, 'utf8'));
          assert.equal(settings.hooksConfig.enabled, false);
          assert(!settings.tools.core.includes('run_shell_command'));
          assert(!settings.tools.core.includes('write_file'));
        }
        return { stdin: { on() {}, end(prompt) {
          received = prompt;
          if (provider === 'codex') fs.writeFileSync(output, JSON.stringify(sample));
          done(null, JSON.stringify(provider === 'claude' ? { structured_output:sample } : { response:JSON.stringify(sample) }));
        } } };
      };
      const result = await runProvider({provider, command:{file:process.execPath}, prompt:'판단 보존 $() ` & 한글', repo:dir, output, execute});
      assert.deepEqual(validateResult(result), sample);
      assert(received.startsWith('판단 보존 $() ` & 한글'));
      assert(!fs.existsSync(output));
      assert(!fs.existsSync(output+'.settings.json'));
    }
    await assert.rejects(runProvider({provider:'claude',command:{file:process.execPath},prompt:'x',repo:dir,output,
      execute:(_file,_args,_options,done)=>({stdin:{on(){},end(){done({killed:true});}}})}), /초과/);
    console.log('PASS provider routing, read-only arguments, stdin isolation, response validation, errors and cleanup');
  } finally {
    assert.equal(path.dirname(dir), path.resolve(os.tmpdir()));
    assert(path.basename(dir).startsWith('wx-providers-'));
    fs.rmSync(dir, { recursive:true, force:true });
  }
})().catch(error => { console.error(error); process.exitCode=1; });
