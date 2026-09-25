---
title: "Workflow 웹 새 작업·이어하기와 터미널 창 실행"
source: "MANUAL"
type: notes
ingested: 2026-09-26
tags: [wx, workflow]
summary: "대시보드에서 새 작업을 시작하고 질문 답변·구현 승인·추가 요청·테스트 결과 전달·터미널 이어하기로 기존 작업을 잇는다. 정하기는 읽기 전용, 구현·수정은 사용자 결정으로 모든 명령 허용이며 모든 AI 처리는 터미널 창에서 보이며 실행된다."
---

# 사용자 요청과 결정

2026-09-25 사용자는 "웹페이지에서도 새 작업 시작하거나 기존 작업 이어할 수 있게 합시다"라고 요청했다. 선택지 중 다음을 골랐다.

- 제공 방식: 웹 안에서 처리
- AI 대화 여는 방법: 터미널만
- 웹에서 맡긴 구현의 명령 권한: 모든 명령 허용
- 진행 과정 표시: 터미널 창에 보이며 실행

AI의 해석(사용자에게 알림): 정하기(새 작업·질문 답변 뒤 조사)는 읽기 전용으로 실행한다. 구현 승인·추가 요청·테스트 결과 처리는 권한 확인 없이 모든 명령을 허용한다. 모든 AI 처리는 터미널 창에서 보이며 실행되고 끝난 뒤 60초 또는 Enter로 닫힌다. "터미널에서 이어하기"는 평소 권한 확인을 따르는 AI 대화 창을 연다.

# 구현 관찰

- 로컬 서버 경로는 그대로 `/test-feedback` 하나다(protocol 4). 동작은 `list`·`read`·`create`·`answer`·`approve`·`request`·`submit`·`retry`·`terminal`이다. AI 처리는 한 번에 하나이고, 같은 접수 식별자는 한 번만 처리한다.
- 작업 기록 형식: 제목 아래 상태·다음 행동 두 줄, 이어서 `## 요청` · `## 질문` · `## 구현 계획` · `## 테스트 체크리스트` 절을 이 순서로 두고 이력은 아래에 쌓는다. 질문 표는 `| ID | 질문 | 선택지 | 추천 | 답변 |`이고 선택지는 ` / `로 나누며 답변 칸이 비면 미답변이다. 구현 계획 절의 `구현 승인: 이름 날짜` 줄이 승인이다. 공용 해석기 `wiki-viewer/task-records.js`가 요청·질문·계획을 읽는다.
- 서버 `Workflow-TestFeedback.cjs`:
  - create는 제목의 영문·숫자와 접수 식별자 해시로 기록 이름을 만든다(`boss-hp-bar-1a2b3c4d.md`, 한글만 있으면 `task-<해시>.md`). 요청 원문은 인용(`> `)으로 넣어 제목으로 읽히지 않는다.
  - AI 처리를 시작하면 상태 줄을 `진행 중 · AI 조사 중/구현 중/추가 요청 처리 중/테스트 결과 처리 중`으로 바꾸고, 결과 없이 끝나면 `확인 대기 · AI 처리 실패`, 서버 재시작으로 끊기면 `확인 대기 · AI 처리 중단`으로 바꾼다.
  - 결과 반영 순서: 체크리스트(사람 항목 보호) → 질문 추가(겹치는 ID는 새 번호) → 새 계획(승인 줄 없음) → 상태 결정. 상태는 막힘(blocker·실패) → 미답변 질문 → 미승인 계획 → 체크리스트 전부 통과(완료) → 나머지(다시 확인) 순이다.
  - AI가 준 계획에서 `#`로 시작하는 줄과 `구현 승인:` 줄은 목록 항목으로 바꿔, 절 구조를 깨거나 AI가 스스로 승인할 수 없다.
  - 처리 중 기록이 바뀌면 결과를 반영하지 않고 `확인 대기 · 기록 변경 확인 필요`로 둔다.
  - 이력 재전송 표식(`<!-- test-feedback:접수:시도 -->`)은 이력 제목 아래에 둔다. 표준 절을 새로 넣을 때 이력 제목 앞에 들어가므로 표식이 다른 절로 떨어지지 않는다.
- 실행 방식: 서버가 `Saved/Wiki/jobs/<접수>-<시도>/`에 `job.json`·`prompt.txt`·`schema.json`을 만들고 `cmd.exe /d /c start "제목" /D <저장소> <node> Workflow-Runner.cjs <작업 폴더>`로 새 창을 연다. 실행기가 `pid.txt`를 쓰고 AI를 실행해 `result.json`을 원자적으로 남긴다. 서버는 1초마다 확인하고, 결과 없이 창이 닫히거나 60초 안에 실행기가 시작하지 않거나 35분이 지나면 실패로 처리한 뒤 작업 폴더를 지운다. 인자에 `"`·`%`·`^`·`&`·`|`·`<`·`>`·`!`·줄바꿈이 있으면 창을 열지 않는다.
- 권한 인자(`Wiki-AI-Providers.cjs`의 plan/work):
  - Codex: `exec --sandbox read-only` / `--sandbox danger-full-access`
  - Claude Code: `--tools Read,Glob,Grep --allowedTools Read,Glob,Grep --permission-mode dontAsk` / `--permission-mode bypassPermissions`
  - Gemini CLI: `--approval-mode default`에 쓰기·셸 도구(`replace`·`write_file`·`run_shell_command`)를 뺀 도구 목록 / `--approval-mode yolo`
- 진행 표시: Codex는 출력을 창에 그대로 보여주고 결과는 `--output-last-message` 파일에서 읽는다. Claude Code는 `--output-format stream-json --verbose`로 실행해 AI 문장과 도구 호출(`→ Read 경로`, `→ Bash 명령`)을 한 줄씩 보여주고 마지막 `result` 이벤트의 `structured_output`을 응답으로 쓴다. Gemini CLI는 끝난 결과만 보여준다.
- 터미널에서 이어하기: 고른 AI를 고정 영어 요청문 하나로 연다("Continue the Wx task recorded in <경로>. Follow AGENTS.md and .agents/workflow/process/index.md, read the record head, request, questions, plan and test checklist first, and reply in Korean."). Gemini는 `-i`를 붙인다. 사람이 입력한 글은 넣지 않고, 영문·숫자 이름이 아닌 기록과 AI가 처리 중인 기록은 열지 않는다.
- 화면: 대시보드 머리에 새 작업 버튼, 기록마다 작업 진행 버튼이 있다. 작업 진행은 지금 할 일 하나(질문 카드 → 구현 계획과 구현 승인 → 테스트 체크리스트)와 AI에게 추가 요청·터미널에서 이어하기·최신 상태 불러오기, 처리 중 경과 분을 보여준다. 이름은 작업마다 다시 적지 않도록 하나만 기억한다. 응답을 받지 못한 전달은 같은 접수로 다시 보내 새 기록이 두 번 만들어지지 않는다.

# 검증

- 자동화: TestWorkflowTestFeedback(새 작업→질문→답변→계획→승인→구현→추가 요청 흐름, 절 순서, 실패·중단 상태 줄, 실제 실행기와 가짜 Codex로 read-only/danger-full-access 전달), TestWorkflowFeedbackUI(단계별 패널·새 작업 재전송), TestWikiProviders(권한 인자·Claude 진행 이벤트 분할 수신), TestWikiViewer·TestWikiSpaces 통과.
- 실제 창: 가짜 Codex와 인자 기록 스크립트로 `cmd start` 창을 열었다. 서버 쪽이 숨김 옵션으로 cmd를 띄워도 실행기 창은 보였다(`Wx AI · 창 확인`, visible=True). 이어하기 요청문은 인자 하나로 전달됐고 작업 폴더(`/D`)에서 실행됐다.
- Claude Code 2.1.282를 연결할 수 없는 API 주소로 실행해 초기화 이벤트만 확인했다: `--permission-mode bypassPermissions`만으로 `permissionMode: bypassPermissions`가 됐다(`--allow-dangerously-skip-permissions` 불필요). Codex `exec --sandbox` 값은 read-only·workspace-write·danger-full-access다. Gemini CLI 0.36은 `--approval-mode plan`과 `--output-format stream-json`도 제공한다(이번에는 쓰지 않음).
- 헤드리스 Edge로 새 작업 폼과 작업 진행 패널(질문·승인 단계는 화면 상태만 바꿔 표시)을 1440px·420px로 캡처했다. 캡처 중 서버 요청은 list·read뿐이었다.

# 제한과 주의

- 실제 AI 요청으로 새 작업을 끝까지 돌리지 않았다.
- `Start-WikiAI.ps1`을 Git Bash에서 실행하면 새 서버가 셸 파이프를 물려받아 명령이 끝나지 않는다(서버는 정상 기동). 헤드리스 Edge의 부모만 종료하면 브라우저가 남아 같은 디버깅 포트의 옛 페이지를 잡을 수 있다.
