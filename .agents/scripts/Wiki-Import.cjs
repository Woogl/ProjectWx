// Copyright Woogle. All Rights Reserved.
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const { spawn } = require('node:child_process');
const maxFileSize = 20 * 1024 * 1024;
function importDocument(body) {
  if (!body || typeof body.name !== 'string' || typeof body.content !== 'string') throw new Error('첨부 파일이 필요합니다.');
  const extension = path.extname(body.name).toLowerCase();
  if (!['.docx', '.pdf', '.pptx'].includes(extension)) throw new Error('Word(.docx), PDF, PowerPoint(.pptx)를 선택하세요.');
  if (!body.content.length || body.content.length > Math.ceil(maxFileSize / 3) * 4 || (body.content.length % 4 !== 0 || !/^[A-Za-z0-9+/]*={0,2}$/.test(body.content))) throw new Error('첨부 파일 크기 또는 형식이 올바르지 않습니다.');
  const bytes = Buffer.from(body.content, 'base64');
  if (bytes.length > maxFileSize) throw new Error('첨부 파일은 20MB 이하여야 합니다.');
  const bundled = path.join(os.homedir(), '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe');
  const python = process.env.WX_WIKI_PYTHON || (fs.existsSync(bundled) ? bundled : 'python');
  return new Promise((resolve, reject) => {
    const child = spawn(python, [path.join(__dirname, 'Wiki-Import.py'), extension], { windowsHide: true, stdio: ['pipe', 'pipe', 'pipe'] });
    let output = '', done = false;
    const finish = (error, result) => { if(done)return;done=true;clearTimeout(timer);error ? reject(error) : resolve(result); };
    const timer = setTimeout(() => { child.kill(); finish(new Error('문서 읽기 시간이 초과됐습니다. 파일을 나누어 첨부하세요.')); }, 30000);
    child.on('error', () => finish(new Error('문서 변환용 Python을 찾지 못했습니다. Python 또는 WX_WIKI_PYTHON 설정을 확인하세요.')));
    child.stdin.on('error', () => {});
    child.stderr.resume();
    child.stdout.on('data', data => { output += data.toString(); if(output.length > 4 * 1024 * 1024){child.kill();finish(new Error('변환 결과가 너무 큽니다.'));} });
    child.on('close', code => {
      if(done)return;
      try { if(code !== 0)throw new Error('문서 변환에 실패했습니다.');const result=JSON.parse(output);if(result.error)throw new Error(result.error);finish(null,result); }
      catch(error){finish(error);}
    });
    child.stdin.end(bytes);
  });
}
module.exports = { importDocument };
