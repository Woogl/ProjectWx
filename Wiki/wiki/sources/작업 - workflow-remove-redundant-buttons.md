---
type: source
title: "작업 - workflow-remove-redundant-buttons"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
  - "대시보드"
summary: "작업 탭의 최신 상태 불러오기·대시보드로 버튼을 없애고 브라우저 새로고침과 왼쪽 대시보드 메뉴로 대신한 2026-09-30 완료 작업 기록"
source_type: task-record
source_id: src-e5fb7ec6d60d68028ff6
sha256: a619da7ca7460f86a2097ea36d8f7cf2ff1e2c188001ff2993a93145eb1c959f
authority: primary
independence_key: ".agents/workflow/tasks/workflow-remove-redundant-buttons.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/workflow-remove-redundant-buttons.md"
raw_copy: ".raw/captured/a619da7ca7460f86a2097ea36d8f7cf2ff1e2c188001ff2993a93145eb1c959f.md"
claim_ids:
  - clm-a619da7ca7-c1
key_claims:
  - "2026-09-30 사용자 지시로 워크플로우 작업·새 작업·빈 작업 화면의 최신 상태 불러오기·대시보드로 버튼을 없애고 안내를 브라우저 새로고침과 왼쪽 대시보드 메뉴 기준으로 바꿨으며, woogle의 2026-09-29 확인으로 체크리스트 2/2 완료됐다."
---

# 작업 - workflow-remove-redundant-buttons

- 원본: `.agents/workflow/tasks/workflow-remove-redundant-buttons.md`
- 원자료 사본: `.raw/captured/a619da7ca7460f86a2097ea36d8f7cf2ff1e2c188001ff2993a93145eb1c959f.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

사용자 지시 "그럼 이 두가지 버튼 제거합시다."(2026-09-30)로 작업·새 작업·빈 작업 화면의 최신 상태 불러오기·대시보드로 버튼을 없앤 작업 기록이다. 상태는 완료(체크리스트 2/2 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`를 본다.

## 변경과 검증

- 두 버튼을 없애고 안내를 브라우저 새로고침과 왼쪽 대시보드 메뉴 기준으로 고쳤다(`test-feedback.js`와 UI 검증).
- AI: `TestWorkflowFeedbackUI.cjs`·`TestWorkflowPage.cjs` 재실행 exit 0(버튼 부재·기존 입력 보존·갱신·대시보드 경로), `git diff --check`.
- 확인 대기 중 사용자가 전달 버튼이 안 보인다고 알렸고, 원인은 질문 절에 표 없이 "추가 질문 없음."만 있어 서버가 질문 표 머리글 오류로 읽은 것이었다. 기록에 빈 질문 표를 넣어 고쳤고 코드 변경은 없었다. 이 문제의 재발 방지는 [[작업 - workflow-table-error-recurrence]]에서 다뤘다.
- 사람: woogle 2026-09-29 코드 리뷰·화면 확인 통과.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-30 사용자 지시로 워크플로우 작업·새 작업·빈 작업 화면의 최신 상태 불러오기·대시보드로 버튼을 없애고 안내를 브라우저 새로고침과 왼쪽 대시보드 메뉴 기준으로 바꿨으며, woogle의 2026-09-29 확인으로 체크리스트 2/2 완료됐다. ^c1
