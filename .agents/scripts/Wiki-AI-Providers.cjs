// Copyright Woogle. All Rights Reserved.
const fs = require('node:fs');
const path = require('node:path');
const { execFile } = require('node:child_process');
const labels = { codex: 'Codex', claude: 'Claude Code', gemini: 'Gemini CLI' };

function invocation(provider, schema, output, mode='review') {
  if (!['review','execution'].includes(mode)) throw new Error('지원하지 않는 AI 실행 모드입니다.');
  const writes=mode==='execution';
  if (provider === 'codex') return ['exec', '--sandbox', writes?'workspace-write':'read-only', '--ephemeral', '--output-schema', schema, '--output-last-message', output, '-'];
  if (provider === 'claude') return ['--print', '--output-format', 'json', '--json-schema', JSON.stringify(JSON.parse(fs.readFileSync(schema, 'utf8'))), '--tools', writes?'Read,Glob,Grep,Edit,Write,Bash':'Read,Glob,Grep', '--allowedTools', 'Read,Glob,Grep', '--permission-mode', writes?'acceptEdits':'dontAsk', ...(writes?['--permission-prompts','none']:[]), '--strict-mcp-config', '--mcp-config', '{"mcpServers":{}}', '--no-session-persistence', '--disable-slash-commands', '--settings', '{"disableAllHooks":true}'];
  if (provider === 'gemini') return ['--prompt', '표준 입력의 작업 요청을 수행하고 JSON 객체만 반환하세요.', '--output-format', 'json', '--approval-mode', writes?'auto_edit':'plan', '--extensions', 'none', '--allowed-mcp-server-names', 'wx-wiki-no-mcp', ...(writes?[]:['--policy', path.join(__dirname, 'wiki-gemini-policy.toml')])];
  throw new Error('지원하지 않는 AI 서비스입니다.');
}

function parseResponse(provider, stdout, output) {
  if (provider === 'codex') return JSON.parse(fs.readFileSync(output, 'utf8'));
  const envelope = JSON.parse(stdout);
  if (envelope.is_error || envelope.error) throw new Error('AI 서비스가 검토 실패를 반환했습니다.');
  if (provider === 'claude') {
    if (!envelope.structured_output) throw new Error('Claude Code의 구조화된 응답이 없습니다. CLI를 업데이트하세요.');
    return envelope.structured_output;
  }
  if (typeof envelope.response !== 'string') throw new Error('Gemini CLI의 응답이 없습니다.');
  return JSON.parse(envelope.response.trim().replace(/^```(?:json)?\s*\n([\s\S]*?)\n```$/, '$1'));
}

function runProvider({ provider, command, prompt, repo, output, execute = execFile, mode='review', schema=path.join(__dirname, 'wiki-checklist.schema.json'), onSpawn=()=>{} }) {
  if (!Object.hasOwn(labels,provider)) throw new Error('지원하지 않는 AI 서비스입니다.');
  if (!command?.file) throw new Error(`${labels[provider]} CLI 설치·로그인이 필요합니다.`);
  const args = [...(command.args || []), ...invocation(provider, schema, output, mode)];
  const env = { ...process.env };
  let settingsFile;
  if (provider === 'gemini') {
    prompt += '\n다음 JSON Schema를 정확히 따르는 객체만 반환하세요. 마크다운 설명을 붙이지 마세요.\n' + fs.readFileSync(schema, 'utf8');
    const existingPath = env.GEMINI_CLI_SYSTEM_SETTINGS_PATH || (process.platform==='win32'?path.join(env.ProgramData || 'C:/ProgramData', 'gemini-cli/settings.json'):process.platform==='darwin'?'/Library/Application Support/GeminiCli/settings.json':'/etc/gemini-cli/settings.json');
    const existing = fs.existsSync(existingPath) ? JSON.parse(fs.readFileSync(existingPath, 'utf8')) : {};
    const restricted = JSON.parse(fs.readFileSync(path.join(__dirname, 'wiki-gemini-settings.json'), 'utf8'));
    if(mode==='execution')restricted.tools.core.push('replace','write_file','run_shell_command');
    // Preserve administrator policies and authentication; narrow only this child process's tools.
    const core = existing.tools?.core ? restricted.tools.core.filter(tool => existing.tools.core.includes(tool)) : restricted.tools.core;
    settingsFile = output + '.settings.json';
    fs.writeFileSync(settingsFile, JSON.stringify({ ...existing, tools: { ...existing.tools, core }, hooksConfig: { ...existing.hooksConfig, enabled: false }, mcp: { ...existing.mcp, allowed: restricted.mcp.allowed }, skills: { ...existing.skills, enabled: false } }));
    env.GEMINI_CLI_SYSTEM_SETTINGS_PATH = settingsFile;
  }
  return new Promise((resolve, reject) => {
    const child = execute(command.file, args, { cwd: repo, env, windowsHide: true, timeout: mode==='execution'?30*60*1000:240000, maxBuffer: (mode==='execution'?8:4) * 1024 * 1024 }, (error, stdout) => {
      try {
        if (error) throw new Error(error.killed ? 'AI 처리 시간이 초과되었습니다. 기존 기록과 변경은 보존됩니다.' : `${labels[provider]} 처리에 실패했습니다. CLI 설치·로그인·권한·사용 한도를 확인하세요.`);
        const result=parseResponse(provider, stdout, output);
        if(mode==='execution'&&provider==='claude'&&JSON.parse(stdout).permission_denials?.length&&Array.isArray(result.blockers))result.blockers.push('Claude Code에서 도구 실행 권한이 거부되었습니다. 거부된 검증·정리 범위를 확인하세요.');
        resolve(result);
      } catch (error) { reject(error); }
      finally { for (const file of [output, settingsFile].filter(Boolean)) { try { fs.unlinkSync(file); } catch {} } }
    });
    if(child.pid)onSpawn(child.pid);
    child.stdin.on('error', () => {});
    child.stdin.end(prompt);
  });
}
module.exports = { labels, invocation, parseResponse, runProvider };
