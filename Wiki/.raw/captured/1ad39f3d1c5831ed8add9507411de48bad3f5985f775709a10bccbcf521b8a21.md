---
title: "Workflow 문서 이미지 기능 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-26
tags: [wx, workflow]
summary: "구 워크플로우의 4단계 그림을 지운 뒤 쓰는 곳이 없던 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 사용자 결정으로 없앴다. 화면은 Markdown 이미지를 그리지 않고, 도식은 Mermaid로 그린다. 옛 목차 제목을 건너뛰던 생성 코드도 지웠다."
---

# 사용자 결정

2026-09-26 구 AI 워크플로우 잔재를 지운 뒤, 쓰는 이미지가 없어진 문서 이미지 기능을 남길지 묻자 사용자가 "네, 이미지 기능도 지워주세요."라고 했다. 이어 "이미지 기능은 나중에 다시 쓸 가능성이 있으니 유지합시다. 아니다, 지금 안쓰면 지웁시다"라고 최종 결정했다.

# 변경

- `Export-Wiki.ps1`: `.agents/workflow/assets`의 PNG를 data URI로 묶어 화면 데이터(`images`)에 넣던 코드를 지웠다. 옛 Workflow 목차의 `## 한줄 요약` 제목을 건너뛰던 줄도 지웠다. 지금은 두 목차 모두 그 제목이 없다.
- 뷰어(`wiki-viewer/index.html`): 문서 기준 상대 경로로 묶은 PNG를 찾던 `wikiImageSource`를 지웠다. Markdown을 정리하는 `safeFragment`는 `IMG`를 허용하지 않아 문서의 이미지를 그리지 않으며, 쓰지 않게 된 기준 경로 인자도 뺐다. 문서 이미지용 `article img` 규칙을 지우고, 다이어그램 가운데 정렬에 필요한 `display:block`은 `.wiki-diagram img`로 옮겼다. 다이어그램이 SVG data URI 이미지로 그려지므로 CSP의 `img-src data:`는 남겼다.
- TestWikiViewer: PNG 묶기·상대 경로 해석·외부 이미지 거부 검사를 Markdown 이미지 제거와 다이어그램 이미지 정책 검사로 바꿨다.

# 검증

- Export-Wiki.ps1로 두 화면을 다시 만들었다(각 59문서, 생성 HTML에 `images` 데이터 없음). CheckWikiLinks 오류 0건(61문서).
- TestWikiViewer·TestWikiSpaces·TestWorkflowFeedbackUI·TestWorkflowTestFeedback·TestWikiProviders를 통과했다.
- 헤드리스 Edge로 Workflow 화면의 작업 절차와 Wiki 화면의 Wiki·Workflow 도구 구조를 열었다. 도식 1개가 block 이미지로 그려지고 좌우 여백이 같았으며(37px·17px), 문서 본문 이미지는 0개였다.
