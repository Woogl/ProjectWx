// Copyright Woogle. All Rights Reserved.
const fs = require('node:fs');
const path = require('node:path');
const { execFile } = require('node:child_process');
const labels = { codex: 'Codex', claude: 'Claude Code', gemini: 'Gemini CLI' };

function invocation(provider, schema, output) {
  if (provider === 'codex') return ['exec', '--sandbox', 'read-only', '--ephemeral', '--output-schema', schema, '--output-last-message', output, '-'];
  if (provider === 'claude') return ['--print', '--output-format', 'json', '--json-schema', JSON.stringify(JSON.parse(fs.readFileSync(schema, 'utf8'))), '--tools', 'Read,Glob,Grep', '--allowedTools', 'Read,Glob,Grep', '--permission-mode', 'dontAsk', '--strict-mcp-config', '--mcp-config', '{"mcpServers":{}}', '--no-session-persistence', '--disable-slash-commands', '--settings', '{"disableAllHooks":true}'];
  if (provider === 'gemini') return ['--prompt', '표준 입력의 검토 요청을 수행하고 JSON 객체만 반환하세요.', '--output-format', 'json', '--approval-mode', 'plan', '--extensions', 'none', '--allowed-mcp-server-names', 'wx-wiki-no-mcp', '--policy', path.join(__dirname, 'wiki-gemini-policy.toml')];
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

function runProvider({ provider, command, prompt, repo, output, execute = execFile }) {
  const schema = path.join(__dirname, 'wiki-checklist.schema.json');
  const args = [...(command.args || []), ...invocation(provider, schema, output)];
  const env = { ...process.env };
  let settingsFile;
  if (provider === 'gemini') {
    prompt += '\n다음 JSON Schema를 정확히 따르는 객체만 반환하세요. 마크다운 설명을 붙이지 마세요.\n' + fs.readFileSync(schema, 'utf8');
    const existingPath = env.GEMINI_CLI_SYSTEM_SETTINGS_PATH || path.join(env.ProgramData || 'C:/ProgramData', 'gemini-cli/settings.json');
    const existing = fs.existsSync(existingPath) ? JSON.parse(fs.readFileSync(existingPath, 'utf8')) : {};
    const restricted = JSON.parse(fs.readFileSync(path.join(__dirname, 'wiki-gemini-settings.json'), 'utf8'));
    // Preserve administrator policies and authentication; narrow only this child process's tools.
    const core = existing.tools?.core ? restricted.tools.core.filter(tool => existing.tools.core.includes(tool)) : restricted.tools.core;
    settingsFile = output + '.settings.json';
    fs.writeFileSync(settingsFile, JSON.stringify({ ...existing, tools: { ...existing.tools, core }, hooksConfig: { ...existing.hooksConfig, enabled: false }, mcp: { ...existing.mcp, allowed: restricted.mcp.allowed }, skills: { ...existing.skills, enabled: false } }));
    env.GEMINI_CLI_SYSTEM_SETTINGS_PATH = settingsFile;
  }
  return new Promise((resolve, reject) => {
    const child = execute(command.file, args, { cwd: repo, env, windowsHide: true, timeout: 240000, maxBuffer: 4 * 1024 * 1024 }, (error, stdout) => {
      try {
        if (error) throw new Error(error.killed ? 'AI 검토 시간이 초과되었습니다. 기존 판단은 보존됩니다.' : `${labels[provider]} 검토에 실패했습니다. CLI 설치·로그인·사용 한도를 확인하세요.`);
        resolve(parseResponse(provider, stdout, output));
      } catch (error) { reject(error); }
      finally { for (const file of [output, settingsFile].filter(Boolean)) { try { fs.unlinkSync(file); } catch {} } }
    });
    child.stdin.on('error', () => {});
    child.stdin.end(prompt);
  });
}
module.exports = { labels, invocation, parseResponse, runProvider };
