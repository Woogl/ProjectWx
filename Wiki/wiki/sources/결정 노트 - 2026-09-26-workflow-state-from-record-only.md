---
type: source
title: "결정 노트 - 2026-09-26-workflow-state-from-record-only"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "첫 실제 웹 처리의 두 결함 뒤 AI가 넘기는 것을 질문·구현 계획·테스트 체크리스트로 한정하고, 상태는 이 세 절로만 정하며 완료는 전부 통과로 즉시 판정하게 한 노트."
source_type: decision-note
source_id: src-4d376dc7383e42ce7fe1
sha256: 76204d035faca280463478de8bd6a6c365ca5b6cac24edc8a94175e5c44eedf7
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-state-from-record-only.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-state-from-record-only.md"
raw_copy: ".raw/captured/76204d035faca280463478de8bd6a6c365ca5b6cac24edc8a94175e5c44eedf7.md"
claim_ids:
  - clm-5e19ce271d-c1
  - clm-5e19ce271d-c2
  - clm-5e19ce271d-c3
  - clm-5e19ce271d-c4
key_claims:
  - "2026-09-26 사용자 승인 뒤 Workflow 작업 상태는 기록의 질문·구현 계획·테스트 체크리스트 세 절에서만 정해진다."
  - "Workflow에서 테스트 체크리스트가 모두 통과하면 서버가 즉시 완료로 판정하고, 이어지는 AI 정리가 실패·중단해도 완료가 유지된다."
  - "이 결정으로 AI 결과 지시문에서 blockers·checks 칸이 없어져 무관한 lint 경고 같은 범위 밖 문제는 요약에만 적는다."
  - "바뀐 상태 판정 흐름은 자동 테스트로만 확인했고 실제 AI 요청으로는 다시 실행하지 않았다."
---

# 결정 노트 - 2026-09-26-workflow-state-from-record-only

- 원본: `.wiki/raw/notes/2026-09-26-workflow-state-from-record-only.md`
- 원자료 사본: `.raw/captured/76204d035faca280463478de8bd6a6c365ca5b6cac24edc8a94175e5c44eedf7.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-state-from-record-only.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- 2026-09-26 첫 실제 Codex 처리에서 무관한 lint 경고가 완료를 막고, AI의 완료 문장이 구현 계획 절로 들어갔다([[결정 노트 - 2026-09-26-workflow-result-fields-by-kind]]). AI가 "완료로 확정" 버튼을 제안했으나 사용자는 거절하고 흐름 자체를 다듬게 했다.
- **이전 규칙을 바꿈**: [[결정 노트 - 2026-09-26-workflow-web-tasks]]의 상태 판정 순서(막힘 blocker·실패 → 미답변 질문 → 미승인 계획 → 전부 통과 → 나머지)에서 blocker 통로를 없앴고, 결과 칸 노트의 "판단이 필요하면 blockers에" 지시도 대체했다.

## 사람의 판단 원문

> 사용자 2026-09-26: "이 과정에서 문제가 없도록 워크플로우를 직관적으로 다듬어야한다고 생각합니다."

> 사용자 2026-09-26: "AI가 무관한지 판단하는게 많은 실수를 일으킬까요?"

AI의 답을 들은 뒤:

> 사용자 2026-09-26: "네."

(승인)

## 원인 분석(AI 판단)

- AI 결과의 여러 칸(blockers·checks·구현 계획·질문)이 모두 상태를 바꾸는 통로라, 한 칸만 잘못 채워도 상태가 틀어졌다.
- 완료가 사람의 판단 뒤 AI 정리 처리의 성공에 걸려 있었다.
- 여러 세션이 한 체크아웃을 동시에 고쳐 git diff·빌드·lint에 다른 작업 변경이 섞이고, 작업별 변경 추적이 없어 AI가 관련 여부를 확실히 가를 수 없다. 첫 처리에서 AI는 무관함을 맞게 판단했지만 지시가 범위 밖 문제를 blockers에 넣게 해 완료가 막혔다.

## 바뀐 규칙(확정 결정)

- AI가 사람에게 넘기는 것은 질문(판단 필요, 선택지와 추천)·구현 계획(승인 필요)·테스트 체크리스트(테스트 필요) 셋뿐이다.
- 상태는 기록의 이 세 절에서만 정한다: 미답변 질문 → 질문 답변 필요, 미승인 계획 → 구현 승인 필요, 체크리스트 전부 통과 → 완료, 실패 행 → 실패 확인 필요, 나머지 → 사람 확인 필요. 셋 다 없으면 처리 결과 확인.
- 단계별 AI 결과 칸(모두 summary·evidence 포함): 정하기 = 질문·계획, 구현·수정 = 변경·질문·체크리스트, 추가 요청 = 변경·질문·계획·체크리스트, 정리 = 변경. CLI JSON 스키마도 단계별이고 서버는 허용된 칸만 읽는다.
- 테스트 결과 전달: 실패가 있으면 AI 수정, 실패 없이 일부 통과면 AI 없이 기록만, 모두 통과면 서버가 즉시 완료하고 AI 정리(Wiki 반영)를 맡긴다. 정리는 상태를 바꾸지 않으며 실패·중단해도 완료가 유지된다.
- AI의 미실행·대기 항목은 사람 항목으로 넘긴다(근거 "AI가 실행하지 못함: …"). 사람 항목이 없으면 결과 확인 항목을 더해 완료는 항상 사람의 확인으로 끝난다.
- 구현 중 판단이 필요하면 질문으로 묻고, 답하면 정하기로 돌아가 새 계획과 승인을 거친다.
- 처리 중 기록이 바뀌면 기록 충돌로 두고 AI 결과를 반영하지 않는다.
- 지시문에서 blockers·checks를 없앴다. 무관한 문제는 요약에 한 줄로만 알리고, 다른 작업 변경 때문으로 보이는 실패는 고치지 않는다.
- 이력: 사람 결과는 즉시 "사용자 테스트 결과"로, AI 결과는 단계별 제목(AI 조사 결과·AI 구현 결과·AI 추가 요청 처리·AI 수정 결과·AI 완료 정리)으로 남긴다.
- 규칙 문서는 작업 절차(`.agents/workflow/process/index.md`) 한 장에만 둔다. 같은 날 AGENTS.md와 tasks 안내는 링크만 두도록 바뀌었다([[결정 노트 - 2026-09-26-workflow-ssot-copies]]).

## 검증 범위

- 자동 테스트 TestWorkflowTestFeedback: 단계별 결과 칸, 즉시 완료와 상태를 못 바꾸는 정리, 일부 통과 기록만 처리, 실패 수정, 미실행 항목 넘김, 결과 확인 항목 추가, 기록 충돌, 구현 중 질문, 정리 실패·중단 뒤 완료 유지.
- 자동 테스트 TestWorkflowFeedbackUI: 즉시 완료 안내·완료 단계·결과 기록 안내·옛 처리 결과 표시.
- 바뀐 흐름을 실제 AI 요청으로는 다시 돌리지 않았다(가짜 입력 기반 자동 테스트만).

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-26 사용자 승인 뒤 Workflow 작업 상태는 기록의 질문·구현 계획·테스트 체크리스트 세 절에서만 정해진다. ^c1
- Workflow에서 테스트 체크리스트가 모두 통과하면 서버가 즉시 완료로 판정하고, 이어지는 AI 정리가 실패·중단해도 완료가 유지된다. ^c2
- 이 결정으로 AI 결과 지시문에서 blockers·checks 칸이 없어져 무관한 lint 경고 같은 범위 밖 문제는 요약에만 적는다. ^c3
- 바뀐 상태 판정 흐름은 자동 테스트로만 확인했고 실제 AI 요청으로는 다시 실행하지 않았다. ^c4
