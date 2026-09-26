---
title: "Workflow 단계별 일감 대시보드"
source: "MANUAL"
type: "notes"
ingested: "2026-09-22"
tags: [wx, workflow]
summary: "사용자 요청과 단계별 대시보드 구현·검증 범위"
---

# 사용자 요청

작업 현황 대시보드에서 각 단계별 진행 중인 일감을 한눈에 확인하도록 요청했다. 앞선 요청에서 탐색 메뉴를 작업 현황 대시보드·새 작업 만들기·기존 작업 이어하기·사용 방법 안내·LLM 위키 검색으로 확정했다.

# 구현 관찰

.agents/scripts/wiki-viewer/workflow.js의 dashboardStage, dashboardStatus, renderWorkSummary를 확인했다. 전체 공용 작업을 기획 검토·설계·구현·코드 리뷰·테스트·완료로 분류하고 개수·제목·상태를 표시한다. changeTo가 있는 작업은 후속 변경으로 인계 구역에 둔다. 확정은 loopConfirmed로 유효성을 검사한다. verify 단계의 중단도 테스트에 남는다. 완료는 execution.status=complete, 즉 테스트 수용이며 Wiki 정리 완료를 의미하지 않는다.

카드는 현재 입력 저장과 공용 상태 재조회 후 작업을 연다. 연결 전 상태와 실제 일감 없음은 구분한다.

# 검증 범위

TestWikiViewer.cjs와 TestWikiSpaces.cjs 통과. 단계별 분류·중단 검증·변경 인계·기획 수정으로 인한 확정 무효·빈 대시보드를 모의 DOM에서 확인했다. 실제 브라우저 육안·실제 AI 실행 검증은 포함하지 않는다.
