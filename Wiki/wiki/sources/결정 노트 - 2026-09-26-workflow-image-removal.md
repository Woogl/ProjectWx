---
type: source
title: "결정 노트 - 2026-09-26-workflow-image-removal"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "Wiki"
summary: "구 워크플로우 그림 삭제 뒤 쓰는 곳이 없던 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 사용자 결정으로 없애고 도식은 Mermaid만 쓰게 한 노트."
source_type: decision-note
source_id: src-ef89d987e8946eef6271
sha256: 1ad39f3d1c5831ed8add9507411de48bad3f5985f775709a10bccbcf521b8a21
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-image-removal.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-image-removal.md"
raw_copy: ".raw/captured/1ad39f3d1c5831ed8add9507411de48bad3f5985f775709a10bccbcf521b8a21.md"
claim_ids:
  - clm-52a30a4798-c1
  - clm-52a30a4798-c2
  - clm-52a30a4798-c3
key_claims:
  - "사용자는 2026-09-26 문서 이미지 기능을 유지하자고 했다가 곧바로 지금 안 쓰면 지우자고 최종 결정했다."
  - "이 변경 뒤 Wiki 뷰어의 safeFragment는 IMG를 허용하지 않아 문서 본문 이미지를 그리지 않고, 다이어그램만 SVG data URI 이미지로 표시된다."
  - "이 노트는 구 워크플로우 잔재 제거 노트가 사용자 미결로 남겨 둔 PNG 묶기 기능 보존을 뒤집었다."
---

# 결정 노트 - 2026-09-26-workflow-image-removal

- 원본: `.wiki/raw/notes/2026-09-26-workflow-image-removal.md`
- 원자료 사본: `.raw/captured/1ad39f3d1c5831ed8add9507411de48bad3f5985f775709a10bccbcf521b8a21.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-image-removal.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- 구 AI 워크플로우 잔재를 지운 뒤([[결정 노트 - 2026-09-26-workflow-legacy-removal]]), 쓰는 이미지가 없어진 문서 이미지 기능을 제거했다.
- **이전 결정을 바꿈**: 잔재 제거 노트는 PNG 묶기 기능을 "범용 기능이라 남겼다. 없앨지는 사용자가 정하지 않았다"고 적었다. 이 노트에서 사용자가 제거를 결정해 그 보류를 뒤집었다.

## 사람의 판단 원문

> 사용자 2026-09-26: "네, 이미지 기능도 지워주세요."

> 사용자 2026-09-26: "이미지 기능은 나중에 다시 쓸 가능성이 있으니 유지합시다. 아니다, 지금 안쓰면 지웁시다"

마지막 문장이 최종 결정(제거)이다.

## 변경(구현 관찰)

- `Export-Wiki.ps1`: `.agents/workflow/assets`의 PNG를 data URI로 묶어 화면 데이터(`images`)에 넣던 코드를 지웠다. 옛 Workflow 목차의 `## 한줄 요약` 제목을 건너뛰던 줄도 지웠다(지금은 두 목차 모두 그 제목이 없음).
- 뷰어 `wiki-viewer/index.html`: 상대 경로로 PNG를 찾던 `wikiImageSource`와 쓰지 않는 기준 경로 인자를 지웠다. `safeFragment`는 `IMG`를 허용하지 않아 문서 본문 이미지를 그리지 않는다. `article img` 규칙을 지우고 `display:block`은 `.wiki-diagram img`로 옮겼다.
- 다이어그램은 Mermaid로 그려 SVG data URI 이미지로 표시하므로 CSP의 `img-src data:`는 남겼다.
- TestWikiViewer의 PNG 묶기·상대 경로·외부 이미지 거부 검사를 Markdown 이미지 제거와 다이어그램 이미지 정책 검사로 바꿨다.

## 검증 범위

- 실행: Export-Wiki.ps1로 두 화면을 다시 생성(각 59문서, 생성 HTML에 `images` 데이터 없음), CheckWikiLinks 오류 0건(61문서).
- 자동 테스트 통과: TestWikiViewer·TestWikiSpaces·TestWorkflowFeedbackUI·TestWorkflowTestFeedback·TestWikiProviders.
- 헤드리스 Edge로 Workflow·Wiki 화면을 열어 도식 1개가 block 이미지로 그려지고 좌우 여백(37px·17px)이 같으며 본문 이미지 0개임을 확인했다. 사람의 화면 확인 기록은 이 노트에 없다.

## 관련 주제

- [[Wiki 운영]]
- [[작업 절차(Workflow)]]

## 핵심 주장

- 사용자는 2026-09-26 문서 이미지 기능을 유지하자고 했다가 곧바로 지금 안 쓰면 지우자고 최종 결정했다. ^c1
- 이 변경 뒤 Wiki 뷰어의 safeFragment는 IMG를 허용하지 않아 문서 본문 이미지를 그리지 않고, 다이어그램만 SVG data URI 이미지로 표시된다. ^c2
- 이 노트는 구 워크플로우 잔재 제거 노트가 사용자 미결로 남겨 둔 PNG 묶기 기능 보존을 뒤집었다. ^c3
