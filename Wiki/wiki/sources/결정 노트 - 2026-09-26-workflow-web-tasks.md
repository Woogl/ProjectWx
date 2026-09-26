---
type: source
title: "결정 노트 - 2026-09-26-workflow-web-tasks"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "AI"
summary: "대시보드에서 새 작업 시작과 질문 답변·구현 승인·추가 요청·테스트 결과 전달·터미널 이어하기를 하게 하고, AI 처리를 터미널 창에서 보이게 실행하도록 한 노트."
source_type: decision-note
source_id: src-2a1863fa3b2d8cda086e
sha256: 6c2fab2cbaa84fc91b4b4504af7630f3863d76ffdc4483006a2ea428ba725081
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-web-tasks.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-web-tasks.md"
raw_copy: ".raw/captured/6c2fab2cbaa84fc91b4b4504af7630f3863d76ffdc4483006a2ea428ba725081.md"
claim_ids:
  - clm-bda6487128-c1
  - clm-bda6487128-c2
  - clm-bda6487128-c3
key_claims:
  - "사용자는 2026-09-25 웹에서 새 작업 시작과 기존 작업 이어하기를 요청하고, 웹에서 맡긴 구현에 모든 명령 허용과 터미널 창 표시 실행을 골랐다."
  - "Workflow 웹 처리에서 정하기는 읽기 전용(Codex read-only, Claude Code Read·Glob·Grep만)으로, 구현·추가 요청·테스트 결과 처리는 모든 명령 허용으로 실행된다."
  - "웹 새 작업 기능은 가짜 Codex와 자동 테스트로만 검증했고 실제 AI 요청으로 새 작업을 끝까지 돌리지 않았다."
---

# 결정 노트 - 2026-09-26-workflow-web-tasks

- 원본: `.wiki/raw/notes/2026-09-26-workflow-web-tasks.md`
- 원자료 사본: `.raw/captured/6c2fab2cbaa84fc91b4b4504af7630f3863d76ffdc4483006a2ea428ba725081.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-web-tasks.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`). 사용자 요청 날짜는 2026-09-25다.
- 이 노트의 일부 세부는 같은 날 뒤 노트에서 바뀌었다: 기록 이름 규칙·이어하기 영문·숫자 제한([[결정 노트 - 2026-09-26-workflow-korean-record-names]]), 상태 판정의 blocker 통로([[결정 노트 - 2026-09-26-workflow-state-from-record-only]]), 이어하기 요청문([[결정 노트 - 2026-09-26-workflow-ssot-copies]]).

## 사람의 판단 원문

> 사용자 2026-09-25: "웹페이지에서도 새 작업 시작하거나 기존 작업 이어할 수 있게 합시다"

사용자가 고른 선택지: 제공 방식 = 웹 안에서 처리, AI 대화 여는 방법 = 터미널만, 웹에서 맡긴 구현의 명령 권한 = 모든 명령 허용, 진행 과정 표시 = 터미널 창에 보이며 실행.

AI의 해석(사용자에게 알림): 정하기(새 작업·질문 답변 뒤 조사)는 읽기 전용, 구현 승인·추가 요청·테스트 결과 처리는 권한 확인 없이 모든 명령 허용. AI 처리 창은 끝난 뒤 60초 또는 Enter로 닫힌다. "터미널에서 이어하기"는 평소 권한 확인을 따르는 AI 대화 창을 연다.

## 구현 관찰

- 로컬 서버 경로는 `/test-feedback` 하나(protocol 4). 동작 `list`·`read`·`create`·`answer`·`approve`·`request`·`submit`·`retry`·`terminal`. AI 처리는 한 번에 하나, 같은 접수 식별자는 한 번만 처리한다.
- 작업 기록 형식: 제목 아래 상태·다음 행동 두 줄 → `## 요청` · `## 질문` · `## 구현 계획` · `## 테스트 체크리스트` → 이력. 질문 표 `| ID | 질문 | 선택지 | 추천 | 답변 |`, 답변 칸이 비면 미답변. `구현 승인: 이름 날짜` 줄이 승인. 공용 해석기는 `wiki-viewer/task-records.js`.
- 서버 `Workflow-TestFeedback.cjs`:
  - (이 노트 시점) create는 제목의 영문·숫자와 접수 해시로 이름을 만들었다(`boss-hp-bar-1a2b3c4d.md`, 한글만 있으면 `task-<해시>.md`). 뒤에 한글 제목 이름으로 바뀜.
  - 처리 중 상태 줄 `진행 중 · AI 조사 중/구현 중/…`, 결과 없이 끝나면 `확인 대기 · AI 처리 실패`, 서버 재시작으로 끊기면 `확인 대기 · AI 처리 중단`.
  - (이 노트 시점) 상태 순서는 막힘(blocker·실패) → 미답변 질문 → 미승인 계획 → 전부 통과(완료) → 나머지.
  - AI 계획의 `#` 줄과 `구현 승인:` 줄은 목록 항목으로 바꿔 절 구조를 깨거나 AI가 스스로 승인할 수 없다.
  - 처리 중 기록이 바뀌면 결과를 반영하지 않고 `확인 대기 · 기록 변경 확인 필요`.
- 실행 방식: 서버가 `Saved/Wiki/jobs/<접수>-<시도>/`에 `job.json`·`prompt.txt`·`schema.json`을 만들고 `cmd.exe /d /c start ...`로 `Workflow-Runner.cjs` 새 창을 연다. 실행기가 `result.json`을 원자적으로 남기고, 서버는 1초마다 확인하며 창이 결과 없이 닫히거나 60초 안에 시작하지 않거나 35분이 지나면 실패 처리한다. 인자에 cmd 특수 문자·줄바꿈이 있으면 창을 열지 않는다.
- 권한 인자(`Wiki-AI-Providers.cjs`, plan / work):
  - Codex: `exec --sandbox read-only` / `--sandbox danger-full-access`
  - Claude Code: `--tools Read,Glob,Grep --allowedTools Read,Glob,Grep --permission-mode dontAsk` / `--permission-mode bypassPermissions`
  - Gemini CLI: `--approval-mode default`에서 쓰기·셸 도구를 뺀 목록 / `--approval-mode yolo`
- 진행 표시: Codex는 출력 그대로, Claude Code는 `--output-format stream-json --verbose`로 AI 문장과 도구 호출을 한 줄씩, Gemini CLI는 끝난 결과만.
- 터미널 이어하기: 고정 영어 요청문 하나로 AI를 연다(Gemini는 `-i`). 사람 입력 글은 넣지 않고, AI가 처리 중인 기록은 열지 않는다.
- 화면: 대시보드 머리에 새 작업 버튼, 기록마다 작업 진행 버튼. 작업 진행은 지금 할 일 하나(질문 카드 → 구현 계획·승인 → 체크리스트)와 추가 요청·터미널 이어하기·최신 상태 불러오기를 보여준다. 응답을 못 받은 전달은 같은 접수로 다시 보내 기록이 중복 생성되지 않는다.

## 검증 범위

- 자동 테스트: TestWorkflowTestFeedback(새 작업→질문→답변→계획→승인→구현→추가 요청 흐름 등, 실제 실행기와 **가짜 Codex**로 read-only/danger-full-access 전달), TestWorkflowFeedbackUI, TestWikiProviders, TestWikiViewer·TestWikiSpaces 통과.
- 실제 창: 가짜 Codex와 인자 기록 스크립트로 `cmd start` 창을 열어 실행기 창이 보이고(visible=True) 요청문이 인자 하나로 전달됨을 확인했다.
- CLI 확인: Claude Code 2.1.282를 연결 불가 API 주소로 실행해 초기화 이벤트만 보고 `--permission-mode bypassPermissions`만으로 우회 모드가 됨을 확인했다. Codex `--sandbox` 값과 Gemini CLI 0.36 옵션은 확인만 했다.
- 헤드리스 Edge로 새 작업 폼과 작업 진행 패널을 1440px·420px로 캡처했다(질문·승인 단계는 화면 상태만 바꿔 표시).
- **실제 AI 요청으로 새 작업을 끝까지 돌리지 않았다.** 사람의 실제 웹 흐름 확인은 이후 [[결정 노트 - 2026-09-26-workflow-human-verification]]에 있다.

## 미결정·충돌

- 주의: `Start-WikiAI.ps1`을 Git Bash에서 실행하면 새 서버가 셸 파이프를 물려받아 명령이 끝나지 않는다(서버는 정상 기동). 헤드리스 Edge 부모만 종료하면 브라우저가 남아 옛 페이지를 잡을 수 있다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]

## 핵심 주장

- 사용자는 2026-09-25 웹에서 새 작업 시작과 기존 작업 이어하기를 요청하고, 웹에서 맡긴 구현에 모든 명령 허용과 터미널 창 표시 실행을 골랐다. ^c1
- Workflow 웹 처리에서 정하기는 읽기 전용(Codex read-only, Claude Code Read·Glob·Grep만)으로, 구현·추가 요청·테스트 결과 처리는 모든 명령 허용으로 실행된다. ^c2
- 웹 새 작업 기능은 가짜 Codex와 자동 테스트로만 검증했고 실제 AI 요청으로 새 작업을 끝까지 돌리지 않았다. ^c3
