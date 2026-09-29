---
type: source
title: "작업 - workflow-codex-terminal-no-daemon"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
  - "Codex"
summary: "대시보드의 Codex 터미널 이어하기 창이 바로 닫히던 문제를 대화형 실행에만 --no-daemon을 붙여 고친 2026-09-30 완료 작업 기록"
source_type: task-record
source_id: src-f1c1ee3a4db1d8361cff
sha256: 7d1f6c5b48808aab0d8f9d124d8c076074e3a3c7d84e22986eb2f1a9f469eeb5
authority: primary
independence_key: ".agents/workflow/tasks/workflow-codex-terminal-no-daemon.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/workflow-codex-terminal-no-daemon.md"
raw_copy: ".raw/captured/7d1f6c5b48808aab0d8f9d124d8c076074e3a3c7d84e22986eb2f1a9f469eeb5.md"
claim_ids:
  - clm-7d1f6c5b48-c1
  - clm-7d1f6c5b48-c2
key_claims:
  - "2026-09-30 원인 조사에서 데스크톱 앱에 딸린 codex.exe는 대화형 실행 때 --no-daemon 없이 바로 종료되어 대시보드 터미널 이어하기 창이 닫혔고, codex exec 자동 처리는 영향이 없었다."
  - "수정 뒤 Workflow-TestFeedback.cjs의 openSession은 Codex일 때만 --no-daemon을 붙이며, 실제 서버 경유 이어하기와 woogle의 2026-09-29 코드 리뷰·화면 확인이 통과해 체크리스트 6/6으로 완료됐다."
---

# 작업 - workflow-codex-terminal-no-daemon

- 원본: `.agents/workflow/tasks/workflow-codex-terminal-no-daemon.md`
- 원자료 사본: `.raw/captured/7d1f6c5b48808aab0d8f9d124d8c076074e3a3c7d84e22986eb2f1a9f469eeb5.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

사용자가 Codex 터미널에서 이어하기가 안 된다고 알린 뒤 원인 보고에 "네. 고치세요"로 승인한 작업 기록이다. 상태는 완료(체크리스트 6/6 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`를 본다.

## 원인과 수정

- 대시보드가 쓰는 Codex는 데스크톱 앱에 딸린 codex.exe이고, 대화형으로 켜면 "this CLI has no complete local package … rerun the same command with --no-daemon" 오류로 바로 끝나 창이 닫혔다. AI 자동 처리(`codex exec`)는 영향이 없었다.
- `Workflow-TestFeedback.cjs`의 openSession에서 Codex일 때만 `--no-daemon`을 붙이고, 기존 openSession 테스트에 Codex 인자 검증을 더했다.

## 검증 범위

- AI: 실제 콘솔 창에서 인자 없이는 오류로 끝나고 `--no-daemon`이면 Codex 화면까지 뜨는 것을 확인(사용자가 창 화면을 보내 확인), `TestWorkflowTestFeedback.cjs` 종료 코드 0, 서버를 재시작해 실제 이어하기 요청으로 `codex.exe --no-daemon` 프로세스가 10초 뒤에도 살아 있음. 첫 시도 한 번은 프로세스가 보이지 않았고 원인은 확인하지 못했다.
- 이어하기로 시작된 대화에서 기록을 읽고 테스트를 다시 돌렸다.
- 사람: woogle 2026-09-29 코드 리뷰, 대시보드·터미널 화면 통과.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-30 원인 조사에서 데스크톱 앱에 딸린 codex.exe는 대화형 실행 때 --no-daemon 없이 바로 종료되어 대시보드 터미널 이어하기 창이 닫혔고, codex exec 자동 처리는 영향이 없었다. ^c1
- 수정 뒤 Workflow-TestFeedback.cjs의 openSession은 Codex일 때만 --no-daemon을 붙이며, 실제 서버 경유 이어하기와 woogle의 2026-09-29 코드 리뷰·화면 확인이 통과해 체크리스트 6/6으로 완료됐다. ^c2
