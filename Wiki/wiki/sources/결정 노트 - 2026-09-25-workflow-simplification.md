---
type: source
title: "결정 노트 - 2026-09-25-workflow-simplification"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지하며 AI·사람 담당 테스트 체크리스트를 도입한 기록"
source_type: decision-note
source_id: src-e19019bb10553c1e2211
sha256: dc7d6f746e1d0608e1f778cf3189efd843277a5f7dc7b66fe22bc402b37049e1
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-workflow-simplification.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-workflow-simplification.md"
raw_copy: ".raw/captured/dc7d6f746e1d0608e1f778cf3189efd843277a5f7dc7b66fe22bc402b37049e1.md"
claim_ids:
  - clm-ad92d94a05-c1
  - clm-ad92d94a05-c2
  - clm-ad92d94a05-c3
  - clm-ad92d94a05-c4
key_claims:
  - "2026-09-25 사용자 승인으로 Workflow는 정하기·만들기·확인하기 3단계와 확인 대기·진행 중·완료 상태 3개로 단순화되었다."
  - "정하기 단계는 AI의 추가 질문이 더 없을 때 사람이 구현을 승인해야 만들기 단계로 넘어간다."
  - "확인하기 단계는 담당을 AI·사람으로 나눈 테스트 체크리스트 표로 진행하며 서버의 guardChecklist가 AI의 사람 항목 변경을 되돌린다."
  - "단순화 작업 시점에 웹 새 작업 경로 전용 파일들은 자동 모드 권한 검사가 삭제를 거부해 삭제 대기로 남았다."
---

# 결정 노트 - 2026-09-25-workflow-simplification

- 원본: `.wiki/raw/notes/2026-09-25-workflow-simplification.md`
- 원자료 사본: `.raw/captured/dc7d6f746e1d0608e1f778cf3189efd843277a5f7dc7b66fe22bc402b37049e1.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-workflow-simplification.md`(제목 "Workflow 3단계 단순화와 테스트 체크리스트", source `MANUAL`, ingested 2026-09-25)를 요약한다. 기준 HEAD `c9e2efec6`에 미커밋 작업 트리를 포함한 기록이다. 노트 날짜 기준이라 현재 절차·스크립트와 다를 수 있으며(곧이어 [[결정 노트 - 2026-09-25-workflow-convenience-review]]에서 일부가 바뀜), 현재 정본은 `.agents/workflow/process/index.md`다.

## 사람의 판단 원문

사용자는 워크플로우 도식 설명을 받은 뒤 다음처럼 말했다.

> 사용자 2026-09-25: "좀 복잡하다", "한눈에 파악되는 직관적인 워크플로우를 원한다"

AI가 제시한 다섯 제안(웹 새 작업 경로 폐지, 사람 판단 지점 축소, 작은 수정의 진입 기준, 상태 3개, 규칙 한 장)에 대한 답:

> 사용자 2026-09-25: "좋네요. 워크플로우를 직관적이고 단순하게 해주세요. 그래야 문제가 있을 때 해결도 쉽죠."

같은 작업 중 추가 요청(노트 서술):

- 정하기 → 만들기: AI가 구현에 필요한 질문을 꼼꼼하게 하고, 더 이상 추가 질문이 없을 때 사람이 구현을 승인해야 구현 단계로 넘어간다.
- 만들기 → 확인하기: AI가 테스트 체크리스트를 만들고 스스로 테스트할 수 있는 부분은 직접 테스트해 반영한다. AI가 테스트할 수 없는 부분은 사람이 직접 테스트한 뒤 체크리스트에 반영할 수 있게 제공한다.

## 판단 근거(관찰)

- 웹 새 작업 경로의 저장 파일(`.agents/workflow/tasks/workflow_*`)은 0건이었고 작업 기록 21건은 모두 대화로 진행됐다.
- 이상 없음/이상 있음 처리 규칙이 네 문서에 중복됐다.
- 사람 확정 지점이 다섯 곳(검토본·설계·코드 리뷰·테스트 수용·정리 완료), 상태 용어가 일곱 가지 이상이었다.
- 2026-09-18 이후 커밋 204개 중 53개가 `.agents`·`.wiki`·AGENTS.md만 바꿨다.

## 구현 관찰

- 절차: `.agents/workflow/process/index.md` 한 장에 도식·단계표·체크리스트 형식·상태·문제 처리·결과 전달·기록 규칙을 둔다. 사람 판단은 정하기의 답변과 구현 승인, 만들기의 코드 리뷰, 확인하기의 사람 항목 테스트였다(코드 리뷰 위치는 후속 점검에서 확인하기 체크리스트로 이동). 구현 내용을 명확히 지시한 요청은 구현 승인으로 보고, 승인된 범위 안의 수정은 만들기부터 시작한다.
- 상태: 확인 대기·진행 중·완료 세 개.
- 테스트 체크리스트: 작업 기록의 `## 테스트 체크리스트` 절에 `| 항목 | 확인 방법 | 담당 | 결과 | 근거 |` 표. 담당은 AI·사람, 결과는 대기·통과·실패·미실행.
- 제출: 대시보드 체크리스트 창이 사람 항목(이번에 확인 안 함·통과·실패, 실패 시 문제 상황 필수)을 받고 서버가 즉시 기록 표에 쓴 뒤 AI 처리를 시작한다. `guardChecklist`는 AI가 사람 항목을 통과로 바꾸거나 지우거나 담당을 바꾸면 되돌린다. 결과는 확인 필요(blocked)·재확인(retest)·완료로 분류한다.
- 로컬 서버 `Wiki-AI.cjs`는 `/health`와 `/test-feedback`만 받는다(protocol 4). `/analyze`·`/handoff`·`/tasks`·`/execution`·`/import` 등을 없앴다.
- AI 실행 권한은 한 가지(Codex workspace-write, Claude acceptEdits, Gemini auto_edit)만 쓴다.
- 화면 메뉴는 작업 현황 대시보드·작업 절차·LLM 위키 검색 세 개다.

## 미결정·충돌

- 자동 모드 권한 검사가 파일 삭제를 거부해 웹 경로 전용 파일(`Wiki-Tasks.cjs`, `Workflow-Execution.cjs`, `Wiki-Import.cjs` 등)과 테스트 5개, 옛 절차 문서 4개·사용 방법 안내·4단계 그림이 삭제 대기로 남았다. 남은 코드는 이 파일들을 참조하지 않는다.

## 검증 범위

- TestWorkflowTestFeedback, TestWorkflowFeedbackUI, TestWikiProviders, TestWikiViewer·TestWikiSpaces 통과. CheckWikiLinks 오류 0. `Start-WikiAI.ps1`로 서버를 재시작해 protocol 4 확인. 헤드리스 Edge 캡처로 도식·대시보드·체크리스트 창 표시 확인.
- 실제 AI 처리 요청과 사람의 화면 확인은 하지 않았다. 삭제 대기 테스트 5개는 제거된 함수를 불러 실패하므로 실행하지 않았다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]
- [[결정 노트 - 2026-09-22-current-workflow]]
- [[결정 노트 - 2026-09-26-workflow-legacy-removal]]

## 핵심 주장

- 2026-09-25 사용자 승인으로 Workflow는 정하기·만들기·확인하기 3단계와 확인 대기·진행 중·완료 상태 3개로 단순화되었다. ^c1
- 정하기 단계는 AI의 추가 질문이 더 없을 때 사람이 구현을 승인해야 만들기 단계로 넘어간다. ^c2
- 확인하기 단계는 담당을 AI·사람으로 나눈 테스트 체크리스트 표로 진행하며 서버의 guardChecklist가 AI의 사람 항목 변경을 되돌린다. ^c3
- 단순화 작업 시점에 웹 새 작업 경로 전용 파일들은 자동 모드 권한 검사가 삭제를 거부해 삭제 대기로 남았다. ^c4
