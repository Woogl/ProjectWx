---
type: source
title: "결정 노트 - 2026-09-22-workflow-dashboard"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "UI"
summary: "Workflow 작업 현황 대시보드에서 단계별 진행 일감을 한눈에 보도록 한 2026-09-22 사용자 요청과 workflow.js 구현·모의 DOM 검증 범위 기록"
source_type: decision-note
source_id: src-73560212904f810d69bc
sha256: c7b89b299b5bd56fa064de5e8301ed48614bbb69ceab22d3dd0981fb0ce1ddf8
authority: primary
independence_key: ".wiki/raw/notes/2026-09-22-workflow-dashboard.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-22-workflow-dashboard.md"
raw_copy: ".raw/captured/c7b89b299b5bd56fa064de5e8301ed48614bbb69ceab22d3dd0981fb0ce1ddf8.md"
claim_ids:
  - clm-553dabec7a-c1
  - clm-553dabec7a-c2
  - clm-553dabec7a-c3
key_claims:
  - "2026-09-22 Workflow 대시보드는 공용 작업을 기획 검토·설계·구현·코드 리뷰·테스트·완료 단계로 분류해 개수·제목·상태를 표시하도록 구현됐다."
  - "2026-09-22 Workflow 대시보드의 완료 분류는 execution.status=complete인 테스트 수용을 뜻하며 Wiki 정리 완료를 뜻하지 않는다."
  - "2026-09-22 Workflow 대시보드 검증은 TestWikiViewer.cjs·TestWikiSpaces.cjs 모의 DOM 테스트이며 실제 브라우저 육안 검증은 포함하지 않는다."
---

# 결정 노트 - 2026-09-22-workflow-dashboard

- 원본: `.wiki/raw/notes/2026-09-22-workflow-dashboard.md`
- 원자료 사본: `.raw/captured/c7b89b299b5bd56fa064de5e8301ed48614bbb69ceab22d3dd0981fb0ce1ddf8.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-22-workflow-dashboard.md`(제목 "Workflow 단계별 일감 대시보드").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-22`, 태그 `wx, workflow`.
- 성격: 사용자 요청과 구현 관찰, 모의 검증 범위. 조사 시점(2026-09-22) 기준이라 현재 Workflow 뷰어와 다를 수 있다.

## 사용자 요청 (노트의 간접 기록)

> 노트 2026-09-22: "작업 현황 대시보드에서 각 단계별 진행 중인 일감을 한눈에 확인하도록 요청했다."

앞선 요청에서 탐색 메뉴를 **작업 현황 대시보드·새 작업 만들기·기존 작업 이어하기·사용 방법 안내·LLM 위키 검색**으로 확정했다고 노트는 적는다.

## 구현 관찰

- `.agents/scripts/wiki-viewer/workflow.js`의 `dashboardStage`, `dashboardStatus`, `renderWorkSummary`가 전체 공용 작업을 기획 검토·설계·구현·코드 리뷰·테스트·완료로 분류하고 개수·제목·상태를 표시한다.
- `changeTo`가 있는 작업은 후속 변경으로 보고 인계 구역에 둔다. 확정 여부는 `loopConfirmed`로 유효성을 검사한다. verify 단계에서 중단된 작업도 테스트 구역에 남는다.
- 완료는 `execution.status=complete`, 즉 **테스트 수용**이며 Wiki 정리 완료를 뜻하지 않는다.
- 카드를 누르면 현재 입력을 저장하고 공용 상태를 다시 조회한 뒤 작업을 연다. 연결 전 상태와 실제로 일감이 없는 상태를 구분한다.

## 검증 범위

- `TestWikiViewer.cjs`, `TestWikiSpaces.cjs` 통과. 단계별 분류·중단 검증·변경 인계·기획 수정으로 인한 확정 무효·빈 대시보드를 **모의 DOM**에서 확인했다.
- 실제 브라우저 육안·실제 AI 실행 검증은 포함하지 않는다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-22 Workflow 대시보드는 공용 작업을 기획 검토·설계·구현·코드 리뷰·테스트·완료 단계로 분류해 개수·제목·상태를 표시하도록 구현됐다. ^c1
- 2026-09-22 Workflow 대시보드의 완료 분류는 execution.status=complete인 테스트 수용을 뜻하며 Wiki 정리 완료를 뜻하지 않는다. ^c2
- 2026-09-22 Workflow 대시보드 검증은 TestWikiViewer.cjs·TestWikiSpaces.cjs 모의 DOM 테스트이며 실제 브라우저 육안 검증은 포함하지 않는다. ^c3
