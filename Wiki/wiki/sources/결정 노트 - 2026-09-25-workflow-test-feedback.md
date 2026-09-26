---
type: source
title: "결정 노트 - 2026-09-25-workflow-test-feedback"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "테스트"
summary: "기존 Workflow 작업에 사람의 테스트 결과(이상 없음/이상 있음)를 접수해 AI가 수정·정리·재확인을 이어가는 경로의 구현 관찰과 검증 범위"
source_type: decision-note
source_id: src-0829caca7b781c4aac5c
sha256: d38cc221dea02acfc8e7718b778326c77a91e0e77a5ef5007f83f7e73e7cb4f2
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-workflow-test-feedback.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-workflow-test-feedback.md"
raw_copy: ".raw/captured/d38cc221dea02acfc8e7718b778326c77a91e0e77a5ef5007f83f7e73e7cb4f2.md"
claim_ids:
  - clm-d65a7c607a-c1
  - clm-d65a7c607a-c2
  - clm-d65a7c607a-c3
  - clm-d65a7c607a-c4
key_claims:
  - "Workflow 테스트 결과 접수 경로는 이상 있음을 고른 경우에만 문제 상황·재현 방법 입력을 요구한다."
  - "테스트 결과 접수 후 작업은 AI 결과가 통과이고 코드·Task 변경과 남은 확인이 없을 때만 완료로 처리된다."
  - "테스트 결과 접수 경로는 부분 테스트 결과를 작업 전체 수용이나 코드 리뷰 승인으로 확대하지 않도록 AI에 지시한다."
  - "테스트 결과 접수 기능은 모의 AI와 모의 DOM으로 검증했으며 실제 AI 결과 제출·브라우저 육안·UE 실행은 수행하지 않았다."
---

# 결정 노트 - 2026-09-25-workflow-test-feedback

- 원본: `.wiki/raw/notes/2026-09-25-workflow-test-feedback.md`
- 원자료 사본: `.raw/captured/d38cc221dea02acfc8e7718b778326c77a91e0e77a5ef5007f83f7e73e7cb4f2.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 조사 노트 `2026-09-25-workflow-test-feedback.md`(제목 "Workflow 작업 테스트 결과 접수", 출처 `MANUAL`, 수집일 2026-09-25).
- 내용: 기존 작업에서 사람의 테스트 결과를 AI에게 전달하고 수정·정리·재확인을 이어가는 경로. 구현 기준 HEAD는 `38d4dde08`이며 미커밋 작업 트리를 포함한다.

## 사용자 요청

원자료는 요청을 요약 서술로만 남겼다(직접 인용 문장 없음). 사용자는 기존 Workflow 웹 대시보드에서 작업을 이어가길 원했고, 테스트 결과를 이상 없음/이상 있음으로 선택하고 문제가 있으면 내용을 기록해 AI가 접수·마무리하는 기능을 요청했다.

## 구현 관찰

- 입력: 작업별로 확인자·직접 확인한 항목·상호 배타적인 두 결과를 둔다. 이상 있음일 때만 문제 상황·재현 방법 입력이 나오며 필수다.
- 저장: `/test-feedback`의 list/read/submit/retry가 접수와 조회를 처리한다. `test_feedback_<task path hash>.json`에 원문·확인자·시각·작업 해시·코드 식별값·결과·재시도 이력을 보존하고, operationId와 요청 본문 해시로 재전송 중복 실행을 막는다.
- AI 처리: Codex CLI의 기존 workspace-write 실행 경로에서 원래 Task의 최신 결정과 남은 일을 읽는다. 이상 없음은 수용 범위 대조·지식 정리, 이상 있음은 합의 범위 내 수정·검증이다. 기존 기획·설계·리뷰 승인 파일은 만들지 않는다.
- 상태 판정: AI 결과가 통과이고 코드·Task 변경과 남은 확인이 없을 때만 완료한다. 문제 보고·코드 변경·미실행·사람 확인이 남으면 재확인, 실패 검사·미해결 판단은 확인 필요로 남긴다. 부분 테스트를 작업 전체 수용·코드 리뷰 승인으로 확대하지 않도록 지시한다.
- 기록: 서버가 원래 Task에 사람 결과와 AI 보고를 추가하고 목차를 갱신한다. 완료 기록에 새 문제가 생기면 확인할 일로 돌아간다. 입력 초안과 응답 유실 요청은 브라우저에 보존하고 실패·중단은 저장된 결과로 재시도한다. 접수 이후 코드가 달라지면 새 결과가 필요하다.
- 코드 식별은 이 경로에서 Markdown·Wiki 정리를 제외하되 HEAD 변경은 감지한다. 두 실행 경로와 기존 분석은 로컬 서버에서 동시에 시작되지 않도록 잠근다.
- 기록 열기의 상세 본문은 생성 HTML이므로 `OpenWorkflow.bat` 재실행 후 추가 기록이 보인다.
- 입력 식별: `Workflow-TestFeedback.cjs`·`wiki-viewer/test-feedback.js`·`Wiki-AI.cjs`·`Workflow-Execution.cjs`의 SHA-256을 남겼다.

## 검증 범위

- 자동 테스트: TestWorkflowTestFeedback·TestWorkflowFeedbackUI와 기존 TestWikiViewer·TestWikiSpaces·TestWikiTasks·TestWikiRecovery·TestWikiAI·TestWorkflowExecution 통과.
- 임시 저장소·모의 AI 실행기·실제 로컬 HTTP·모의 DOM으로 입력 검증, 응답 유실, 재시도·재시작, 코드/Task 변경, 처리 분류, 원문 보존, 목차 이동, 실행 잠금, 토큰/Origin, 화면 갱신을 확인했다.
- 확인하지 않은 것: 실제 AI 결과 제출·브라우저 육안·UE 실행. 브라우저 자동화 확인은 로컬 파일 URL 보안 정책으로 제한된다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- Workflow 테스트 결과 접수 경로는 이상 있음을 고른 경우에만 문제 상황·재현 방법 입력을 요구한다. ^c1
- 테스트 결과 접수 후 작업은 AI 결과가 통과이고 코드·Task 변경과 남은 확인이 없을 때만 완료로 처리된다. ^c2
- 테스트 결과 접수 경로는 부분 테스트 결과를 작업 전체 수용이나 코드 리뷰 승인으로 확대하지 않도록 AI에 지시한다. ^c3
- 테스트 결과 접수 기능은 모의 AI와 모의 DOM으로 검증했으며 실제 AI 결과 제출·브라우저 육안·UE 실행은 수행하지 않았다. ^c4
