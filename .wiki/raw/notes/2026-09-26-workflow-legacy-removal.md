---
title: "Workflow 구 AI 워크플로우 잔재 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-26
tags: [wx, workflow]
summary: "사용자 요청으로 삭제 대기였던 옛 웹 경로 스크립트·테스트와 한 장으로 합친 옛 절차 문서·4단계 그림 19개를 지웠다. 현재 코드에 남은 옛 결과 형식(남은 확인·검사 목록·확인 범위·AI 확인 요청 상태) 표시와 옛 대시보드 CSS를 없애고, Claude 권한 거부 알림은 처리 근거에 남기도록 옮겼다."
---

# 사용자 요청

2026-09-26 사용자가 "구 AI 워크플로우의 잔재가 남아있다면 제거합시다."라고 했다. 작업 기록 `workflow-review`의 삭제 대기 목록은 사용자 승인을 기다리던 상태였다.

# 삭제한 파일

`git rm`으로 지웠다(사용자가 직접 요청한 삭제라 자동 모드에서도 허용됐다).

- 옛 웹 작업 경로: `.agents/scripts/`의 `Wiki-Tasks.cjs`, `Workflow-Execution.cjs`, `Wiki-Import.cjs`, `Wiki-Import.py`, `wiki-checklist.schema.json`, `wiki-gemini-policy.toml`, `wiki-viewer/workflow-model.js`, `wiki-viewer/execution.js`
- 그 경로의 테스트: `TestWikiAI.cjs`, `TestWikiTasks.cjs`, `TestWikiRecovery.cjs`, `TestWikiImport.cjs`, `TestWorkflowExecution.cjs`
- 작업 절차 한 장으로 합친 옛 절차 문서: `.agents/workflow/process/`의 `design_review.md`, `implementation.md`, `testing.md`, `completion.md`, `.agents/workflow/usage.md`, `.agents/workflow/assets/ai-workflow.png`(4단계 그림)

지우기 전에 저장소 전체를 검색했다. 현재 코드·문서·스킬·배치 파일·에이전트 설정은 이 파일들을 참조하지 않았고, 참조는 삭제 대상끼리와 로그·출처 메모 같은 이력뿐이었다. 뷰어 번들은 현재 스크립트만 이름으로 묶어 옛 스크립트가 들어가지 않았다.

# 현재 코드에서 없앤 흔적

- 작업 진행 화면(`wiki-viewer/test-feedback.js`): 옛 처리 상태 `blocked`의 "AI 확인 요청" 문구, 옛 보고서의 남은 확인(`blockers`)·검사 목록(`checks`) 표시, 첫 테스트 결과 접수 방식의 확인 범위(`scope`) 표시를 없앴다. 서버의 작업 요약(`Workflow-TestFeedback.cjs` view)에서도 `scope`를 뺐다.
- 처리 AI 실행(`Wiki-AI-Providers.cjs`): Claude Code 권한 거부 알림을 옛 결과 칸 `blockers`에 넣던 코드는 현재 결과 형식에 그 칸이 없어 알림이 버려지고 있었다. 모든 단계에 있는 처리 근거(`evidence`)에 한 줄로 남기도록 옮겼다.
- 뷰어 CSS(`wiki-viewer/index.html`): 2026-09-22 옛 대시보드를 바꿀 때 사용처가 사라진 통계 카드(`.stats`·`.stat`)와 상태 배지(`.badge` 및 needs-review·blocked·historical·untracked) 규칙 9개를 지웠다.
- 테스트: TestWikiProviders의 표본을 현재 결과 형식(요약·근거·변경·질문·체크리스트)으로 바꾸고 권한 거부가 근거에 남는지 확인한다. TestWorkflowFeedbackUI의 옛 결과 표시 사례를 현재 형식(처리 결과 확인과 요약)으로, TestWikiViewer의 AI 상태 배지 예시를 "질문 답변 필요"로, 경로 해석 예시를 지운 문서 대신 작업 절차로 바꿨다.

옛 형식 보고서가 남은 접수 이력은 `checkpoint-savegame`(최근 처리는 결과 기록)과 `animnotify-categories`(완료 기록이라 작업 진행에서 열지 않음) 둘뿐이라 화면에서 사라지는 내용은 없다. 접수 이력 JSON은 이력이라 고치지 않았다.

# 잔재로 보지 않고 남긴 것

- 옛 결과 칸(`blockers`·`checks`)을 AI가 보내도 버리는지 확인하는 회귀 테스트와, 폐지한 웹 경로 API·문구가 생성 화면에 돌아오지 않는지 확인하는 뷰어 테스트
- 요청·질문·계획·체크리스트 절이 없는 기록(상태 줄만 있는 모듈 리뷰 등)의 다음 행동을 사람 항목 하나로 보여주는 처리, 자유 형식 상태 줄 기록을 리뷰·참고로 분류하는 처리
- 워크플로우 문서의 PNG 묶기(`Export-Wiki.ps1`·뷰어 이미지 해석): 4단계 그림을 지운 뒤 쓰는 이미지가 없지만 폴더가 없어도 동작하는 범용 기능이라 남겼다. 없앨지는 사용자가 정하지 않았다.

# 검증

- Export-Wiki.ps1로 화면을 다시 만들었다(Workflow 59문서, 옛 절차 문서 5개 제외). CheckWikiLinks 오류 0건(61문서).
- TestWorkflowTestFeedback·TestWorkflowFeedbackUI·TestWikiProviders·TestWikiViewer·TestWikiSpaces를 통과했다.
- 로컬 서버는 사용 중인 대시보드를 끊지 않도록 재시작하지 않았다. Start-WikiAI.ps1이 서버 코드 해시가 다르면 다시 띄우므로 다음 OpenWorkflow.bat 실행 때 새 코드가 적용된다.
