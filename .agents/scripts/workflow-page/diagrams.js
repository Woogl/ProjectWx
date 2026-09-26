// Copyright Woogle. All Rights Reserved.
// 문서와 구현 계획의 Mermaid 코드 블록을 순정 mermaid.run으로 그리고 원문은 접어서 곁에 둔다.
async function renderWikiDiagrams(root = $('article')) {
  if (typeof mermaid === 'undefined') return;
  for (const code of root.querySelectorAll('pre.mermaid, pre > code.language-mermaid')) {
    const source = code.textContent;
    const original = code.tagName === 'PRE' ? code : code.parentElement;
    const figure = el('figure', undefined, 'wiki-diagram');
    const diagram = el('div', source, 'mermaid');
    const details = el('details');
    const pre = el('pre');
    pre.append(el('code', source));
    details.append(el('summary', 'Mermaid 원문'), pre);
    figure.append(diagram, details);
    original.replaceWith(figure);
    try {
      await mermaid.run({ nodes: [diagram] });
    } catch {
      if (!figure.isConnected) continue;
      diagram.replaceWith(el('p', '다이어그램을 표시하지 못했습니다. Mermaid 문법과 뷰어 지원 범위를 확인하세요.', 'diagram-error'));
      details.open = true;
    }
  }
}
