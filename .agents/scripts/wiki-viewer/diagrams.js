// Copyright Woogle. All Rights Reserved.
let wikiDiagramId = 0;
async function renderWikiDiagrams() {
  if (typeof mermaid === 'undefined') return;
  for (const code of $('article').querySelectorAll('pre.mermaid, pre > code.language-mermaid')) {
    const source = code.textContent;
    const original = code.tagName === 'PRE' ? code : code.parentElement;
    const figure = el('figure', undefined, 'wiki-diagram');
    const status = el('p', '다이어그램을 표시하는 중입니다.');
    const details = el('details');
    const pre = el('pre');
    pre.append(el('code', source));
    details.append(el('summary', 'Mermaid 원문'), pre);
    figure.append(status, details);
    original.replaceWith(figure);
    const id = 'wx-diagram-' + (++wikiDiagramId);
    try {
      if (/^\s*---/.test(source) || /%%\s*\{/.test(source)) throw new Error('diagram configuration is managed by the viewer');
      const result = await mermaid.render(id, source);
      if (!figure.isConnected) continue;
      const img = el('img');
      img.alt = '문서 다이어그램. 상세 관계와 조건은 Mermaid 원문 및 본문을 참고하세요.';
      // Mermaid may emit HTML void tags in foreignObject; serialize valid XML for image decoding.
      const svg = new DOMParser().parseFromString(result.svg, 'text/html').querySelector('svg');
      if (!svg) throw new Error('missing diagram SVG');
      const width = Number((svg.getAttribute('viewBox') || '').split(/\s+/)[2]);
      if (Number.isFinite(width) && width > 0) { img.width = Math.ceil(width); }
      img.src = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(new XMLSerializer().serializeToString(svg));
      status.replaceWith(img);
    } catch {
      if (!figure.isConnected) continue;
      status.textContent = '다이어그램을 표시하지 못했습니다. Mermaid 문법과 뷰어 지원 범위를 확인하세요.';
      status.className = 'diagram-error';
      details.open = true;
    }
  }
}
