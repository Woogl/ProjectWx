// Copyright Woogle. All Rights Reserved.
const fs = require('node:fs');
const path = require('node:path');
const { execFile, spawn } = require('node:child_process');
const labels = { codex: 'Codex', claude: 'Claude Code', gemini: 'Gemini CLI' };
const writeTools = ['replace', 'write_file', 'run_shell_command'];

// plan은 조사만 하는 읽기 전용이다. work는 사용자 결정(2026-09-25)에 따라 권한 확인 없이 모든 명령을 실행한다.
// stream이면 Claude Code가 진행 이벤트를 한 줄씩 내보낸다.
function invocation(provider, schema, output, mode = 'work', stream = false) {
  if (!['plan', 'work'].includes(mode)) throw new Error('지원하지 않는 AI 실행 모드입니다.');
  const work = mode === 'work';
  if (provider === 'codex') return ['exec', '--sandbox', work ? 'danger-full-access' : 'read-only', '--ephemeral', '--output-schema', schema, '--output-last-message', output, '-'];
  if (provider === 'claude') return ['--print', '--output-format', stream ? 'stream-json' : 'json', ...(stream ? ['--verbose'] : []), '--json-schema', JSON.stringify(JSON.parse(fs.readFileSync(schema, 'utf8'))), ...(work ? ['--permission-mode', 'bypassPermissions'] : ['--tools', 'Read,Glob,Grep', '--allowedTools', 'Read,Glob,Grep', '--permission-mode', 'dontAsk']), '--strict-mcp-config', '--mcp-config', '{"mcpServers":{}}', '--no-session-persistence', '--disable-slash-commands', '--settings', '{"disableAllHooks":true}'];
  if (provider === 'gemini') return ['--prompt', '표준 입력의 작업 요청을 수행하고 JSON 객체만 반환하세요.', '--output-format', 'json', '--approval-mode', work ? 'yolo' : 'default', '--extensions', 'none', '--allowed-mcp-server-names', 'wx-wiki-no-mcp'];
  throw new Error('지원하지 않는 AI 서비스입니다.');
}

function parseResponse(provider, stdout, output) {
  if (provider === 'codex') return JSON.parse(fs.readFileSync(output, 'utf8'));
  const envelope = JSON.parse(stdout);
  if (envelope.is_error || envelope.error) throw new Error('AI 서비스가 처리 실패를 반환했습니다.');
  if (provider === 'claude') {
    if (!envelope.structured_output) throw new Error('Claude Code의 구조화된 응답이 없습니다. CLI를 업데이트하세요.');
    return envelope.structured_output;
  }
  if (typeof envelope.response !== 'string') throw new Error('Gemini CLI의 응답이 없습니다.');
  return JSON.parse(envelope.response.trim().replace(/^```(?:json)?\s*\n([\s\S]*?)\n```$/, '$1'));
}

// Claude Code 진행 이벤트에서 AI의 말과 도구 호출만 한 줄로 보여준다.
function showClaudeEvent(event, print) {
  if (event.type !== 'assistant') return;
  for (const block of event.message?.content || []) {
    if (block.type === 'text' && block.text.trim()) print(block.text.trim());
    if (block.type === 'tool_use') print(`→ ${block.name} ${String(block.input?.command || block.input?.file_path || block.input?.pattern || block.input?.path || block.input?.description || '').replace(/\s+/g, ' ').slice(0, 160)}`);
  }
}

// visible이면 Codex·Claude Code의 진행 과정을 현재 콘솔(터미널 창)에 보여준다. Gemini CLI는 끝난 뒤 결과만 받는다.
function runProvider({ provider, command, prompt, repo, output, schema, mode = 'work', visible = false, execute = execFile, launch = spawn, onSpawn = () => {}, print = console.log }) {
  if (!Object.hasOwn(labels,provider)) throw new Error('지원하지 않는 AI 서비스입니다.');
  if (!command?.file) throw new Error(`${labels[provider]} CLI 설치·로그인이 필요합니다.`);
  const stream = visible && provider === 'claude';
  const args = [...(command.args || []), ...invocation(provider, schema, output, mode, stream)];
  const env = { ...process.env };
  let settingsFile;
  if (provider === 'gemini') {
    prompt += '\n다음 JSON Schema를 정확히 따르는 객체만 반환하세요. 마크다운 설명을 붙이지 마세요.\n' + fs.readFileSync(schema, 'utf8');
    const existingPath = env.GEMINI_CLI_SYSTEM_SETTINGS_PATH || (process.platform==='win32'?path.join(env.ProgramData || 'C:/ProgramData', 'gemini-cli/settings.json'):process.platform==='darwin'?'/Library/Application Support/GeminiCli/settings.json':'/etc/gemini-cli/settings.json');
    const existing = fs.existsSync(existingPath) ? JSON.parse(fs.readFileSync(existingPath, 'utf8')) : {};
    const restricted = JSON.parse(fs.readFileSync(path.join(__dirname, 'wiki-gemini-settings.json'), 'utf8'));
    const allowed = mode === 'work' ? restricted.tools.core : restricted.tools.core.filter(tool => !writeTools.includes(tool));
    // Preserve administrator policies and authentication; narrow only this child process's tools.
    const core = existing.tools?.core ? allowed.filter(tool => existing.tools.core.includes(tool)) : allowed;
    settingsFile = output + '.settings.json';
    fs.writeFileSync(settingsFile, JSON.stringify({ ...existing, tools: { ...existing.tools, core }, hooksConfig: { ...existing.hooksConfig, enabled: false }, mcp: { ...existing.mcp, allowed: restricted.mcp.allowed }, skills: { ...existing.skills, enabled: false } }));
    env.GEMINI_CLI_SYSTEM_SETTINGS_PATH = settingsFile;
  }
  const cleanup = () => { for (const file of [output, settingsFile].filter(Boolean)) { try { fs.unlinkSync(file); } catch {} } };
  const failure = killed => new Error(killed ? 'AI 처리 시간이 초과되었습니다. 기존 기록과 변경은 보존됩니다.' : `${labels[provider]} 처리에 실패했습니다. CLI 설치·로그인·권한·사용 한도를 확인하세요.`);
  // Claude Code의 최종 메시지(단일 JSON 또는 스트림의 result 이벤트)에서 응답을 꺼낸다.
  const finish = stdout => {
    const result = parseResponse(provider, stdout, output);
    if (provider === 'claude' && JSON.parse(stdout).permission_denials?.length && Array.isArray(result.blockers)) result.blockers.push('Claude Code에서 도구 실행 권한이 거부되었습니다. 거부된 검증·정리 범위를 확인하세요.');
    return result;
  };
  return new Promise((resolve, reject) => {
    let child;
    if (visible && provider !== 'gemini') {
      let killed = false, pending = '', last = '';
      child = launch(command.file, args, { cwd: repo, env, stdio: ['pipe', stream ? 'pipe' : 'inherit', 'inherit'], windowsHide: false });
      const timer = setTimeout(() => { killed = true; child.kill(); }, 30*60*1000);
      const read = line => { try { const event = JSON.parse(line); if (event.type === 'result') last = line; else showClaudeEvent(event, print); } catch {} };
      if (stream) {
        child.stdout.setEncoding('utf8');
        child.stdout.on('data', chunk => { const lines = (pending + chunk).split('\n'); pending = lines.pop(); lines.filter(line => line.trim()).forEach(read); });
      }
      child.on('error', () => {});
      child.on('close', code => {
        clearTimeout(timer);
        try {
          if (pending.trim()) read(pending);
          if (code !== 0) throw failure(killed);
          resolve(stream ? finish(last || '{}') : parseResponse(provider, '', output));
        }
        catch (error) { reject(error); }
        finally { cleanup(); }
      });
    } else {
      child = execute(command.file, args, { cwd: repo, env, windowsHide: true, timeout: 30*60*1000, maxBuffer: 8 * 1024 * 1024 }, (error, stdout) => {
        try {
          if (error) throw failure(error.killed);
          resolve(finish(stdout));
        } catch (error) { reject(error); }
        finally { cleanup(); }
      });
    }
    if(child.pid)onSpawn(child.pid);
    child.stdin.on('error', () => {});
    child.stdin.end(prompt);
  });
}
module.exports = { labels, invocation, parseResponse, runProvider };
