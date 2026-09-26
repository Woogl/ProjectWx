---
type: source
title: "결정 노트 - 2026-09-25-workflow-task-records"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "대시보드"
summary: "기존 Workflow 웹 대시보드에 대화 작업 기록을 네 분류로 나눠 확인 범위와 다음 행동을 보여 주는 변경의 구현 관찰과 검증 범위"
source_type: decision-note
source_id: src-d2a4509667c21f7ee0fc
sha256: f57f6f3b997f4f3a1cd3671e73e739e8c83b0bd1d87963131cbff85602c5b035
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-workflow-task-records.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-workflow-task-records.md"
raw_copy: ".raw/captured/f57f6f3b997f4f3a1cd3671e73e739e8c83b0bd1d87963131cbff85602c5b035.md"
claim_ids:
  - clm-c2d424e44d-c1
  - clm-c2d424e44d-c2
  - clm-c2d424e44d-c3
key_claims:
  - "Workflow 작업 기록 현황 변경에서 대화 작업 현황의 표시 원본은 `.agents/workflow/tasks/index.md`의 플레이 확인·에디터 확인·개선 판단·완료 기록 네 표다."
  - "작업 기록 현황 표시는 기존 웹 작업의 JSON 상태·승인·저장 키를 바꾸지 않고 `taskRecordGroups`와 `renderTaskRecords`로 목차를 읽어 표시한다."
  - "작업 기록 현황 변경의 검증은 자동 테스트와 모의 DOM까지이며 실제 브라우저 확인은 로컬 파일 URL 보안 정책으로 수행하지 못했다."
---

# 결정 노트 - 2026-09-25-workflow-task-records

- 원본: `.wiki/raw/notes/2026-09-25-workflow-task-records.md`
- 원자료 사본: `.raw/captured/f57f6f3b997f4f3a1cd3671e73e739e8c83b0bd1d87963131cbff85602c5b035.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 조사 노트 `2026-09-25-workflow-task-records.md`(제목 "Workflow 작업 기록 현황 표시", 출처 `MANUAL`, 수집일 2026-09-25).
- 내용: 사용자가 작업 목차를 파악하기 어렵다고 개선을 요청했고, 기존 Workflow 웹 대시보드에서 대화 작업의 확인 범위와 다음 행동을 분류별로 조회하게 한 변경을 기록한다.
- 기준 HEAD는 `38d4dde08`이며 그 위의 미커밋 구현을 확인했다. 다른 세션의 기존 변경은 보존했다.

## 사람의 판단 원문

사용자는 표시 위치 질문에 다음 선택지를 골랐다.

> 사용자 2026-09-25: "기존 Workflow 웹 화면에서 보기"

## 구현 관찰

- `.agents/workflow/tasks/index.md`의 네 표(플레이 확인·에디터 확인·개선 판단·완료 기록)가 대화 작업 현황의 표시 원본이다. 각 행에 상세 Task 링크·확인된 범위·다음 행동을 둔다.
- `.agents/scripts/wiki-viewer/workflow.js`의 `taskRecordGroups`가 생성 문서에 포함된 목차를 읽고, `renderTaskRecords`가 분류 버튼·건수·상세 기록 링크를 표시한다. 기존 웹 작업의 JSON 상태·승인·저장 키는 바꾸지 않는다.
- 생성된 기록은 AI 서버 연결 전에도 보인다. 웹 작업이 없으면 빈 단계 칸을 만들지 않고, 새 작업 만들기·기존 작업 이어하기 화면에서는 기록 현황을 숨긴다.
- `.agents/workflow/process/index.md`에 단계 완료·인계 시 상세 Task와 목차의 분류·확인 범위·다음 행동을 함께 갱신하도록 적었다. 표시 내용은 기존 후속 기록의 요약이며 현재 코드의 재검증이나 새 승인이 아니다.
- 표시 내용은 `OpenWorkflow.bat`의 기존 내보내기 경로로 갱신하고, 웹 작업의 실행 상태는 기존 공용 작업 API에서 계속 조회한다.
- 입력 식별용으로 `workflow.js`·`index.html`·`tasks/index.md`의 SHA-256을 남겼다.

## 검증 범위

- 자동 테스트: TestWikiViewer·TestWikiSpaces·TestWikiTasks·TestWorkflowExecution 통과.
- 모의 DOM에서 분류 전환·링크·21개 기록 누락/중복·서버 연결 전 표시·승인 상태 불변·잘못된 링크 제외·생성/재개 경로와 기존 저장·실행 회귀를 확인했다.
- 확인하지 않은 것: 실제 브라우저 확인은 로컬 파일 URL 보안 정책으로 차단되어 수행하지 못했다. 게임 실행·에셋·실제 AI 호출 검증은 범위 밖이다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]

## 핵심 주장

- Workflow 작업 기록 현황 변경에서 대화 작업 현황의 표시 원본은 `.agents/workflow/tasks/index.md`의 플레이 확인·에디터 확인·개선 판단·완료 기록 네 표다. ^c1
- 작업 기록 현황 표시는 기존 웹 작업의 JSON 상태·승인·저장 키를 바꾸지 않고 `taskRecordGroups`와 `renderTaskRecords`로 목차를 읽어 표시한다. ^c2
- 작업 기록 현황 변경의 검증은 자동 테스트와 모의 DOM까지이며 실제 브라우저 확인은 로컬 파일 URL 보안 정책으로 수행하지 못했다. ^c3
