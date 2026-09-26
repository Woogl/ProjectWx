---
type: source
title: "결정 노트 - 2026-09-26-workflow-korean-record-names"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "웹 새 작업의 기록 파일 이름을 접수 식별자 해시 대신 한글 제목으로 만들고, 같은 이름은 -2를 붙이며 터미널 이어하기의 영문·숫자 제한을 없앤 노트."
source_type: decision-note
source_id: src-8ab2271b7b6733f7b352
sha256: 461f292d16f7bef8e18a46306db038e6aae0f95a6a3cadb9ba927c35ad67953b
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-korean-record-names.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-korean-record-names.md"
raw_copy: ".raw/captured/461f292d16f7bef8e18a46306db038e6aae0f95a6a3cadb9ba927c35ad67953b.md"
claim_ids:
  - clm-e2728b400a-c1
  - clm-e2728b400a-c2
  - clm-e2728b400a-c3
key_claims:
  - "2026-09-26부터 Workflow 웹 새 작업의 기록 파일 이름은 접수 식별자 해시 대신 NFC 정규화한 한글 제목(최대 60자)으로 만들고 같은 이름이 있으면 -2, -3을 붙인다."
  - "기록 이름에 접수 식별자가 없어진 뒤 같은 접수의 재전송은 처리 이력의 create 요청으로 찾으며, 내용이 다르면 거절한다."
  - "이 변경으로 터미널에서 이어하기의 영문·숫자 기록 이름 제한이 없어졌고 cmd 특수 문자만 막는다."
---

# 결정 노트 - 2026-09-26-workflow-korean-record-names

- 원본: `.wiki/raw/notes/2026-09-26-workflow-korean-record-names.md`
- 원자료 사본: `.raw/captured/461f292d16f7bef8e18a46306db038e6aae0f95a6a3cadb9ba927c35ad67953b.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-korean-record-names.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- **이전 결정을 바꿈**: [[결정 노트 - 2026-09-26-workflow-web-tasks]]의 기록 이름 규칙(제목의 영문·숫자 + 접수 식별자 해시, 한글만 있으면 `task-<해시>.md`)과 터미널 이어하기의 영문·숫자 이름 제한을 대체한다.

## 사람의 판단 원문

> 사용자 2026-09-26: "해시 말고 한글로 추적할 수는 없나요?"

AI가 한글 제목 이름이 사람·AI 모두 제목으로 찾기 쉬워 더 낫다고 답하고, 이미 승인된 웹 새 작업 기능의 세부 사항으로 바로 바꿨다(별도 승인 문장은 원자료에 없음).

## 구현 관찰

- 이름 규칙: 제목을 NFC 정규화 → 글자·숫자(`\p{L}\p{N}`)가 아닌 연속 문자를 `-` 하나로 → 앞뒤 `-` 제거 → 60자 자름. Windows 예약 이름(CON·PRN·AUX·NUL·COM1~9·LPT1~9)은 `-작업`을 붙이고, 남는 글자가 없으면 `작업`. 대소문자는 유지(`Boss HP bar 개선` → `Boss-HP-bar-개선.md`).
- 같은 이름: `이름-2.md`, `이름-3.md` 순서로 빈 이름을 찾아 배타적 생성(`wx`)으로 쓴다. Windows에서는 대소문자만 다른 이름도 같은 이름으로 본다.
- 재전송: 이름에 접수 식별자가 없으므로 처리 이력(`test_feedback_*.json`)의 create 요청에서 같은 접수를 찾는다. 내용이 같으면 그 기록을 돌려주고, 다르면 "같은 접수의 내용이 바뀌었습니다"로 거절한다(서버 재시작 뒤에도 같음).
- 터미널 이어하기: 영문·숫자 이름 제한을 없앴다. cmd 특수 문자(`"`·`%`·`^`·`&`·`|`·`<`·`>`·`!`)는 창을 여는 쪽에서 막고, 새 기록 이름에는 이 문자가 생기지 않는다.
- AI 처리 이력 파일 이름(`test_feedback_<기록 경로 SHA-256>.json`)은 바꾸지 않았다.

## 검증 범위

- 자동 테스트 TestWorkflowTestFeedback: 한글 제목 이름, 같은 제목의 `-2`, 기호·예약·빈 이름, 서버 재시작 뒤 재전송, 작업 목록의 한글 기록 표시, 한글 경로 이어하기 인자.
- 실제 `cmd start` 창에서 `.agents/workflow/tasks/보스-체력바-지연-감소.md`가 든 요청문이 인자 하나로 전달됨을 인자 기록 스크립트로 확인했다(AI 호출 없음).
- 참고: Git은 `core.quotepath` 기본값에서 한글 경로를 8진수 escape로 보여주며, 이 저장소에는 해당 설정이 없다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-26부터 Workflow 웹 새 작업의 기록 파일 이름은 접수 식별자 해시 대신 NFC 정규화한 한글 제목(최대 60자)으로 만들고 같은 이름이 있으면 -2, -3을 붙인다. ^c1
- 기록 이름에 접수 식별자가 없어진 뒤 같은 접수의 재전송은 처리 이력의 create 요청으로 찾으며, 내용이 다르면 거절한다. ^c2
- 이 변경으로 터미널에서 이어하기의 영문·숫자 기록 이름 제한이 없어졌고 cmd 특수 문자만 막는다. ^c3
