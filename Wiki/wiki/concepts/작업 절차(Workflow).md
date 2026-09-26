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
  - "[[결정 노트 - 2026-09-26-workflow-human-verification]]"
  - "[[결정 노트 - 2026-09-26-workflow-image-removal]]"
  - "[[결정 노트 - 2026-09-26-workflow-korean-record-names]]"
  - "[[결정 노트 - 2026-09-26-workflow-legacy-removal]]"
  - "[[결정 노트 - 2026-09-26-workflow-process-diagram]]"
  - "[[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]]"
  - "[[결정 노트 - 2026-09-26-workflow-row-actions-final]]"
  - "[[결정 노트 - 2026-09-26-workflow-ssot-copies]]"
  - "[[결정 노트 - 2026-09-26-workflow-state-from-record-only]]"
  - "[[결정 노트 - 2026-09-26-workflow-tasks-guide-removed]]"
  - "[[결정 노트 - 2026-09-26-workflow-web-tasks]]"
  - "[[작업 - workflow-review]]"
  - "[[작업 - dashboard-work-tab-split]]"
  - "[[작업 - harness-legacy-cleanup]]"
  - "[[작업 - wiki-claude-obsidian-migration]]"
  - "[[작업 - workflow-final-fixes]]"
  - "[[작업 - workflow-inspection]]"
---

# 작업 절차(Workflow)

AI·사람 작업 절차, 대시보드, 테스트 체크리스트 운영 결정에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다. 이 페이지는 결정 이력과 근거를 모으며, 지금 지킬 작업 절차 규칙은 정본 `.agents/workflow/process/index.md`를 봅니다.

## 요구사항

- 사용자는 2026-09-22 Workflow의 코드 리뷰 승인 버전 보장, 테스트 수용과 최종 정리 완료 분리, 승인자 누락 수정을 요청했다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 사용자는 2026-09-22 Workflow 작업 현황 대시보드에서 단계별 진행 중인 일감을 한눈에 확인하도록 요청했다. ([[결정 노트 - 2026-09-22-workflow-dashboard]])
- 사용자는 기존 Workflow 웹 대시보드에서 테스트 결과를 이상 없음/이상 있음으로 제출하고 AI가 접수·마무리하는 기능을 요청했다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])
- 사용자는 2026-09-26 워크플로우 종합 점검을 요청하며 "가능하면 순정을 그대로 쓰고, 복잡한 장치는 줄였으면 해요. 유지보수 때문이에요", "모든 지침은 SSOT를 엄격하게 준수해야합니다"를 기준으로 제시했다. ([[작업 - workflow-inspection]])
- 사용자는 2026-09-26 대시보드는 작업 현황을 보는 곳으로 두고 새 작업·이어서 작업을 누르면 작업 탭으로 전환되게 나누며, 이름 입력을 작성자로 바꾸고 구현 계획을 Markdown과 도식으로 보이게 해 달라고 요청했다. ([[작업 - dashboard-work-tab-split]])
- 사용자는 2026-09-26 워크플로우 최종 마무리 코드 리뷰를 요청했고, 2026-09-27 확인된 지적을 고치자고 했다("네 고칩시다"). ([[작업 - workflow-final-fixes]])
- 사용자는 2026-09-27 옛 스킬 빈 폴더와 그 밖의 옛 레거시·흔적을 모두 지우고, 이름에 옛 Wiki가 남은 스크립트는 쓰지 않으면 지우고 쓰면 더 적절한 이름으로 바꾸라고 요청했다. ([[작업 - harness-legacy-cleanup]])

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
- Workflow 문서의 도식은 PNG 그림 대신 Mermaid로만 그린다(2026-09-26 문서 이미지 기능 제거). ([[결정 노트 - 2026-09-26-workflow-image-removal]])
- 사용자 질문(2026-09-26 해시 말고 한글로 추적할 수는 없나요?)에 따라 Workflow 웹 새 작업의 기록 이름은 해시 대신 한글 제목으로 바뀌어, 이전의 영문·숫자+해시 규칙을 대체했다. ([[결정 노트 - 2026-09-26-workflow-korean-record-names]])
- 사용자는 2026-09-26 구 AI 워크플로우의 잔재 제거를 요청해, 옛 웹 작업 경로 스크립트·테스트와 작업 절차 한 장으로 합친 옛 절차 문서·4단계 그림이 삭제됐다. ([[결정 노트 - 2026-09-26-workflow-legacy-removal]])
- 작업 절차 도식은 2026-09-26 사용자가 고른 시안 B 기반의 정하기·만들기·확인하기 단계 상자 배치로 바뀌었고, 옛 승인된 범위 안의 수정 지름길은 도식에서 뺐다. ([[결정 노트 - 2026-09-26-workflow-process-diagram]])
- 사용자는 2026-09-26 Workflow 대시보드의 진행 중 행에서도 기록 열기 버튼을 빼라고 해, 확인 대기·진행 중 행은 작업 진행만 두는 것이 최종 결정이 됐다. ([[결정 노트 - 2026-09-26-workflow-row-actions-final]])
- 사용자 요청으로 Workflow 사이드바의 사람이 작업하고 판단하는 공간 소개 문구가 삭제됐다. ([[결정 노트 - 2026-09-26-workflow-row-actions-final]])
- 사용자는 2026-09-26 워크플로우의 SSoT를 엄격하게 지키기로 해, 작업 절차 규칙은 .agents/workflow/process/index.md 한 장에만 두고 AGENTS.md는 링크 한 줄만 둔다. ([[결정 노트 - 2026-09-26-workflow-ssot-copies]])
- 작업 절차 정본은 사람이 구현 내용을 명확히 지시한 요청을 구현 승인으로 보는 규칙을 AI 대화에만 적용하고, 웹 새 작업은 지시가 명확해도 정하기부터 시작한다. ([[결정 노트 - 2026-09-26-workflow-ssot-copies]])
- 사용자 승인(2026-09-26)으로 Workflow에서 AI가 사람에게 넘기는 것은 질문·구현 계획·테스트 체크리스트 셋뿐이고, 작업 상태는 기록의 이 세 절에서만 정한다. ([[결정 노트 - 2026-09-26-workflow-state-from-record-only]])
- Workflow에서 테스트 체크리스트가 모두 통과하면 서버가 즉시 완료하고, 이어지는 AI 정리(Wiki 반영)는 상태를 바꾸지 않아 실패해도 완료가 유지된다. ([[결정 노트 - 2026-09-26-workflow-state-from-record-only]])
- Workflow AI 결과는 단계별로 받을 칸이 제한되고(정하기=질문·계획, 구현·수정=변경·질문·체크리스트, 추가 요청=변경·질문·계획·체크리스트, 정리=변경) blockers·checks 칸은 없어졌다. ([[결정 노트 - 2026-09-26-workflow-state-from-record-only]])
- Workflow에서 AI가 실행하지 못한 항목은 사람 항목으로 넘기고, 사람 항목이 없으면 결과 확인 항목을 더해 완료는 항상 사람 확인으로 끝난다. ([[결정 노트 - 2026-09-26-workflow-state-from-record-only]])
- 사용자 요청(2026-09-26)으로 Workflow 대시보드의 기록 작성 규칙 링크와 기록 폴더 안내 문서 .agents/workflow/tasks/index.md가 삭제됐고, 기록 규칙은 작업 절차 한 장에만 있다. ([[결정 노트 - 2026-09-26-workflow-tasks-guide-removed]])
- 작업 절차 도식 점검(2026-09-26)이 미결로 남긴 두 가지, 곧 정하기 중(미답변 질문·미승인 계획)에 보낸 추가 요청이 모든 명령 허용으로 실행되는 문제와 완료 직후 작업 진행 패널의 추가 요청 버튼 재활성은 같은 날 워크플로우 종합 점검(`.agents/workflow/tasks/workflow-inspection.md`의 Q7·Q8)으로 정해졌다. 현재 규칙은 정본 `.agents/workflow/process/index.md`를 본다. ([[결정 노트 - 2026-09-26-workflow-process-diagram]], [[작업 - workflow-review]], [[작업 - workflow-inspection]])
- 사용자는 2026-09-25 Workflow 대시보드에서 새 작업 시작과 기존 작업 이어하기를 요청하고, 웹 안 처리·터미널로만 AI 대화 열기·구현은 모든 명령 허용·터미널 창에 보이며 실행을 골랐다. ([[결정 노트 - 2026-09-26-workflow-web-tasks]])
- 2026-09-26 claude-obsidian 전환으로 완료 뒤 AI의 Wiki 정리가 없어졌고, 사용자 요청("좌측 탭 아래쪽으로 합시다. 항상 보이게요.")으로 처리할 AI는 왼쪽 메뉴 아래 드롭다운 하나에서 골라 모든 전달·터미널·Wiki 갱신에 쓴다. ([[작업 - wiki-claude-obsidian-migration]])
- 2026-09-26 워크플로우 종합 점검에서 사용자는 "Q6는 플러그인 순정대로 해주시고, 나머지는 모두 추천대로 해주세요."라고 답해, 코드 속 AI 요청문에는 결과 칸 형식과 권한 제한 한 줄만 남기고(Q1), 접수 상태 파일은 Git 밖 `Saved/Workflow`로 옮기며(Q2), 화면의 보류·재전송 장치를 없애고(Q3), 작업 처리와 Wiki 갱신이 따로 돌게 했다(Q5). ([[작업 - workflow-inspection]])
- 종합 점검 Q7·Q8(2026-09-26)로 구현 승인 전 정하기 단계의 추가 요청은 읽기 전용으로 실행되고, 완료된 작업은 추가 요청을 받지 않게 되었다. 이는 웹에서 맡긴 추가 요청을 단계와 상관없이 모든 명령 허용으로 실행하던 2026-09-26 이전 결정을 대체한다. ([[작업 - workflow-inspection]], [[결정 노트 - 2026-09-26-workflow-web-tasks]])
- 종합 점검 Q9(2026-09-26)는 "Wiki"가 붙은 대시보드 도구 이름을 그대로 두기로 했으나, 2026-09-27 사용자 추가 요청으로 `Wiki-AI.cjs` → `Workflow-Server.cjs`, `Export-Wiki.ps1` → `Export-WorkflowPage.ps1`, `wiki-viewer/` → `workflow-page/`, `CheckWikiLinks.ps1` → `CheckDocLinks.ps1` 등으로 이름을 바꿨고 `Wiki-Obsidian.cjs`는 그대로 두었다. ([[작업 - workflow-inspection]], [[작업 - harness-legacy-cleanup]])
- 사용자는 2026-09-26 여러 프로그래머가 병렬로 워크플로우를 돌릴 때의 충돌을 장치 없이 작업자 약속 세 가지(한 작업 기록은 한 번에 한 사람, 구현 승인 전 계획의 "어디에"로 에셋 겹침 조율, 새 작업 제목은 구체적으로)로 다루기로 했다("이건 작업자끼리 서로 조심하도록 약속하면 되지 않을까요?"). ([[작업 - workflow-inspection]])
- 2026-09-26 사용자 요청으로 대시보드와 작업 탭이 나뉘고 작업 진행 버튼은 이어서 작업으로, 이름 칸은 작성자로 바뀌었으며, 사용자는 문서와 구현 계획의 Markdown 렌더러를 marked 하나로 합치는 안을 골랐다. ([[작업 - dashboard-work-tab-split]])
- 이우성은 2026-09-27 AI 실행 제한을 정하기 30분·그 밖 60분으로(Q1), 선택지 없는 자유 답변 질문을 허용하고(Q2), 대시보드의 기록 열기는 서버가 연결돼 있으면 최신 내용을 보이게(Q3) 정했다. ([[작업 - workflow-final-fixes]])
- 사용자는 2026-09-27 "워크플로우와 claude-obsidian의 문서는 서로 독립적으로 관리되지만, 서로를 참고해서 DB를 더욱 견고하게 쌓아올리는 방향을 추구합니다"라고 밝혔고, 이에 따라 워크플로우 정책으로 Wiki 페이지를 줄이게 하던 Wiki 안내 단계를 뺐다("네 그렇게 합시다"). ([[작업 - workflow-final-fixes]])
- 2026-09-27 사용자 요청("제가 확인해야되는 부분만 정리해서 새 일감을 만들고, 기존 일감은 적절히 통폐합합시다")으로 워크플로우 개선 작업 다섯 건의 남은 사람 확인을 확인 대기 기록 `workflow-wrapup-checks.md` 하나로 모았고, 이 다섯 건의 사람 코드 리뷰는 이번에만 AI 코드 리뷰로 대신했다(다음에는 사람이 리뷰). ([[작업 - dashboard-work-tab-split]], [[작업 - harness-legacy-cleanup]], [[작업 - wiki-claude-obsidian-migration]], [[작업 - workflow-final-fixes]], [[작업 - workflow-inspection]])

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
- Workflow 서버는 같은 기록 이름이 있으면 -2, -3을 붙여 배타적으로 생성하고, 재전송은 처리 이력의 create 요청에서 같은 접수를 찾는다. ([[결정 노트 - 2026-09-26-workflow-korean-record-names]])
- Workflow 터미널 이어하기는 한글 기록 이름도 열 수 있고 cmd 특수 문자만 창을 여는 쪽에서 막는다. ([[결정 노트 - 2026-09-26-workflow-korean-record-names]])
- Workflow 작업 진행 화면은 옛 결과 형식(blockers·checks·scope)과 AI 확인 요청 문구를 더 표시하지 않고, Claude 권한 거부 알림은 evidence에 남는다. ([[결정 노트 - 2026-09-26-workflow-legacy-removal]])
- Workflow 서버에서 질문 답변은 읽기 전용 정하기를 다시 실행하고, 구현 승인은 미답변 질문이 없고 미승인 계획이 있을 때만 받는다. ([[결정 노트 - 2026-09-26-workflow-process-diagram]])
- Workflow에서 사람이 적은 실패는 바로 수정 처리를 시작하지만 AI 항목의 실패는 확인 대기 · 실패 확인에서 멈추고 추가 요청을 기다린다. ([[결정 노트 - 2026-09-26-workflow-process-diagram]])
- 첫 실제 Codex 처리(2026-09-26)에서 결과 스키마가 모든 칸을 요구해 Codex가 plan에 완료 문장을 채웠고, Workflow 서버는 이를 승인 줄 없는 구현 계획 절로 기록했다. ([[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]])
- Workflow 서버는 질문과 새 계획을 정하기(새 작업·질문 답변)와 추가 요청 결과에서만 받아, 구현·테스트 결과 처리의 질문·계획은 기록되지 않는다. ([[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]])
- Workflow 대시보드는 완료·리뷰·참고 행에 AI 처리 상태 배지를 붙이지 않고 확인 대기·진행 중 행에만 붙인다. ([[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]])
- Workflow 대시보드는 행 버튼 칸을 160px 고정·오른쪽 정렬로 두어 상태 배지가 있는 행도 버튼 줄이 맞고, 1000px 이하에서는 버튼 칸이 행 아래로 내려간다. ([[결정 노트 - 2026-09-26-workflow-row-actions-final]])
- Workflow 처리 프롬프트(taskPrompt)는 규칙 문장을 빼고 정본을 따르라는 첫 줄과 처리 단계·결과 칸 형식·전체 권한 제한·evidence 작성법만 담는다. ([[결정 노트 - 2026-09-26-workflow-ssot-copies]])
- tasks/index.md 삭제 뒤 Workflow 대시보드 머리는 제목과 새 작업 버튼만 있고, 기록 폴더의 모든 .md가 작업 기록으로 목록에 오른다. ([[결정 노트 - 2026-09-26-workflow-tasks-guide-removed]])
- Workflow 웹 처리에서 정하기는 읽기 전용 권한으로, 구현 승인·추가 요청·테스트 결과 처리는 권한 확인 없는 모든 명령 허용으로 AI CLI를 실행한다. ([[결정 노트 - 2026-09-26-workflow-web-tasks]])
- Workflow 서버는 AI 처리마다 Saved/Wiki/jobs 아래 작업 폴더를 만들고 cmd start로 Workflow-Runner.cjs 창을 띄워, 결과 없이 닫히거나 35분이 지나면 실패로 처리한다. ([[결정 노트 - 2026-09-26-workflow-web-tasks]])
- Workflow 작업 기록은 상태·다음 행동 두 줄 아래 요청·질문·구현 계획·테스트 체크리스트 절 순서를 따르고, AI가 준 계획의 구현 승인 줄은 목록 항목으로 바뀌어 스스로 승인할 수 없다. ([[결정 노트 - 2026-09-26-workflow-web-tasks]])
- 2026-09-26 종합 점검 반영으로 서버의 35분 제한·`expectedRevision` 검사·재시도 중복 검사(`retryOperationId`)·Gemini `mcp.allowed` 같은 중복 장치가 지워졌고, 접수 상태 파일은 `Saved/Workflow/test-feedback/`에 둔다. 위의 「35분이 지나면 실패」 관찰은 그 이전 코드 기준이다. ([[작업 - workflow-inspection]])
- 종합 점검 반영으로 작업 절차 도식은 자체 SVG→이미지 변환 대신 순정 `mermaid.run`으로 그리고, Gemini CLI가 AGENTS.md를 읽도록 `.gemini/settings.json`(`context.fileName: AGENTS.md`)이 더해졌다. ([[작업 - workflow-inspection]])
- 2026-09-26 작업 탭 분리 뒤 대시보드·작업 탭은 주소(#)가 달라 새로 고침과 뒤로 가기에도 탭과 연 작업이 유지되고, 문서와 구현 계획은 브라우저에서 vendor에 넣은 marked 18.0.14로 그리며 취소선은 `~~`만 인정한다. ([[작업 - dashboard-work-tab-split]])
- 2026-09-27 최종 마무리 수정(`7a1e8a0e4`)에서 실행 제한을 넘은 AI는 하위 프로세스까지 끝내고, 표 형식 오류가 있는 기록은 답변·승인·추가 요청·다시 시도를 받지 않으며(`tableError`), 완료된 작업의 추가 요청·테스트 결과·터미널은 서버도 거부한다. ([[작업 - workflow-final-fixes]])
- Codex 정하기(읽기 전용)는 `--disable plugins --disable apps`로 플러그인 서버를 끄고, `mcp list --json`에서 켜진 설정 MCP 서버만 골라 `-c mcp_servers.<이름>.enabled=false`로 끈다. `-c mcp_servers={}`는 병합이라 효과가 없다(2026-09-27 실측). ([[작업 - workflow-final-fixes]])
- 2026-09-27 하네스 정리로 지난 AI 작업이 이 PC의 `Saved/`에 남긴 파일 203개가 휴지통으로 옮겨졌고, 작업 기록 5개의 Git 밖 `Saved` 링크 73개는 경로 글자로 바뀌었다. ([[작업 - harness-legacy-cleanup]])

## 검증 범위

- workflow-review의 사람 항목 7개(코드 리뷰, 대시보드, 작업 절차 한 장, 체크리스트 전달, 웹 새 작업 흐름, 터미널 이어하기, SSoT)는 이우성이 통과로 확인했다. ([[작업 - workflow-review]])
- 2026-09-22 Workflow 정본·실행 경계 조사는 저장소 원문 발췌와 SHA-256 기록이며 서버·AI·게임 실행 검증을 포함하지 않는다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 Workflow closure 구현은 TestWorkflowExecution.cjs와 TestWikiViewer.cjs 모의 DOM 테스트로만 확인했고 실제 AI 호출·브라우저 육안·UE 실행은 하지 않았다. ([[결정 노트 - 2026-09-22-workflow-closure]])
- 2026-09-22 Workflow 대시보드는 모의 DOM 테스트로만 검증했고 실제 브라우저 육안·AI 실행 검증은 하지 않았다. ([[결정 노트 - 2026-09-22-workflow-dashboard]])
- 2026-09-22 기획서 검토 전환은 TestWikiViewer·TestWikiSpaces·TestWikiAI 모의 테스트로 확인했으며 실제 AI 응답 품질과 브라우저 육안 검증은 포함하지 않는다. ([[결정 노트 - 2026-09-22-workflow-review]])
- Workflow 편의 개선은 스크립트 자동 테스트·링크 검사·로컬 서버 재시작·헤드리스 캡처로 확인했고 실제 AI 처리 요청과 사람의 화면 확인은 하지 않았다. ([[결정 노트 - 2026-09-25-workflow-convenience-review]])
- 작업 기록 현황 표시는 TestWikiViewer 등 자동 테스트와 모의 DOM으로만 확인했고 실제 브라우저 확인은 로컬 파일 URL 보안 정책으로 수행하지 못했다. ([[결정 노트 - 2026-09-25-workflow-task-records]])
- 테스트 결과 접수 기능은 자동 테스트·모의 AI·실제 로컬 HTTP·모의 DOM으로 확인했고 실제 AI 결과 제출·브라우저 육안·UE 실행은 하지 않았다. ([[결정 노트 - 2026-09-25-workflow-test-feedback]])
- Workflow 개선 작업(workflow-review)의 사람 테스트 항목 7개는 2026-09-25T18:13Z 이우성이 모두 통과로 전달해, 웹 새 작업 실제 흐름과 화면이 사람에 의해 확인됐다. ([[결정 노트 - 2026-09-26-workflow-human-verification]])
- Workflow 사람 테스트 결과에는 사용한 AI 제공자·모델이 없으므로 Codex·Claude·Gemini 각각의 전체 흐름 통과로 해석하지 않는다. ([[결정 노트 - 2026-09-26-workflow-human-verification]])
- 한글 기록 이름은 TestWorkflowTestFeedback와 실제 cmd start 창의 인자 전달(AI 호출 없음)로 확인했다. ([[결정 노트 - 2026-09-26-workflow-korean-record-names]])
- 구 워크플로우 잔재 제거는 Export-Wiki 재생성, CheckWikiLinks, Workflow·Wiki 자동 테스트로 확인했고 로컬 서버는 재시작하지 않았다. ([[결정 노트 - 2026-09-26-workflow-legacy-removal]])
- SSoT 사본 정리는 프롬프트 문구 자동 테스트로만 확인했고 실제 AI 처리로 새 프롬프트 준수를 확인하지 않았다. ([[결정 노트 - 2026-09-26-workflow-ssot-copies]])
- 상태를 질문·계획·체크리스트로만 정하는 Workflow 재설계는 TestWorkflowTestFeedback·TestWorkflowFeedbackUI 자동 테스트로 확인했고 실제 AI 요청으로 다시 돌리지 않았다. ([[결정 노트 - 2026-09-26-workflow-state-from-record-only]])
- Workflow 웹 새 작업 기능은 자동 테스트와 가짜 Codex·인자 기록 스크립트로 실제 cmd 창을 확인했지만, 실제 AI 요청으로 새 작업을 끝까지 돌리지 않았다. ([[결정 노트 - 2026-09-26-workflow-web-tasks]])
- 종합 점검 반영은 워크플로우 Node 테스트·결함 주입·헤드리스 Edge·문서 링크·하네스 등 AI 항목 11개로 확인했고, 사람 항목 「대시보드 작업 흐름」은 이우성이 2026-09-26 AI 브라우저 점검 결과를 근거로 통과 처리했다("문제 없었으면 통과로 처리합시다"). ([[작업 - workflow-inspection]])
- 작업 탭 분리(체크리스트 7/7)와 최종 마무리 수정(6/6)은 모두 AI 항목(자동 테스트, 결함 주입 13건·44건, 헤드리스 Edge 1440px·375px)으로 통과했고, 사람의 화면 사용감 확인은 `workflow-wrapup-checks.md`로 옮겨져 이 수집 시점에 끝나지 않았다. ([[작업 - dashboard-work-tab-split]], [[작업 - workflow-final-fixes]])

## 미결정·충돌

- 여러 대화 세션이 한 체크아웃을 동시에 편집·빌드할 때의 조율 절차는 미해결이며 다른 작업의 코드 변경은 자동 감지되지 않는다. ([[작업 - workflow-review]])
- Workflow 단순화 시점에 웹 경로 전용 스크립트·테스트·옛 절차 문서는 자동 모드 권한 검사의 삭제 거부로 삭제 대기 상태였다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 대시보드 행 버튼 원자료는 리뷰·참고 행을 구현 설명에서는 기록 열기만, TestWikiViewer 설명에서는 버튼 없음으로 서로 다르게 적는다. ([[결정 노트 - 2026-09-26-workflow-dashboard-row-actions]])
- Workflow 사람 테스트 통과는 작업 절차 도식 점검에서 나온 미결 두 사항(정하기 중 추가 요청 권한, 완료 직후 추가 요청 재활성)의 조치 결정이 아니다. ([[결정 노트 - 2026-09-26-workflow-human-verification]])
- 결과 칸 노트의 사람 판단은 blockers로 받는다는 지시는 같은 날 상태 결정 재설계에서 blockers가 지시문에서 없어지며 대체됐다. ([[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]])
- Workflow 대시보드 리뷰·참고 행의 기록 열기 버튼 유무는 행 버튼 최종 결정 노트의 구현 설명(기록 열기만)과 테스트 기술(버튼 없음)이 다르게 적는다. ([[결정 노트 - 2026-09-26-workflow-row-actions-final]])
- 병렬 작업은 약속으로만 다루므로 같은 `.uasset`(LFS 잠금 없는 이진)·같은 작업 기록·같은 제목의 새 작업은 여전히 병합 충돌이 나고, 충돌 표식이 남은 기록은 대시보드가 사람 항목 없이 읽는다(2026-09-26 병합 흉내 실측). ([[작업 - workflow-inspection]])
- 프로젝트 `.codex/config.toml`의 unreal-mcp가 다른 PC의 Codex에서 `mcp list`에 나오는지는 2026-09-27 기준 확인하지 못했다. ([[작업 - workflow-final-fixes]])
- 워크플로우 개선 다섯 건의 남은 사람 확인(대시보드 사용감, 대시보드 Wiki 갱신, Wiki 정기 갱신, 휴지통 정리)은 확인 대기 기록 `workflow-wrapup-checks.md`에 있고, 완료 전이라 이 Wiki에 수집되지 않았다. ([[작업 - workflow-final-fixes]], [[작업 - workflow-inspection]], [[작업 - wiki-claude-obsidian-migration]], [[작업 - harness-legacy-cleanup]])

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
- [[결정 노트 - 2026-09-26-workflow-human-verification]] — Workflow 개선 작업의 사람 테스트 항목 7개를 이우성이 모두 통과로 전달한 결과와, 그 확인이 덮는 범위·해석 한계를 정리한 노트.
- [[결정 노트 - 2026-09-26-workflow-image-removal]] — 구 워크플로우 그림 삭제 뒤 쓰는 곳이 없던 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 사용자 결정으로 없애고 도식은 Mermaid만 쓰게 한 노트.
- [[결정 노트 - 2026-09-26-workflow-korean-record-names]] — 웹 새 작업의 기록 파일 이름을 접수 식별자 해시 대신 한글 제목으로 만들고, 같은 이름은 -2를 붙이며 터미널 이어하기의 영문·숫자 제한을 없앤 노트.
- [[결정 노트 - 2026-09-26-workflow-legacy-removal]] — 사용자 요청으로 옛 웹 작업 경로 스크립트·테스트, 한 장으로 합친 옛 절차 문서와 4단계 그림, 옛 결과 형식 표시와 대시보드 CSS를 지운 노트.
- [[결정 노트 - 2026-09-26-workflow-process-diagram]] — 작업 절차 도식을 정하기·만들기·확인하기 단계 상자 배치로 바꾸고 서버·화면 코드와 대조한 노트. 추가 요청 권한과 완료 뒤 재활성 두 가지가 미결이다.
- [[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]] — 첫 실제 Codex 처리에서 완료 문장이 구현 계획 절로 기록된 결함 뒤, 질문·새 계획을 정하기·추가 요청 결과에서만 받고 완료 행 배지를 없앤 노트.
- [[결정 노트 - 2026-09-26-workflow-row-actions-final]] — 대시보드 행 버튼 최종 결정으로 진행 중 기록에서도 기록 열기를 빼고, 배지 행의 버튼 줄 어긋남을 고정 폭 칸으로 고치며 사이드바 소개 문구를 없앤 노트.
- [[결정 노트 - 2026-09-26-workflow-ssot-copies]] — 워크플로우 SSoT를 엄격히 지키기로 한 사용자 결정에 따라 처리 프롬프트와 Wiki의 규칙 사본을 정리하고 정본을 가리키게 한 노트.
- [[결정 노트 - 2026-09-26-workflow-state-from-record-only]] — 첫 실제 웹 처리의 두 결함 뒤 AI가 넘기는 것을 질문·구현 계획·테스트 체크리스트로 한정하고, 상태는 이 세 절로만 정하며 완료는 전부 통과로 즉시 판정하게 한 노트.
- [[결정 노트 - 2026-09-26-workflow-tasks-guide-removed]] — 사용자 요청으로 대시보드 머리의 기록 작성 규칙 링크를 없애고 그 링크만 가리키던 기록 폴더 안내 문서 tasks/index.md를 삭제한 노트.
- [[결정 노트 - 2026-09-26-workflow-web-tasks]] — 대시보드에서 새 작업 시작과 질문 답변·구현 승인·추가 요청·테스트 결과 전달·터미널 이어하기를 하게 하고, AI 처리를 터미널 창에서 보이게 실행하도록 한 노트.
- [[작업 - workflow-review]] — AI 작업 워크플로우를 점검하고 작업 절차 한 장·세 단계·테스트 체크리스트·웹 새 작업과 이어하기로 단순화한 2026-09-22~26 완료 작업 기록
- [[작업 - dashboard-work-tab-split]] — 대시보드와 작업 탭을 나누고 작성자 칸·이어서 작업 용어·marked 렌더러로 바꾼 2026-09-26 완료 작업 기록
- [[작업 - harness-legacy-cleanup]] — 하네스·워크플로우·Wiki의 옛 흔적과 로컬 Saved 잔여 파일을 정리하고 워크플로우 스크립트 이름을 바꾼 2026-09-27 완료 작업 기록
- [[작업 - wiki-claude-obsidian-migration]] — 옛 LLM Wiki를 claude-obsidian vault로 바꾸고 정기·즉시 Wiki 갱신을 만든 2026-09-26 완료 작업 기록
- [[작업 - workflow-final-fixes]] — 최종 마무리 코드 리뷰 지적 수정과 실행 제한·자유 답변·기록 열기 결정을 담은 2026-09-27 완료 작업 기록
- [[작업 - workflow-inspection]] — 순정 우선·장치 축소·SSOT 기준의 워크플로우 종합 점검과 Q1~Q9 결정 반영을 담은 2026-09-26 완료 작업 기록
