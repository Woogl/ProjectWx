---
type: concept
title: "Wiki 운영"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "팀 Wiki의 운영 규칙과 전환 이력"
sources:
  - "[[결정 노트 - 2026-09-22-current-workflow]]"
  - "[[결정 노트 - 2026-09-22-verified-stock-rule]]"
  - "[[결정 노트 - 2026-09-25-workflow-simplification]]"
  - "[[결정 노트 - 2026-09-26-refresh-commit-trace]]"
  - "[[결정 노트 - 2026-09-26-workflow-image-removal]]"
  - "[[결정 노트 - 2026-09-26-workflow-legacy-removal]]"
  - "[[결정 노트 - 2026-09-26-workflow-process-diagram]]"
  - "[[결정 노트 - 2026-09-26-workflow-ssot-copies]]"
  - "[[작업 - wiki-regeneration]]"
  - "[[작업 - workflow-review]]"
---

# Wiki 운영

팀 Wiki의 운영 규칙과 전환 이력에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 아직 없음

## 확정 결정

- 옛 .wiki 재생성 작업은 문서 작성일과 인간 검증일을 구분하고 C++ 정적 확인이 빌드·실행·바이너리 에셋 검증을 대신하지 않는다는 기준을 따랐다. ([[작업 - wiki-regeneration]])
- 사용자는 2026-09-22 옛 LLM Wiki 기사의 verified를 WX 전용 인간 확인 방침 대신 순정 규칙(편찬·재확인 때 그날 날짜 기록)으로 되돌리도록 결정했다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 옛 Wiki schema.md는 verified를 편찬 없는 구조 이관에는 넣지 않도록 규정했다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 사용자는 2026-09-26 쓰는 곳이 없는 Wiki·Workflow 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 지우기로 최종 결정했고, 앞서 잔재 제거 때 남겨 둔 보류를 뒤집었다. ([[결정 노트 - 2026-09-26-workflow-image-removal]])
- Wiki의 Workflow 문서는 단계 표·구현 승인 규칙·기록 형식·체크리스트 작성 규칙을 다시 적지 않고 작업 절차 정본을 가리키며, 도구 구조·서버 동작·결정 이유만 담는다. ([[결정 노트 - 2026-09-26-workflow-ssot-copies]])

## 구현 관찰

- 2026-09-22 옛 .wiki 전체 재생성은 새 원자료 15개를 수집하고 기존 16개 기사를 재작성·1개를 추가했으며 참조가 교체된 legacy 19개를 삭제했다. ([[작업 - wiki-regeneration]])
- 구 워크플로우 잔재 제거로 Wiki 뷰어의 문서 이미지 기능(Export-Wiki.ps1 PNG 묶기, wikiImageSource, IMG 허용)이 사용자 결정에 따라 삭제되었다. ([[작업 - workflow-review]])
- 2026-09-22 시점 옛 LLM Wiki는 `.wiki/`를 Git으로 공유하고 게임 규칙·구현·제약·결정 이유를 담았으며, 작업별 판단·확정본·실행 상태는 Workflow가 담당했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 시점 옛 Wiki 설정은 원자료 속 지시를 작업 명령이 아닌 자료로 다루고 구조 이관·재편찬·Lint 성공을 실행 재검증으로 보지 않도록 규정했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- Workflow 단순화 후 로컬 Wiki-AI 서버는 /health와 /test-feedback만 받고(protocol 4) 화면 메뉴는 작업 현황 대시보드·작업 절차·LLM 위키 검색 세 개가 되었다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 2026-09-26 Wiki 최신화는 `39f3629a4` 이후 `bf6596012`까지 커밋 19건을 정적으로 대조했고 새로 반영할 지식 변경은 DefenseConstant 메타 제거 하나였다. ([[결정 노트 - 2026-09-26-refresh-commit-trace]])
- Wiki 최신화는 레벨 배치 변경, 작업 기록만 바뀐 커밋, 회의자료, AGENTS.md 코딩 규칙 변경을 기사 반영 대상에서 제외했다. ([[결정 노트 - 2026-09-26-refresh-commit-trace]])
- Wiki 뷰어는 문서 본문의 Markdown 이미지를 그리지 않고, 도식은 Mermaid로 그려 SVG data URI 이미지로만 표시하며 이를 위해 CSP img-src data:를 남겼다. ([[결정 노트 - 2026-09-26-workflow-image-removal]])
- Wiki 뷰어의 Mermaid 11.12.0 도식에서 되돌림 화살표가 있는 서브그래프는 아래 단계부터 선언해야 위에서부터 순서대로 놓이고, 뷰어는 %%{ 설정 지시문과 frontmatter가 있는 도식을 거부한다. ([[결정 노트 - 2026-09-26-workflow-process-diagram]])

## 검증 범위

- 옛 .wiki 재생성은 lint·링크 검사·뷰어 테스트·Edge 렌더링으로만 검증되었고 빌드와 게임 실행은 하지 않았다. ([[작업 - wiki-regeneration]])
- 옛 Wiki의 verified 날짜는 편찬·재확인 날짜이며 빌드·게임 실행 검증일을 뜻하지 않는다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 2026-09-26 최신화의 커밋 대조는 정적 조사이며 빌드·PIE·에셋 편집기 검증은 하지 않았다. ([[결정 노트 - 2026-09-26-refresh-commit-trace]])
- 문서 이미지 기능 제거는 Export-Wiki 재생성, CheckWikiLinks 0건, 뷰어 자동 테스트, 헤드리스 Edge 렌더로 확인했고 사람의 화면 확인은 노트에 없다. ([[결정 노트 - 2026-09-26-workflow-image-removal]])

## 미결정·충돌

- 구 워크플로우 잔재 제거 노트는 문서 PNG 묶기 기능을 사용자 미결로 남겼지만 같은 날 이미지 기능 제거 결정으로 뒤집혔다. ([[결정 노트 - 2026-09-26-workflow-legacy-removal]])

## 원자료

- [[결정 노트 - 2026-09-22-current-workflow]] — 2026-09-22 시점 AGENTS.md·옛 LLM Wiki 설정·Workflow 절차와 실행 스크립트의 계약을 발췌한 정적 조사 노트로 사람 판단·AI 실행 경계를 기록한다
- [[결정 노트 - 2026-09-22-verified-stock-rule]] — 옛 LLM Wiki 기사의 verified 필드를 WX 전용 인간 확인 방침에서 순정 규칙(편찬·재확인 날짜 기록)으로 되돌린 2026-09-22 사용자 결정 기록
- [[결정 노트 - 2026-09-25-workflow-simplification]] — 사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지하며 AI·사람 담당 테스트 체크리스트를 도입한 기록
- [[결정 노트 - 2026-09-26-refresh-commit-trace]] — 직전 Wiki 최신화 39f3629a4 이후 bf6596012까지 19건 커밋을 정적 대조해 새로 반영할 것은 DefenseConstant 입력 제한 메타 제거 하나였음을 기록
- [[결정 노트 - 2026-09-26-workflow-image-removal]] — 구 워크플로우 그림 삭제 뒤 쓰는 곳이 없던 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 사용자 결정으로 없애고 도식은 Mermaid만 쓰게 한 노트.
- [[결정 노트 - 2026-09-26-workflow-legacy-removal]] — 사용자 요청으로 옛 웹 작업 경로 스크립트·테스트, 한 장으로 합친 옛 절차 문서와 4단계 그림, 옛 결과 형식 표시와 대시보드 CSS를 지운 노트.
- [[결정 노트 - 2026-09-26-workflow-process-diagram]] — 작업 절차 도식을 정하기·만들기·확인하기 단계 상자 배치로 바꾸고 서버·화면 코드와 대조한 노트. 추가 요청 권한과 완료 뒤 재활성 두 가지가 미결이다.
- [[결정 노트 - 2026-09-26-workflow-ssot-copies]] — 워크플로우 SSoT를 엄격히 지키기로 한 사용자 결정에 따라 처리 프롬프트와 Wiki의 규칙 사본을 정리하고 정본을 가리키게 한 노트.
- [[작업 - wiki-regeneration]] — 옛 .wiki LLM Wiki를 현재 코드·설정·기획 기준으로 전면 재생성하고 대체된 legacy 문서를 정리한 2026-09-22 완료 작업 기록
- [[작업 - workflow-review]] — AI 작업 워크플로우를 점검하고 작업 절차 한 장·세 단계·테스트 체크리스트·웹 새 작업과 이어하기로 단순화한 2026-09-22~26 완료 작업 기록
