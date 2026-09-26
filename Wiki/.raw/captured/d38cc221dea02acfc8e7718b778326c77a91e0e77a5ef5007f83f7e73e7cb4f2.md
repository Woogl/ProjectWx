---
title: "Workflow 작업 테스트 결과 접수"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, workflow]
summary: "기존 작업에서 사람의 테스트 결과를 AI에게 전달하고 수정·정리·재확인을 이어가는 경로"
---

# 사용자 요청

사용자는 기존 Workflow 웹 대시보드에서 작업을 이어가길 원했다. 이어서 테스트 결과를 이상 없음/이상 있음으로 선택하고 문제가 있으면 내용을 기록해 AI가 접수·마무리하는 기능을 요청했다. 구현 기준 HEAD는 `38d4dde08`이며 미커밋 작업 트리를 포함한다.

# 구현 관찰

- 작업별 입력에 확인자·직접 확인한 항목·상호 배타적인 두 결과를 둔다. 이상 있음에만 문제 상황·재현 방법 입력을 표시하며 필수다.
- `/test-feedback`의 list/read/submit/retry가 접수와 조회를 처리한다. `test_feedback_<task path hash>.json`에 원문·확인자·시각·작업 해시·코드 식별값·결과·재시도 이력을 보존한다. operationId와 요청 본문 해시로 재전송의 중복 실행을 막는다.
- Codex CLI의 기존 workspace-write 실행 경로에서 원래 Task의 최신 결정과 남은 일을 읽는다. 이상 없음은 수용 범위 대조·지식 정리, 이상 있음은 합의 범위 내 수정·검증이다. 기존 기획·설계·리뷰 승인 파일을 생성하지 않는다.
- AI 결과가 통과이고 코드·Task 변경과 남은 확인이 없을 때만 완료한다. 문제가 보고되었거나 코드 변경·미실행·사람 확인이 남으면 재확인, 실패 검사·미해결 판단은 확인 필요로 남긴다. 부분 테스트를 작업 전체 수용·코드 리뷰 승인으로 확대하지 않도록 지시한다.
- 서버가 원래 Task에 사람 결과와 AI 보고를 추가하고 목차를 갱신한다. 완료 기록에 새 문제가 생기면 확인할 일로 돌아간다. 입력 초안과 응답 유실 요청을 브라우저에 보존하며 실패·중단은 저장된 결과로 재시도한다. 접수 이후 코드가 달라지면 새 결과가 필요하다.
- 코드 식별은 이 경로에서 Markdown·Wiki 정리를 제외하며 HEAD 변경은 여전히 감지한다. 기존 웹 작업의 승인 버전 판정은 그대로다. 두 실행 경로와 기존 분석은 로컬 서버에서 동시에 시작되지 않도록 잠근다.
- 대시보드 상태·AI 결과는 주기적으로 조회한다. 기록 열기의 상세 본문은 생성 HTML이므로 OpenWorkflow.bat 재실행 후 추가 기록이 보인다.

# 입력 식별

| 파일 | SHA-256 |
| --- | --- |
| `.agents/scripts/Workflow-TestFeedback.cjs` | `7A5CE2F459B7ADE7D3940E7AAAD312795C2530880B243A7BD1320F1CFEE1E795` |
| `.agents/scripts/wiki-viewer/test-feedback.js` | `83ACE58EF0FEDDB4BB60A15C96C69984CE2ECB154BBD0A0E92819B6F2BB0FD01` |
| `.agents/scripts/Wiki-AI.cjs` | `6D9B5B35B66FCF8FF8299E40B7D365531BD775EE42435E41CCEDA95D454EF891` |
| `.agents/scripts/Workflow-Execution.cjs` | `B21A86E926B61F0EAB5D03F003B20C65D4B110C5BA92A7BBF86E70EF5710137E` |

# 검증과 제한

TestWorkflowTestFeedback·TestWorkflowFeedbackUI와 기존 TestWikiViewer·TestWikiSpaces·TestWikiTasks·TestWikiRecovery·TestWikiAI·TestWorkflowExecution 통과. 임시 저장소·모의 AI 실행기·실제 로컬 HTTP·모의 DOM으로 입력 검증, 응답 유실, 재시도·재시작, 코드/Task 변경, 처리 분류, 원문 보존, 목차 이동, 실행 잠금, 토큰/Origin과 화면 갱신을 확인했다.

실제 AI 결과 제출·브라우저 육안·UE 실행은 수행하지 않았다. 기존 로컬 파일 URL 보안 정책으로 브라우저 자동화 확인은 제한된다. 다른 세션의 게임 코드·에셋·생성 문서 변경을 보존했다.
