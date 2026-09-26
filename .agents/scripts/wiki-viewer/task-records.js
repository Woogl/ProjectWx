// Copyright Woogle. All Rights Reserved.
// 작업 기록의 제목 아래 상태·다음 행동 줄과 테스트 체크리스트 표를 읽는다. 로컬 서버와 Workflow 화면이 같이 쓴다.
var WxTaskRecords = (() => {
  const owners = ['AI', '사람'], results = ['대기', '통과', '실패', '미실행'];
  const checklistHeading = '## 테스트 체크리스트', checklistHeader = '| 항목 | 확인 방법 | 담당 | 결과 | 근거 |';
  const requestHeading = '## 요청', questionHeading = '## 질문', questionHeader = '| ID | 질문 | 선택지 | 추천 | 답변 |', planHeading = '## 구현 계획';
  // 웹에서 만든 작업의 절은 이 순서로 제목 아래에 둔다. 나머지 절은 이력이다.
  const sectionOrder = [requestHeading, questionHeading, planHeading, checklistHeading];
  const stateLine = /^((?:[-*]\s*)?)상태:\s*(확인 대기|진행 중|완료)(?=$|[\s·:,(])\s*[·:,]?\s*(.*)$/, nextLine = /^((?:[-*]\s*)?)다음 행동:\s*(.*)$/;
  const validRow = row => !!row && ['item', 'method', 'evidence'].every(key => typeof row[key] === 'string') && !!row.item.trim() && owners.includes(row.owner) && results.includes(row.result);
  // 제목 아래부터 첫 절 전까지가 기록의 현재 상태다.
  function headRange(lines) {
    const title = lines.findIndex(line => line.startsWith('# ')), end = lines.findIndex((line, i) => i > title && line.startsWith('## '));
    return { title, end: end < 0 ? lines.length : end };
  }
  function readHead(content) {
    const lines = content.split(/\r?\n/), range = headRange(lines), head = { title: range.title < 0 ? '' : lines[range.title].slice(2).trim(), state: '', detail: '', next: '' };
    for (let i = range.title + 1; i < range.end; i++) {
      const state = lines[i].match(stateLine), next = lines[i].match(nextLine);
      if (state && !head.state) { head.state = state[2]; head.detail = state[3].trim(); }
      else if (next && !head.next) head.next = next[2].trim();
    }
    return head;
  }
  function readSection(content, heading) {
    const lines = content.split(/\r?\n/), at = lines.indexOf(heading);
    if (at < 0) return null;
    const end = lines.findIndex((line, i) => i > at && line.startsWith('## '));
    return lines.slice(at + 1, end < 0 ? lines.length : end).join('\n').trim();
  }
  // 절 제목 바로 아래의 5열 표를 읽는다. 머리글이나 행이 어긋나면 이름을 붙여 알린다.
  function readTable(content, heading, header, name, valid) {
    const lines = content.split(/\r?\n/), at = lines.indexOf(heading);
    if (at < 0) return null;
    let table = at + 1; while (table < lines.length && !lines[table].trim()) table++;
    if (lines[table]?.trim() !== header || !/^\| ---/.test(lines[table + 1] || '')) throw Error(`${name} 표의 머리글이 올바르지 않습니다.`);
    const rows = []; let end = table + 2;
    for (; end < lines.length && lines[end].trim().startsWith('|'); end++) {
      const line = lines[end].trim(), cells = line.slice(1, -1).split('|').map(cell => cell.trim());
      if (!line.endsWith('|') || cells.length !== 5 || !valid(cells)) throw Error(`${name} ${rows.length + 1}번째 행의 형식이 올바르지 않습니다.`);
      rows.push(cells);
    }
    return { rows, table, end };
  }
  function readChecklist(content) {
    const found = readTable(content, checklistHeading, checklistHeader, '테스트 체크리스트', ([item, method, owner, result, evidence]) => validRow({ item, method, owner, result, evidence }));
    return found && { ...found, rows: found.rows.map(([item, method, owner, result, evidence]) => ({ item, method, owner, result, evidence })) };
  }
  // 답변 칸이 비어 있으면 아직 답하지 않은 질문이다. 선택지는 ' / '로 나눈다.
  function readQuestions(content) {
    const found = readTable(content, questionHeading, questionHeader, '질문', ([id, question]) => /^[A-Za-z0-9_-]{1,20}$/.test(id) && !!question);
    return found && { ...found, rows: found.rows.map(([id, question, options, recommendation, answer]) => ({ id, question, options: options ? options.split(' / ') : [], recommendation, answer })) };
  }
  // 구현 계획 본문과 '구현 승인: 확인자 날짜' 줄을 읽는다. 계획이 바뀌면 승인 줄도 사라진다. 본문은 승인 줄만 빼고 그대로 둔다.
  function readPlan(content) {
    const body = readSection(content, planHeading);
    if (body === null) return { text: '', approval: '' };
    const lines = body.split('\n'), at = lines.findIndex(line => /^구현 승인: /.test(line));
    const approval = at < 0 ? '' : lines.splice(at, 1)[0].replace(/^구현 승인: /, '').trim();
    return { text: lines.join('\n').trim(), approval };
  }
  // 목록 한 줄에 필요한 값만 만든다. 상태 줄이 없는 기록(모듈 리뷰 등)은 state가 빈 문자열이고, 표가 깨진 기록은 상태 줄의 설명을 보인다.
  function readTaskRecord(path, content, modified = '') {
    const head = readHead(content); let rows = null;
    try { rows = readChecklist(content)?.rows || null; } catch {}
    const summary = rows ? `체크리스트 ${rows.filter(row => row.result === '통과').length}/${rows.length} 통과` : head.detail;
    return { path, title: head.title || path.split('/').pop(), state: head.state, summary, next: head.next, modified };
  }
  return { owners, results, checklistHeading, checklistHeader, requestHeading, questionHeading, questionHeader, planHeading, sectionOrder, stateLine, nextLine, validRow, headRange, readHead, readSection, readChecklist, readQuestions, readPlan, readTaskRecord };
})();
if (typeof module !== 'undefined') module.exports = WxTaskRecords;
