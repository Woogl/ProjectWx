---
type: source
title: "결정 노트 - 2026-09-26-workflow-legacy-removal"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "정리"
summary: "사용자 요청으로 옛 웹 작업 경로 스크립트·테스트, 한 장으로 합친 옛 절차 문서와 4단계 그림, 옛 결과 형식 표시와 대시보드 CSS를 지운 노트."
source_type: decision-note
source_id: src-eeabfc3c85b14871517f
sha256: c18496a77a96336911d747c623e7458e5db30d4ecd3331b558228a4b9132d7bb
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-legacy-removal.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-legacy-removal.md"
raw_copy: ".raw/captured/c18496a77a96336911d747c623e7458e5db30d4ecd3331b558228a4b9132d7bb.md"
claim_ids:
  - clm-9e9081322a-c1
  - clm-9e9081322a-c2
  - clm-9e9081322a-c3
key_claims:
  - "사용자 요청(2026-09-26)으로 옛 웹 작업 경로 스크립트 8개와 테스트 5개, 옛 절차 문서 5개와 4단계 그림 ai-workflow.png가 git rm으로 삭제됐다."
  - "Claude Code 권한 거부 알림은 없어진 blockers 칸 대신 처리 근거 evidence에 한 줄로 남도록 옮겨졌다."
  - "이 노트가 남겨 둔 문서 PNG 묶기 기능은 같은 날 사용자 결정으로 제거돼 이 노트의 보존 판단은 더 이상 유효하지 않다."
---

# 결정 노트 - 2026-09-26-workflow-legacy-removal

- 원본: `.wiki/raw/notes/2026-09-26-workflow-legacy-removal.md`
- 원자료 사본: `.raw/captured/c18496a77a96336911d747c623e7458e5db30d4ecd3331b558228a4b9132d7bb.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-legacy-removal.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- [[작업 - workflow-review]]의 삭제 대기 목록은 사용자 승인을 기다리던 상태였고, 사용자 요청으로 실행했다.

## 사람의 판단 원문

> 사용자 2026-09-26: "구 AI 워크플로우의 잔재가 남아있다면 제거합시다."

## 삭제한 파일(확정 결정의 실행)

`git rm`으로 지웠다(사용자가 직접 요청한 삭제라 자동 모드에서도 허용).

- 옛 웹 작업 경로(`.agents/scripts/`): `Wiki-Tasks.cjs`, `Workflow-Execution.cjs`, `Wiki-Import.cjs`, `Wiki-Import.py`, `wiki-checklist.schema.json`, `wiki-gemini-policy.toml`, `wiki-viewer/workflow-model.js`, `wiki-viewer/execution.js`와 그 테스트 5개.
- 작업 절차 한 장으로 합친 옛 절차 문서: `.agents/workflow/process/`의 `design_review.md`·`implementation.md`·`testing.md`·`completion.md`, `.agents/workflow/usage.md`, 4단계 그림 `.agents/workflow/assets/ai-workflow.png`.
- 삭제 전 저장소 전체 검색에서 현재 코드·문서·스킬·배치·에이전트 설정의 참조는 없었고 이력(로그·출처 메모)만 있었다.

## 현재 코드에서 없앤 흔적(구현 관찰)

- 작업 진행 화면 `wiki-viewer/test-feedback.js`: 옛 처리 상태 `blocked`의 "AI 확인 요청" 문구, 옛 보고서의 `blockers`·`checks` 표시, 첫 접수 방식의 `scope` 표시를 없앴다. 서버 `Workflow-TestFeedback.cjs` view에서도 `scope`를 뺐다.
- `Wiki-AI-Providers.cjs`: Claude Code 권한 거부 알림을 없어진 `blockers` 칸에 넣어 버려지던 문제를 처리 근거 `evidence`에 한 줄로 남기도록 옮겼다.
- `wiki-viewer/index.html`: 2026-09-22 옛 대시보드의 통계 카드(`.stats`·`.stat`)와 상태 배지 규칙 9개를 지웠다.
- 테스트 표본을 현재 결과 형식(요약·근거·변경·질문·체크리스트)으로 바꿨다.
- 옛 형식 보고서가 남은 접수 이력은 `checkpoint-savegame`·`animnotify-categories` 둘뿐이라 화면에서 사라지는 내용은 없다. 접수 이력 JSON은 이력이라 고치지 않았다.

## 잔재로 보지 않고 남긴 것

- 옛 결과 칸을 AI가 보내도 버리는지 확인하는 회귀 테스트, 폐지한 웹 경로 API가 돌아오지 않는지 확인하는 뷰어 테스트.
- 표준 절이 없는 기록의 다음 행동을 사람 항목 하나로 보여주는 처리, 자유 형식 상태 줄 기록을 리뷰·참고로 분류하는 처리.
- 문서 PNG 묶기 기능: 이 노트 시점에는 "없앨지는 사용자가 정하지 않았다"며 남겼으나, 같은 날 [[결정 노트 - 2026-09-26-workflow-image-removal]]에서 사용자가 제거를 결정해 **뒤집혔다**.

## 검증 범위

- Export-Wiki.ps1 재생성(Workflow 59문서), CheckWikiLinks 오류 0건(61문서).
- 자동 테스트 통과: TestWorkflowTestFeedback·TestWorkflowFeedbackUI·TestWikiProviders·TestWikiViewer·TestWikiSpaces.
- 로컬 서버는 재시작하지 않았다. 다음 `OpenWorkflow.bat` 실행 때 `Start-WikiAI.ps1`이 코드 해시 차이로 다시 띄워 새 코드가 적용된다(실제 서버 반영은 이 노트에서 확인하지 않음).

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]

## 핵심 주장

- 사용자 요청(2026-09-26)으로 옛 웹 작업 경로 스크립트 8개와 테스트 5개, 옛 절차 문서 5개와 4단계 그림 ai-workflow.png가 git rm으로 삭제됐다. ^c1
- Claude Code 권한 거부 알림은 없어진 blockers 칸 대신 처리 근거 evidence에 한 줄로 남도록 옮겨졌다. ^c2
- 이 노트가 남겨 둔 문서 PNG 묶기 기능은 같은 날 사용자 결정으로 제거돼 이 노트의 보존 판단은 더 이상 유효하지 않다. ^c3
