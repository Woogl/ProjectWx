---
type: source
title: "결정 노트 - 2026-09-22-workflow-review"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "기획자에게 툴 난도가 높다는 이유로 Workflow 기획 단계를 기획서 검토 단계로 바꾸고 Workflow 문서 검색을 제거한 2026-09-22 사용자 결정 기록"
source_type: decision-note
source_id: src-7f8afee49742fa7bf2c8
sha256: d6af9f837b5beff13e4a4c35a80302349686153ab247c462559ae91d771f0574
authority: primary
independence_key: ".wiki/raw/notes/2026-09-22-workflow-review.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-22-workflow-review.md"
raw_copy: ".raw/captured/d6af9f837b5beff13e4a4c35a80302349686153ab247c462559ae91d771f0574.md"
claim_ids:
  - clm-0e01aa4f14-c1
  - clm-0e01aa4f14-c2
  - clm-0e01aa4f14-c3
key_claims:
  - "사용자는 2026-09-22 기획자에게 툴 난도가 높다는 이유로 Workflow의 기획 단계를 기획서 검토 단계로 바꾸도록 요청했다."
  - "2026-09-22 Workflow 기획서 검토 전환에서 AI는 없는 기획이나 수치를 임의로 작성하지 않고 누락·충돌·구현 영향만 조사하도록 정해졌다."
  - "2026-09-22 결정으로 Workflow의 문서 검색 진입은 제거하고 Wiki 검색은 유지했다."
---

# 결정 노트 - 2026-09-22-workflow-review

- 원본: `.wiki/raw/notes/2026-09-22-workflow-review.md`
- 원자료 사본: `.raw/captured/d6af9f837b5beff13e4a4c35a80302349686153ab247c462559ae91d771f0574.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-22-workflow-review.md`(제목 "Workflow 기획서 검토 전환과 검색 제거").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-22`, 태그 `wx, workflow`.
- 성격: 사용자 결정과 반영 범위, 모의 검증. 같은 주제의 작업 기록은 [[작업 - workflow-review]]에 있다. 조사 시점 기준이라 현재 Workflow와 다를 수 있다.

## 사람의 판단 (노트의 간접 기록)

노트는 사용자의 말을 직접 따옴표로 남기지 않았다. 노트 문장 그대로:

> 노트 2026-09-22: "2026-09-22 대화에서 사용자는 기획자에게 제공하기에는 툴의 난도가 높으므로 기획 단계를 기획서 검토 단계로 바꾸고, Workflow의 문서 검색을 제거하도록 요청했다."

## 확정 결정과 반영 범위

- 개발자가 외부에서 작성된 기획서를 입력하고, AI가 누락·충돌·구현 영향을 조사한다.
- 기획 판단은 담당자와 확인한 답변으로 기록하고 검토본을 확정해 설계로 넘긴다.
- AI는 없는 기획이나 수치를 임의로 작성하지 않는다.
- Workflow의 검색 링크·단축키·검색 주소 진입을 제거한다. Wiki 검색은 유지한다.
- 기존 `planning` 저장 키와 확정 인계 구조는 유지한다.

## 검증 범위

- `TestWikiViewer.cjs`, `TestWikiSpaces.cjs`, `TestWikiAI.cjs` 통과.
- 모의 UI·저장·인계 검증이며 실제 AI 응답 품질과 브라우저 육안 검증은 포함하지 않는다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 사용자는 2026-09-22 기획자에게 툴 난도가 높다는 이유로 Workflow의 기획 단계를 기획서 검토 단계로 바꾸도록 요청했다. ^c1
- 2026-09-22 Workflow 기획서 검토 전환에서 AI는 없는 기획이나 수치를 임의로 작성하지 않고 누락·충돌·구현 영향만 조사하도록 정해졌다. ^c2
- 2026-09-22 결정으로 Workflow의 문서 검색 진입은 제거하고 Wiki 검색은 유지했다. ^c3
