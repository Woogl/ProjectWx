---
type: source
title: "결정 노트 - 2026-09-26-workflow-result-fields-by-kind"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "첫 실제 Codex 처리에서 완료 문장이 구현 계획 절로 기록된 결함 뒤, 질문·새 계획을 정하기·추가 요청 결과에서만 받고 완료 행 배지를 없앤 노트."
source_type: decision-note
source_id: src-e9ce44e694f9ea0a01fa
sha256: 0f3e2ab3bb15182b3c433c8fb745957174d2ea6094c27b24df494a8e99bc20b0
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-result-fields-by-kind.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-result-fields-by-kind.md"
raw_copy: ".raw/captured/0f3e2ab3bb15182b3c433c8fb745957174d2ea6094c27b24df494a8e99bc20b0.md"
claim_ids:
  - clm-59a4d3a7e7-c1
  - clm-59a4d3a7e7-c2
  - clm-59a4d3a7e7-c3
key_claims:
  - "2026-09-26 첫 실제 Codex 처리에서 Codex가 plan 칸에 완료 문장을 채웠고 서버는 이를 승인 줄 없는 구현 계획 절로 기록했다."
  - "조치 뒤 Workflow 서버는 질문과 새 계획을 정하기와 추가 요청 결과에서만 받고 구현·테스트 결과 처리에서 온 것은 쓰지 않는다."
  - "Workflow 대시보드의 완료·리뷰·참고 행에는 AI 처리 상태 배지를 표시하지 않는다."
---

# 결정 노트 - 2026-09-26-workflow-result-fields-by-kind

- 원본: `.wiki/raw/notes/2026-09-26-workflow-result-fields-by-kind.md`
- 원자료 사본: `.raw/captured/0f3e2ab3bb15182b3c433c8fb745957174d2ea6094c27b24df494a8e99bc20b0.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-result-fields-by-kind.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- 2026-09-26 첫 실제 Codex 처리([[작업 - animnotify-categories]]의 테스트 결과)에서 드러난 결과 칸 결함과 조치를 적었다. 같은 결함에서 출발한 더 넓은 재설계는 [[결정 노트 - 2026-09-26-workflow-state-from-record-only]]에 있다.

## 관찰(실제 실행에서 본 것)

- 결과 스키마가 모든 칸을 요구해 Codex가 `plan`에 "이번 작업은 완료입니다. 서버가 완료 상태와 처리 결과를 기록합니다."를 채웠다.
- 서버는 동작 종류와 관계없이 계획이 있으면 `## 구현 계획` 절(승인 줄 없음)로 썼다. 이번에는 무관한 lint 경고 blocker로 확인 필요가 먼저 판정돼 드러나지 않았지만, 아니었다면 "구현 승인 필요"와 구현 승인 버튼이 나오는 가짜 단계가 됐다.
- 테스트 결과 처리 지시에는 questions·plan을 비우라는 문장이 없었다.

## 조치(확정·구현)

- 서버는 질문과 새 계획을 정하기(새 작업·질문 답변)와 추가 요청 결과에서만 받는다. 구현·테스트 결과 처리에서 온 질문·계획은 쓰지 않아, AI가 지시를 어겨도 기록이 바뀌지 않는다.
- 이 노트 시점의 테스트 결과 처리 지시: "사람의 판단이 필요하면 blockers에, questions는 비우고 plan은 빈 문자열로". **이후 바뀜**: 같은 날 state-from-record-only 노트에서 지시문의 blockers·checks가 없어지고 단계별 결과 칸이 다시 정해졌다.
- 대시보드 목록의 완료·리뷰·참고 행에는 AI 처리 상태 배지를 붙이지 않는다. 사람이 완료로 확정한 기록에 마지막 AI 상태("AI 확인 요청")가 남아 어긋나 보였기 때문이다. 배지는 확인 대기·진행 중 행에만 있다.

## 사람의 판단 원문

> 사용자 2026-09-26: "지금 완료로 바꿔주세요."

이 요청으로 AnimNotify 기록의 잘못 들어간 절을 지우고 상태를 완료로 바꿨다. 확정 근거는 그 기록 이력에 남겼다.

## 검증 범위

- 자동 테스트 TestWorkflowTestFeedback: 테스트 결과 처리·구현 결과에 계획·질문이 있어도 기록되지 않고 승인이 유지된다.
- 자동 테스트 TestWikiViewer: 확인 대기 행은 작업 진행과 AI 상태 배지, 진행 중 행은 작업 진행, 완료 행은 기록 열기만.
- 조치 뒤 실제 AI 처리로 다시 확인한 기록은 이 노트에 없다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-26 첫 실제 Codex 처리에서 Codex가 plan 칸에 완료 문장을 채웠고 서버는 이를 승인 줄 없는 구현 계획 절로 기록했다. ^c1
- 조치 뒤 Workflow 서버는 질문과 새 계획을 정하기와 추가 요청 결과에서만 받고 구현·테스트 결과 처리에서 온 것은 쓰지 않는다. ^c2
- Workflow 대시보드의 완료·리뷰·참고 행에는 AI 처리 상태 배지를 표시하지 않는다. ^c3
