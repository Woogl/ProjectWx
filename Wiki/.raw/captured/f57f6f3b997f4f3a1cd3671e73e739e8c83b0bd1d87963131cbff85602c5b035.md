---
title: "Workflow 작업 기록 현황 표시"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, workflow]
summary: "기존 Workflow 대시보드에서 대화 작업의 확인 범위와 다음 행동을 분류별로 조회하는 변경"
---

# 요청과 확인 범위

사용자는 작업 목차를 파악하기 어렵다고 하며 개선을 요청했고, 표시 위치 질문에 "기존 Workflow 웹 화면에서 보기"를 선택했다. 기준 HEAD는 `38d4dde08`이며 아래 미커밋 구현을 확인했다. 다른 세션의 기존 변경은 보존했다.

# 구현 관찰

- `.agents/workflow/tasks/index.md`의 네 표가 대화 작업 현황의 표시 원본이다. 플레이 확인·에디터 확인·개선 판단·완료 기록으로 나누며, 각 행에 상세 Task 링크·확인된 범위·다음 행동을 둔다.
- `.agents/scripts/wiki-viewer/workflow.js`의 `taskRecordGroups`가 생성 문서에 포함된 목차를 읽고, `renderTaskRecords`가 분류 버튼·건수·상세 기록 링크를 표시한다. 기존 웹 작업의 JSON 상태·승인·저장 키를 바꾸지 않는다.
- 생성된 기록은 AI 서버 연결 전에도 볼 수 있다. 웹 작업이 없는 경우 빈 단계 칸을 만들지 않는다. 새 작업 만들기·기존 작업 이어하기에서는 기록 현황을 숨긴다.
- `.agents/workflow/process/index.md`에 단계 완료·인계 시 상세 Task와 목차의 분류·확인 범위·다음 행동을 함께 갱신하도록 기록했다. 표시 내용은 기존 후속 기록의 요약이며 현재 코드의 재검증이나 새로운 승인이 아니다.
- `OpenWorkflow.bat`의 기존 내보내기 경로로 표시 내용을 갱신한다. 웹 작업의 실행 상태는 기존 공용 작업 API에서 계속 조회한다.

# 입력 식별

| 파일 | SHA-256 |
| --- | --- |
| `.agents/scripts/wiki-viewer/workflow.js` | `9D35BB0DD5FE3F12B3352CF4F95554F2D8ADE3260BEF3F7C7523E1B332629AAE` |
| `.agents/scripts/wiki-viewer/index.html` | `7A64D3A5D61AF343FA5D499A747AC9110BF78293349F1236BAA683F7AE7A060E` |
| `.agents/workflow/tasks/index.md` | `20D35D18AFB91BFB59071CF50CCC519D61F0FFF60DC0801B42FB7456AD01F84A` |

# 검증과 제한

TestWikiViewer·TestWikiSpaces·TestWikiTasks·TestWorkflowExecution 통과. 모의 DOM에서 분류 전환·링크·21개 기록 누락/중복·서버 연결 전 표시·승인 상태 불변·잘못된 링크 제외·생성/재개 경로와 기존 저장·실행 회귀를 확인했다. 실제 브라우저 확인은 로컬 파일 URL 보안 정책으로 차단되어 수행하지 못했다. 게임 실행·에셋·실제 AI 호출 검증은 이번 범위 밖이다.
