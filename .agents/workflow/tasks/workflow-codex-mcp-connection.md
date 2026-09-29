# Workflow Codex 로컬 MCP 연결 오류

상태: 완료 · 실제 Codex 실행 및 Workflow 회귀 검증 통과
다음 행동: 없음.

## 요청

사용자 · 2026-09-30: “어떻게 고쳐야할지 확인해서 올바른 방법으로 고쳐주세요.”

## 구현 계획

- Workflow Codex work 실행 전에 실제 CLI 설정의 unreal-mcp 로컬 HTTP 서버를 확인한다.
- TCP 연결 거부가 확인된 경우에만 해당 실행의 서버를 제외한다. 정상 서버·원격 서버·다른 MCP·설정 파일은 유지한다.
- 제외 사실을 콘솔·AI 입력·결과 evidence에 남기고 미실행 에디터 검증을 통과 처리하지 않도록 안내한다.
- plan 격리와 기존 provider 회귀, 포트 열림/닫힘을 임시 테스트로 확인한다.

구현 승인: 사용자 2026-09-30 · 위 수정 요청을 승인으로 적용

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 연결 상태별 처리 | 임시 Node 회귀 테스트 | AI | 통과 | 임시 Tests/mcp-regression.cjs: 실제 로컬 TCP 열림·닫힘, 콘솔·입력·evidence, 정상·비활성·다른 MCP·원격 유지 및 목록 실패 확인. 실제 Codex 설정 사전 조회도 통과. |
| Workflow 회귀 | 기존 provider 및 feedback 테스트 | AI | 통과 | TestWorkflowProviders.cjs 및 TestWorkflowTestFeedback.cjs 종료 코드 0. node --check 및 git diff --check 통과. |
| 코드 리뷰 | Workflow-Providers.cjs의 실행 한정 제외와 안내·근거 보존 확인 | AI | 통과 | 사용자 2026-09-30의 직접 테스트 후 완료 지시에 따라 AI 검토로 수행. 연결 거부만 제외하며 정상 연결·plan 격리·설정 파일 보존 확인. |
| 실제 Codex 통합 실행 | 실제 Runner 및 정상 테스트 MCP로 모델 응답까지 실행 | AI | 통과 | 오프라인: MCP_SMOKE_OK 및 제외 evidence, 해당 Transport 오류 없음. 온라인: initialize·tools/list와 MCP_ONLINE_OK, 제외 evidence 없음. |

## 조사 근거

- .codex/config.toml 및 실제 codex --disable plugins mcp list --json에서 unreal-mcp = http://127.0.0.1:8000/mcp 활성화 확인.
- Saved/Logs/Wx.log: 2026-09-29T15:12:53.222Z HttpListener stopping listening on Port 8000. 사용자 오류는 약 65초 뒤인 15:13:58Z부터 발생. 에디터 MCP 리스너 종료 후 Codex가 연결을 시도했다.
- 실제 사용자 환경에서 포트 8000 리스너와 Unreal 에디터 프로세스 없음 확인.
- 기존 Workflow-Providers.cjs는 plan에서만 MCP 격리, work에서는 중단된 서버도 연결한다.
- 기존 사용자 변경 .agents/workflow/tasks/지침-SSOT-점검.md는 보존한다.

## 검증 범위

- 최초 검증에서는 모의 AI 프로세스를 사용했다. 아래 완료 검증에서 실제 Codex 모델 응답까지 추가 확인했다. Unreal 에디터 도구 자체는 호출하지 않았다.
- 사전 확인 이후 서버가 종료되는 경우나 HTTP 프로토콜 오류까지 해결하는 변경은 아니다.
- 임시 테스트는 검증 후 제거했다. 기존 provider 테스트는 추가 사전 조회를 모의 처리하도록 갱신했다.
- 다음 Workflow 작업부터 새 Runner 프로세스가 수정된 provider를 읽는다. 실행 중인 작업은 중단하지 않았다.

## 완료 검증 · 2026-09-30

- 사용자 원문: “직접 테스트해보고 문제 없으면 완료처리 하세요.” 별도 사람 리뷰 대기를 AI 직접 검증 후 완료로 대체하라는 지시로 적용했다. 사람이 직접 코드를 리뷰했다고 기록하지 않는다.
- 임시 Tests/workflow-mcp-live/live.cjs 실행: 실제 Workflow-Runner.cjs → 설치된 Codex → 실제 모델 응답. 8000번 서버 미실행 상태에서 MCP_SMOKE_OK, 제외 evidence, 해당 Transport 오류 없음 확인.
- 임시 Tests/workflow-mcp-live/online.cjs 실행: 별도 로컬 테스트 MCP 서버를 실행하고 해당 프로세스의 URL만 임시 지정. 실제 Codex의 initialize·tools/list 및 MCP_ONLINE_OK 응답 확인. 정상 서버 유지와 제외 안내 없음 확인. Unreal 에디터 테스트를 대신하는 것은 아니다.
- TestWorkflowProviders.cjs, TestWorkflowTestFeedback.cjs, node --check .agents/scripts/Workflow-Providers.cjs, git diff --check 통과.
- 수정 코드 검토 통과. 설정 파일 영구 변경·커밋·푸시 없음. 기존 다른 작업 변경 보존. 임시 테스트 파일 정리.
