// Copyright Woogle. All Rights Reserved.
const assert = require('node:assert/strict');
const { spawnSync } = require('node:child_process');
const path = require('node:path');
const fs = require('node:fs');
const os = require('node:os');
const { createServer } = require('./Wiki-AI.cjs');
const bundled = path.join(os.homedir(), '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe');
const python = process.env.WX_WIKI_PYTHON || (fs.existsSync(bundled) ? bundled : 'python');
const fixtures = spawnSync(python, ['-c', `
import io, zipfile, json, base64
from pypdf import PdfWriter
from pypdf.generic import DictionaryObject, NameObject, DecodedStreamObject
def package(files):
    output=io.BytesIO()
    with zipfile.ZipFile(output,'w') as z:
        for name,text in files.items(): z.writestr(name,text)
    return base64.b64encode(output.getvalue()).decode()
docx=package({'word/document.xml':'<document><p><t>Planning</t></p><tbl><tr><tc><p><t>Cost 20</t></p></tc></tr></tbl></document>'})
pptx=package({'ppt/presentation.xml':'<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"><p:sldIdLst><p:sldId r:id="second"/><p:sldId r:id="first"/></p:sldIdLst></p:presentation>', 'ppt/_rels/presentation.xml.rels':'<Relationships><Relationship Id="first" Target="slides/slide1.xml"/><Relationship Id="second" Target="slides/slide2.xml"/></Relationships>', 'ppt/slides/slide1.xml':'<slide><p><t>First</t></p></slide>', 'ppt/slides/slide2.xml':'<slide><p><t>Second</t></p></slide>'})
writer=PdfWriter();page=writer.add_blank_page(200,200)
font=DictionaryObject({NameObject('/Type'):NameObject('/Font'),NameObject('/Subtype'):NameObject('/Type1'),NameObject('/BaseFont'):NameObject('/Helvetica')})
page[NameObject('/Resources')]=DictionaryObject({NameObject('/Font'):DictionaryObject({NameObject('/F1'):writer._add_object(font)})})
stream=DecodedStreamObject();stream.set_data(b'BT /F1 12 Tf 10 100 Td (Planning PDF) Tj ET');page[NameObject('/Contents')]=writer._add_object(stream)
output=io.BytesIO();writer.write(output)
print(json.dumps({'docx':docx,'pptx':pptx,'pdf':base64.b64encode(output.getvalue()).decode()}))
`], { encoding: 'utf8' });
assert.equal(fixtures.status, 0, fixtures.stderr);
const files = JSON.parse(fixtures.stdout);
(async () => {
  const server = createServer({ token: 'test-import', port: 18745, providers: [] });
  await new Promise(resolve => server.listen(18745, '127.0.0.1', resolve));
  const send = (name, content, token = 'test-import') => fetch('http://127.0.0.1:18745/import', { method: 'POST', headers: { Origin: 'null', 'Content-Type': 'application/json', 'X-Wx-Token': token }, body: JSON.stringify({ name, content }) });
  try {
    for (const extension of ['docx', 'pptx', 'pdf']) {
      const response = await send('plan.' + extension, files[extension]);
      assert.equal(response.status, 200);
      const result = await response.json();
      assert.ok(result.warnings.length);
      if (extension === 'docx') assert.match(result.text, /Planning\nCost 20/);
      if (extension === 'pptx') assert.ok(result.text.indexOf('Second') < result.text.indexOf('First'));
      if (extension === 'pdf') assert.match(result.text, /Planning PDF/);
    }
    assert.equal((await send('plan.docx', files.docx, 'bad')).status, 403);
    assert.equal((await send('plan.doc', files.docx)).status, 400);
    assert.equal((await send('broken.docx', Buffer.from('broken').toString('base64'))).status, 400);
    assert.equal((await send('plan.pdf', '!!!')).status, 400);
    console.log('PASS DOCX table text, PPTX presentation order, PDF text, import authentication and malformed files');
  } finally { await new Promise(resolve => server.close(resolve)); }
})().catch(error => { console.error(error); process.exitCode = 1; });
