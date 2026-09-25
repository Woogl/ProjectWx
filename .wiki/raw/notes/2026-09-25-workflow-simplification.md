---
title: "Workflow 3단계 단순화와 테스트 체크리스트"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, workflow]
summary: "사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지했다. 정하기는 추가 질문이 없을 때 사람의 구현 승인으로 끝나고, 확인하기는 AI·사람 담당을 나눈 테스트 체크리스트로 진행한다."
---

# 사용자 요청과 결정

2026-09-25 사용자는 워크플로우 도식 설명을 받은 뒤 "좀 복잡하다", "한눈에 파악되는 직관적인 워크플로우를 원한다"고 했다. AI가 제시한 다섯 제안(웹 새 작업 경로 폐지, 사람 판단 지점 축소, 작은 수정의 진입 기준, 상태 3개, 규칙 한 장)에 "좋네요. 워크플로우를 직관적이고 단순하게 해주세요. 그래야 문제가 있을 때 해결도 쉽죠."라고 답했다.

같은 작업 중 추가 요청:

- 정하기 → 만들기: AI가 구현에 필요한 질문을 꼼꼼하게 하고, 더 이상 추가 질문이 없을 때 사람이 구현을 승인해야 구현 단계로 넘어간다.
- 만들기 → 확인하기: AI가 테스트 체크리스트를 만들고, AI가 스스로 테스트할 수 있는 부분은 직접 테스트해 체크리스트에 반영한다. AI가 테스트할 수 없는 부분은 사람이 직접 테스트한 뒤 체크리스트에 반영할 수 있게 제공한다.

# 판단 근거

- 웹 새 작업 경로의 저장 파일(`.agents/workflow/tasks/workflow_*`)은 0건이었다. 작업 기록 21건은 모두 대화로 진행한 작업이었다.
- 이상 없음/이상 있음 처리 규칙이 사용 방법 안내·테스트·완료 절차와 Wiki 도구 구조 네 문서에 중복됐다.
- 사람의 확정 지점이 다섯 곳(검토본·설계·코드 리뷰·테스트 수용·정리 완료)이었고 상태 용어가 일곱 가지 이상이었다.
- 2026-09-18 이후 커밋 204개 중 53개가 `.agents`·`.wiki`·AGENTS.md만 바꿨다.

# 구현 관찰

- 절차: `.agents/workflow/process/index.md` 한 장에 도식(Mermaid)·단계표·테스트 체크리스트 형식·상태·문제 처리·결과 전달·기록 규칙을 둔다. 사람 판단은 정하기의 답변과 구현 승인, 만들기의 코드 리뷰, 확인하기의 사람 항목 테스트다. 구현 내용을 명확히 지시한 요청은 구현 승인으로 보며, 이미 승인된 범위 안의 수정은 만들기부터 시작한다. AGENTS.md가 같은 규칙을 요약하고 module-review 스킬의 절차 링크를 이 문서로 바꿨다.
- 상태: 작업 현황 `.agents/workflow/tasks/index.md`는 확인 대기·진행 중·완료 세 표다. 기존 플레이 확인·에디터 확인·개선 판단 행은 확인 대기로 옮겼다.
- 테스트 체크리스트: 작업 기록의 `## 테스트 체크리스트` 절에 `| 항목 | 확인 방법 | 담당 | 결과 | 근거 |` 표를 둔다. 담당은 AI·사람, 결과는 대기·통과·실패·미실행이다. `readChecklist`가 머리글·행 형식을 엄격히 검사하고 `writeChecklist`가 셀의 `|`와 줄바꿈을 지운다.
- 제출: 대시보드의 테스트 체크리스트 창이 사람 항목(이번에 확인 안 함·통과·실패, 실패 시 문제 상황 필수)을 먼저 보여주고 AI 항목은 결과만 표시한다. 서버는 담당이 사람인 행만 받고 결과를 제출 즉시 작업 기록 표에 쓴 뒤(근거: 확인자·날짜·메모) AI 처리를 시작한다. 체크리스트가 없는 기록은 작업 현황의 다음 행동을 사람 항목 하나로 쓰고 첫 제출 때 표를 만든다.
- AI 처리: 보고 스키마에 `checklist`(전체 체크리스트)가 필수이고 `humanChecks`는 없어졌다. 서버는 AI가 사람 항목을 통과로 바꾸거나 지우거나 담당을 AI로 바꾸면 되돌린다(`guardChecklist`). 차단 사항·실패 검사·실패 행이 있으면 확인 필요(blocked), 코드 변경·검사 없음·미실행·통과 아닌 행이 있으면 재확인(retest), 모두 통과면 완료다. 목차 행은 "체크리스트 n/m 통과"와 첫 미통과 항목으로 갱신되고 완료 또는 확인 대기 표 맨 위로 옮겨진다.
- 코드 식별: `codeVersion`은 HEAD와 작업 기록·`.wiki`·Markdown을 제외한 diff·미추적 파일을 해시한다(이전의 문서 포함 변형 제거).
- 로컬 서버: `Wiki-AI.cjs`는 `/health`와 `/test-feedback`만 받는다(protocol 4, 재시작 판단 파일 5개). 기획·설계 분석(`/analyze`), 확정 인계(`/handoff`·`/revoke`), 변경·제목 변경(`/change`·`/rename`), 웹 작업 저장(`/tasks`·`/task`), 실행 서비스(`/execution`), 기획서 첨부 변환(`/import`), `--current` 조회를 없앴다. 연결 파일은 `{token,url}`만 담는다.
- AI 실행: `Wiki-AI-Providers.cjs`는 한 가지 권한(Codex workspace-write, Claude acceptEdits, Gemini auto_edit)만 쓰고 Gemini 설정 파일에 쓰기 도구를 포함한다. 검토용 읽기 전용 모드와 정책 파일은 쓰지 않는다.
- 화면: 메뉴는 작업 현황 대시보드·작업 절차·LLM 위키 검색 세 개다. 새 작업 만들기·기존 작업 이어하기·단계별 웹 작업 칸·AI 검토 대기 화면을 없앴다.
- 삭제 대기: 자동 모드 권한 검사가 파일 삭제를 거부해 웹 경로 전용 파일(`Wiki-Tasks.cjs`, `Workflow-Execution.cjs`, `Wiki-Import.cjs`, `Wiki-Import.py`, `wiki-checklist.schema.json`, `wiki-gemini-policy.toml`, `wiki-viewer/workflow-model.js`, `wiki-viewer/execution.js`)과 그 테스트 5개, 옛 절차 문서 4개·사용 방법 안내·4단계 그림이 남아 있다. 남은 코드는 이 파일들을 참조하지 않는다.

# 입력 식별

기준 HEAD `c9e2efec6`에 미커밋 작업 트리를 포함한다.

| 파일 | SHA-256 |
| --- | --- |
| `AGENTS.md` | `EA865F19F44AF50F181B70C14D39C62E72A2B1738BE21DDEB63FA42F314D0352` |
| `.agents/workflow/process/index.md` | `43283F0B8F57724A83F7C6012D12020EBE5D71ACA9171DA3D23125740C82F078` |
| `.agents/scripts/Wiki-AI.cjs` | `7434E38FF8CF4F05390086BD8FA067D850096337B50FEF6FFC27F8CF8B90A225` |
| `.agents/scripts/Workflow-TestFeedback.cjs` | `3C063A80E998501A86D4B938FF192A554290827EA724C7D07815680771B7BB21` |
| `.agents/scripts/Wiki-AI-Providers.cjs` | `0FC27D48E92BAF88C20F67790E086D351D74ED87E47B30DD7F3AA4AD054C006A` |
| `.agents/scripts/wiki-gemini-settings.json` | `AEBDDE32E42E1BFAB4640AEDA89954DD5FA53D44D34A8F8FFB660DCECE8B1A46` |
| `.agents/scripts/Start-WikiAI.ps1` | `F115E06FF10D810666BC447E948F4B0A1239A64F49EA6F2C3CD57FCF58594628` |
| `.agents/scripts/Export-Wiki.ps1` | `73D9DC5C7A6B5C0A2EC67190C897226D7AB9C474326596C074C9325CCBBB048E` |
| `.agents/scripts/wiki-viewer/index.html` | `F76E729AEEBAEF2F7D0929276DEC8300756A3D37BAF6FF15EC6F6AEAD0FDC51E` |
| `.agents/scripts/wiki-viewer/workflow.js` | `18EE8E986DDA1FDC280BA196A63C9F8982A267831ADB48D4430351FDD5314778` |
| `.agents/scripts/wiki-viewer/test-feedback.js` | `046100A398021DD13B14A5690946944EEF052FE93A5746BE896C91DA075D2C88` |

# 검증과 제한

TestWorkflowTestFeedback(체크리스트 형식·제출 즉시 반영·AI 체크리스트 가드·상태 분류·목차 이동·재시도·재시작·HTTP 경계), TestWorkflowFeedbackUI(사람 항목 선택·실패 메모 필수·초안 보존·응답 유실 재전송·실행 잠금·재시도), TestWikiProviders, TestWikiViewer·TestWikiSpaces(생성 화면 재생성 후)가 통과했다. CheckWikiLinks는 오류 0건이다. Start-WikiAI.ps1로 로컬 서버를 재시작해 `/health`의 protocol 4를 확인했다. 헤드리스 Edge에서 작업 절차 도식, 대시보드 세 분류, 한 작업의 테스트 체크리스트 창(읽기 요청만)을 캡처해 표시를 확인했다.

실제 AI 처리 요청과 사람의 화면 확인은 하지 않았다. 앱 브라우저 창은 로컬 파일 열기를 거절했다. 삭제 대기 테스트 5개는 제거된 함수를 불러 실패하므로 실행하지 않았다. 작업 중 다른 세션이 같은 체크아웃의 C++·Wiki·작업 현황을 수정·커밋했고 그 변경은 건드리지 않았다.
