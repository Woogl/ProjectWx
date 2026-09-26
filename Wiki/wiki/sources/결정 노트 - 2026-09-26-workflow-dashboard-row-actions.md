---
type: source
title: "결정 노트 - 2026-09-26-workflow-dashboard-row-actions"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "대시보드"
summary: "Workflow 대시보드 목록 행 버튼을 분류별로 정리한 사용자 결정과 구현: 확인 대기는 작업 진행만, 완료·리뷰·참고는 기록 열기만"
source_type: decision-note
source_id: src-8afcfcdf618feadc354b
sha256: 2323f8c4675818aae7d1faa639f3951c771dd4757dd1edfaea84fd2bced8f009
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-dashboard-row-actions.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-dashboard-row-actions.md"
raw_copy: ".raw/captured/2323f8c4675818aae7d1faa639f3951c771dd4757dd1edfaea84fd2bced8f009.md"
claim_ids:
  - clm-c6db6d776e-c1
  - clm-c6db6d776e-c2
  - clm-c6db6d776e-c3
key_claims:
  - "사용자 결정에 따라 Workflow 대시보드의 확인 대기 행에는 작업 진행 버튼만 둔다."
  - "Workflow 대시보드의 완료 행에는 기록 열기만 두며, 웹에서는 완료 기록에 테스트 결과를 다시 보낼 수 없고 새 문제는 새 작업으로 요청한다."
  - "Workflow 대시보드의 리뷰·참고 행은 기록 열기 버튼만 가진다."
---

# 결정 노트 - 2026-09-26-workflow-dashboard-row-actions

- 원본: `.wiki/raw/notes/2026-09-26-workflow-dashboard-row-actions.md`
- 원자료 사본: `.raw/captured/2323f8c4675818aae7d1faa639f3951c771dd4757dd1edfaea84fd2bced8f009.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `2026-09-26-workflow-dashboard-row-actions.md`(제목 "Workflow 대시보드 분류별 기록 버튼", 출처 `MANUAL`, 수집일 2026-09-26).
- 내용: 사용자가 웹 새 작업을 테스트하며 내린 결정에 따라 대시보드 목록 행의 버튼을 분류별로 줄였다. 완료 작업의 새 문제는 새 작업으로 요청한다.

## 사람의 판단 원문

> 사용자 2026-09-26: "확인 대기에서는 기록 열기 버튼 없어도 될 거 같아요"
> 사용자 2026-09-26: "완료에서는 작업 진행 버튼이 없어도 되구요"

## 구현 관찰

- `.agents/scripts/wiki-viewer/workflow.js`의 목록 행 버튼: 확인 대기는 작업 진행, 진행 중은 기록 열기와 작업 진행, 완료와 리뷰·참고는 기록 열기만.
- 생성 문서에 아직 없는 새 기록의 "OpenWorkflow.bat을 다시 실행하면 열립니다" 안내도 기록 열기와 같은 분류에만 나온다.
- 완료 목록 안내에 "새 문제는 새 작업으로 요청하세요"를 더했다. 웹에서는 완료 기록에 테스트 결과를 다시 보낼 수 없고, 로컬 서버의 동작은 바꾸지 않았다.

## 검증 범위

- TestWikiViewer: 확인 대기 행은 작업 진행 버튼만, 진행 중 행은 기록 열기와 작업 진행, 완료 행은 기록 열기만, 리뷰·참고 행은 버튼 없음을 확인했다.
- 실제 브라우저 확인 기록은 원자료에 없다.

## 미결정·충돌

- 원자료 안에서 리뷰·참고 행의 버튼 서술이 엇갈린다. 구현 관찰은 "완료와 리뷰·참고는 기록 열기만"이라고 쓰고, 테스트 서술은 "리뷰·참고 행은 버튼 없음"이라고 쓴다. 어느 쪽이 실제 동작인지는 이 원자료만으로 확정할 수 없다.

## 관련 주제

- [[작업 절차(Workflow)]]
- <!--wl-->결정 노트 - 2026-09-26-workflow-row-actions-final

## 핵심 주장

- 사용자 결정에 따라 Workflow 대시보드의 확인 대기 행에는 작업 진행 버튼만 둔다. ^c1
- Workflow 대시보드의 완료 행에는 기록 열기만 두며, 웹에서는 완료 기록에 테스트 결과를 다시 보낼 수 없고 새 문제는 새 작업으로 요청한다. ^c2
- Workflow 대시보드의 리뷰·참고 행은 기록 열기 버튼만 가진다. ^c3
