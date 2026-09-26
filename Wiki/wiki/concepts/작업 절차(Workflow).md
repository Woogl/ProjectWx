---
type: concept
title: "작업 절차(Workflow)"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "AI·사람 작업 절차, 대시보드, 테스트 체크리스트 운영 결정"
sources:
  - "[[결정 노트 - 2026-09-22-current-workflow]]"
  - "[[결정 노트 - 2026-09-22-workflow-closure]]"
  - "[[결정 노트 - 2026-09-22-workflow-dashboard]]"
  - "[[결정 노트 - 2026-09-22-workflow-review]]"
  - "[[결정 노트 - 2026-09-25-workflow-convenience-review]]"
  - "[[결정 노트 - 2026-09-25-workflow-simplification]]"
  - "[[결정 노트 - 2026-09-25-workflow-task-records]]"
  - "[[결정 노트 - 2026-09-25-workflow-test-feedback]]"
  - "[[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]]"
  - "[[작업 - workflow-review]]"
---

# 작업 절차(Workflow)

AI·사람 작업 절차, 대시보드, 테스트 체크리스트 운영 결정에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 사용자는 2026-09-22 Workflow의 코드 리뷰 승인 버전 보장, 테스트 수용과 최종 정리 완료 분리, 승인자 누락 수정을 요청했다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 사용자는 2026-09-22 Workflow 작업 현황 대시보드에서 단계별 진행 중인 일감을 한눈에 확인하도록 요청했다. ([[결정 노트 - 2026-09-22-workflow-dashboard]])
- 사용자는 기존 Workflow 웹 대시보드에서 테스트 결과를 이상 없음/이상 있음으로 제출하고 AI가 접수·마무리하는 기능을 요청했다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])

## 확정 결정

- 작업 절차는 정하기·만들기·확인하기 세 단계와 확인 대기·진행 중·완료 세 상태로 단순화되었고 규칙 정본은 process/index.md 한 장이다. ([[작업 - workflow-review]])
- 사용자는 구현 내용을 명확히 지시한 요청을 구현 승인으로 보는 규칙을 남기기로 했고, 이 지름길은 AI 대화에서만 동작하며 웹 새 작업은 정하기부터 시작한다. ([[작업 - workflow-review]])
- 웹에서 맡긴 구현·추가 요청·테스트 결과 처리는 모든 명령 허용으로, 정하기 조사는 읽기 전용으로 터미널 창에서 실행된다. ([[작업 - workflow-review]])
- 2026-09-22 Workflow 뷰어의 탐색 메뉴는 작업 현황 대시보드·새 작업 만들기·기존 작업 이어하기·사용 방법 안내·LLM 위키 검색으로 확정됐다. ([[결정 노트 - 2026-09-22-workflow-dashboard]])
- 사용자는 2026-09-22 기획자에게 툴 난도가 높으므로 Workflow 기획 단계를 개발자가 전달받은 기획서를 검토하는 단계로 바꾸도록 요청했다. ([[결정 노트 - 2026-09-22-workflow-review]])
- 2026-09-22 Workflow 기획서 검토 단계에서 AI는 누락·충돌·구현 영향을 조사하되 없는 기획이나 수치를 임의로 작성하지 않는다. ([[결정 노트 - 2026-09-22-workflow-review]])
- 2026-09-22 Workflow의 문서 검색 링크·단축키·검색 주소 진입은 제거하고 Wiki 검색은 유지하기로 했다. ([[결정 노트 - 2026-09-22-workflow-review]])
- 사용자는 2026-09-25 Workflow 편의 점검 뒤 코드 식별값 검사 제거, 코드 리뷰를 확인하기 체크리스트의 사람 항목으로 이동, 옛 기록 정리, 작업 현황 자동 생성을 승인했다. ([[결정 노트 - 2026-09-25-workflow-convenience-review]])
- 사용자는 2026-09-25 Workflow를 직관적이고 단순하게 해 달라며 정하기·만들기·확인하기 3단계, 상태 3개, 웹 새 작업 경로 폐지, 규칙 한 장 원칙을 승인했다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 사용자 요청으로 정하기에서 만들기로 넘어가려면 AI의 추가 질문이 없을 때 사람이 구현을 승인해야 한다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 사용자 요청으로 확인하기는 AI가 만든 테스트 체크리스트에서 AI 항목은 AI가 직접 테스트하고 사람 항목은 사람이 테스트해 반영한다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 사용자는 2026-09-25 작업 목차 표시 위치로 기존 Workflow 웹 화면에서 보기를 선택했다. ([[결정 노트 - 2026-09-25-workflow-task-records]])
- 사용자는 2026-09-26 Workflow 대시보드에서 확인 대기 행의 기록 열기 버튼과 완료 행의 작업 진행 버튼을 없애기로 했다. ([[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]])
- 완료된 Workflow 작업에서 새 문제가 생기면 완료 기록에 테스트 결과를 다시 보내지 않고 새 작업으로 요청한다. ([[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]])

## 구현 관찰

- Workflow-TestFeedback.cjs 서버는 단계마다 AI 결과 칸을 제한하고 상태를 기록의 질문·계획·체크리스트에서만 정하며, AI는 사람 체크리스트 항목을 통과로 바꾸거나 지울 수 없다. ([[작업 - workflow-review]])
- 2026-09-22 정적 조사 기준 Workflow는 기획서 검토·설계·구현·테스트·완료 흐름에서 AI 조사 → 사람 판단 → AI 재검토 → 사람 확정의 공통 판단 절차를 두었다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 정적 조사 기준 Workflow 절차는 AI 추천·미확정 답변을 결정으로 보지 않고, 개별 답변·단계 확정·코드 리뷰 수용·테스트 수용을 별개로 다뤘다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 정적 조사 기준 Workflow-Execution.cjs는 AI 구현·검증을 sandbox workspace-write·ephemeral 모드와 30분 타임아웃으로 실행하고 accept 때 complete로 게시했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 정적 조사 기준 웹의 테스트 완료 버튼은 테스트 수용만 기록하고 Wiki 편찬·자료 정리를 자동 실행하지 않았다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 Workflow-Execution.cjs는 검증 시작과 accept 시 현재 코드 버전이 마지막 approve의 codeVersion과 다르면 blocked/implement로 되돌렸다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 2026-09-22 Workflow 실행기에서 accept는 cleanup을 저장하고, finish는 승인자·Wiki 반영 상태·Wiki 근거·정리 근거가 있어야 complete와 closure를 저장했다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 2026-09-22 Workflow의 approve·accept는 승인자 이름이 비면 거부하고 decisions.actor에 표시명을 저장하며 계정 인증은 하지 않았다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 2026-09-22 Workflow 대시보드(workflow.js)는 작업을 단계별로 분류하고 changeTo가 있는 작업을 인계 구역에 두며, 완료를 테스트 수용으로만 취급했다. ([[결정 노트 - 2026-09-22-workflow-dashboard]])
- 편의 점검 후 작업 현황은 작업 기록 제목 아래의 상태: 줄과 다음 행동: 줄을 task-records.js가 읽어 만들고 tasks/index.md는 표 없는 안내 문서가 되었다. ([[결정 노트 - 2026-09-25-workflow-convenience-review]])
- 편의 점검 후 테스트 결과 서버는 코드 식별값 비교를 없애고 작업 기록 해시 대조만 유지한다. ([[결정 노트 - 2026-09-25-workflow-convenience-review]])
- Workflow 단순화 시점 테스트 체크리스트는 작업 기록의 항목·확인 방법·담당·결과·근거 표이며 서버의 guardChecklist가 AI의 사람 항목 통과 처리·삭제·담당 변경을 되돌린다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- Workflow 대시보드는 `.agents/workflow/tasks/index.md`의 네 표를 원본으로 대화 작업 기록을 플레이 확인·에디터 확인·개선 판단·완료 기록으로 나눠 확인 범위와 다음 행동을 보여 준다. ([[결정 노트 - 2026-09-25-workflow-task-records]])
- 작업 절차 정본에는 단계 완료·인계 시 상세 Task와 목차의 분류·확인 범위·다음 행동을 함께 갱신하도록 적혔다. ([[결정 노트 - 2026-09-25-workflow-task-records]])
- 테스트 결과 접수는 `/test-feedback` API와 `test_feedback_<task path hash>.json`에 원문·확인자·시각·코드 식별값·재시도 이력을 보존한다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])
- 테스트 결과 접수 후 AI 결과가 통과이고 코드·Task 변경과 남은 확인이 없을 때만 완료, 문제·코드 변경·사람 확인이 남으면 재확인, 실패 검사·미해결 판단은 확인 필요로 남는다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])
- 테스트 결과 접수 경로는 부분 테스트를 작업 전체 수용이나 코드 리뷰 승인으로 확대하지 않고, 접수 이후 코드가 바뀌면 새 결과를 요구한다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])
- Workflow 대시보드 목록 행 버튼은 확인 대기는 작업 진행, 진행 중은 기록 열기와 작업 진행, 완료는 기록 열기만 표시한다. ([[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]])

## 검증 범위

- workflow-review의 사람 항목 7개(코드 리뷰, 대시보드, 작업 절차 한 장, 체크리스트 전달, 웹 새 작업 흐름, 터미널 이어하기, SSoT)는 이우성이 통과로 확인했다. ([[작업 - workflow-review]])
- 2026-09-22 Workflow 정본·실행 경계 조사는 저장소 원문 발췌와 SHA-256 기록이며 서버·AI·게임 실행 검증을 포함하지 않는다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 Workflow closure 구현은 TestWorkflowExecution.cjs와 TestWikiViewer.cjs 모의 DOM 테스트로만 확인했고 실제 AI 호출·브라우저 육안·UE 실행은 하지 않았다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 2026-09-22 Workflow 대시보드는 모의 DOM 테스트로만 검증했고 실제 브라우저 육안·AI 실행 검증은 하지 않았다. ([[결정 노트 - 2026-09-22-workflow-dashboard]])
- 2026-09-22 기획서 검토 전환은 TestWikiViewer·TestWikiSpaces·TestWikiAI 모의 테스트로 확인했으며 실제 AI 응답 품질과 브라우저 육안 검증은 포함하지 않는다. ([[결정 노트 - 2026-09-22-workflow-review]])
- Workflow 편의 개선은 스크립트 자동 테스트·링크 검사·로컬 서버 재시작·헤드리스 캡처로 확인했고 실제 AI 처리 요청과 사람의 화면 확인은 하지 않았다. ([[결정 노트 - 2026-09-25-workflow-convenience-review]])
- 작업 기록 현황 표시는 TestWikiViewer 등 자동 테스트와 모의 DOM으로만 확인했고 실제 브라우저 확인은 로컬 파일 URL 보안 정책으로 수행하지 못했다. ([[결정 노트 - 2026-09-25-workflow-task-records]])
- 테스트 결과 접수 기능은 자동 테스트·모의 AI·실제 로컬 HTTP·모의 DOM으로 확인했고 실제 AI 결과 제출·브라우저 육안·UE 실행은 하지 않았다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])

## 미결정·충돌

- 여러 대화 세션이 한 체크아웃을 동시에 편집·빌드할 때의 조율 절차는 미해결이며 다른 작업의 코드 변경은 자동 감지되지 않는다. ([[작업 - workflow-review]])
- Workflow 단순화 시점에 웹 경로 전용 스크립트·테스트·옛 절차 문서는 자동 모드 권한 검사의 삭제 거부로 삭제 대기 상태였다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 대시보드 행 버튼 원자료는 리뷰·참고 행을 구현 설명에서는 기록 열기만, TestWikiViewer 설명에서는 버튼 없음으로 서로 다르게 적는다. ([[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]])

## 원자료

- [[결정 노트 - 2026-09-22-current-workflow]] — 2026-09-22 시점 AGENTS.md·옛 LLM Wiki 설정·Workflow 절차와 실행 스크립트의 계약을 발췌한 정적 조사 노트로 사람 판단·AI 실행 경계를 기록한다
- [[결정 노트 - 2026-09-22-workflow-closure]] — Workflow 실행기에 승인 코드 버전 대조, 테스트 수용과 정리 완료 분리, 승인자 이름 기록을 구현한 2026-09-22 요청·구현·모의 테스트 범위 기록
- [[결정 노트 - 2026-09-22-workflow-dashboard]] — Workflow 작업 현황 대시보드에서 단계별 진행 일감을 한눈에 보도록 한 2026-09-22 사용자 요청과 workflow.js 구현·모의 DOM 검증 범위 기록
- [[결정 노트 - 2026-09-22-workflow-review]] — 기획자에게 툴 난도가 높다는 이유로 Workflow 기획 단계를 기획서 검토 단계로 바꾸고 Workflow 문서 검색을 제거한 2026-09-22 사용자 결정 기록
- [[결정 노트 - 2026-09-25-workflow-convenience-review]] — 단순화한 Workflow를 AI·사람 관점에서 점검한 뒤 사용자가 식별값 검사 제거·코드 리뷰 체크리스트화·옛 기록 정리·작업 현황 자동 생성 네 개선을 승인한 기록
- [[결정 노트 - 2026-09-25-workflow-simplification]] — 사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지하며 AI·사람 담당 테스트 체크리스트를 도입한 기록
- [[결정 노트 - 2026-09-25-workflow-task-records]] — 기존 Workflow 웹 대시보드에 대화 작업 기록을 네 분류로 나눠 확인 범위와 다음 행동을 보여 주는 변경의 구현 관찰과 검증 범위
- [[결정 노트 - 2026-09-25-workflow-test-feedback]] — 기존 Workflow 작업에 사람의 테스트 결과(이상 없음/이상 있음)를 접수해 AI가 수정·정리·재확인을 이어가는 경로의 구현 관찰과 검증 범위
- [[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]] — Workflow 대시보드 목록 행 버튼을 분류별로 정리한 사용자 결정과 구현: 확인 대기는 작업 진행만, 완료·리뷰·참고는 기록 열기만
- [[작업 - workflow-review]] — AI 작업 워크플로우를 점검하고 작업 절차 한 장·세 단계·테스트 체크리스트·웹 새 작업과 이어하기로 단순화한 2026-09-22~26 완료 작업 기록
