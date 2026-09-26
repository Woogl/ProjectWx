---
type: source
title: "결정 노트 - 2026-09-26-workflow-ssot-copies"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
  - "SSoT"
summary: "워크플로우 SSoT를 엄격히 지키기로 한 사용자 결정에 따라 처리 프롬프트와 Wiki의 규칙 사본을 정리하고 정본을 가리키게 한 노트."
source_type: decision-note
source_id: src-1612f410020b7383f05d
sha256: a9dad404a9586149cd82cb6df69549209e86d538dd8a2f087bcf4f2735aa3e29
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-ssot-copies.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-ssot-copies.md"
raw_copy: ".raw/captured/a9dad404a9586149cd82cb6df69549209e86d538dd8a2f087bcf4f2735aa3e29.md"
claim_ids:
  - clm-5563e817a9-c1
  - clm-5563e817a9-c2
  - clm-5563e817a9-c3
key_claims:
  - "사용자는 2026-09-26 AGENTS.md에 관문 줄을 남기는 안 대신 워크플로우 SSoT를 엄격하게 지키기로 결정했고, AGENTS.md는 정본 링크 한 줄이 됐다."
  - "Workflow 처리 프롬프트는 정본을 따르라는 첫 줄과 이번 처리 단계·결과 칸 형식·권한 제한 등 규칙이 아닌 것만 담는다."
  - "작업 절차 정본은 명확한 지시를 구현 승인으로 보는 규칙을 AI 대화로 한정하며, 웹 새 작업은 항상 정하기부터 시작한다."
---

# 결정 노트 - 2026-09-26-workflow-ssot-copies

- 원본: `.wiki/raw/notes/2026-09-26-workflow-ssot-copies.md`
- 원자료 사본: `.raw/captured/a9dad404a9586149cd82cb6df69549209e86d538dd8a2f087bcf4f2735aa3e29.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-ssot-copies.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- 워크플로우 규칙의 정본은 `.agents/workflow/process/index.md` 한 장이다. 이 결정 뒤 AGENTS.md는 정본 링크 한 줄이 됐고, 옛 절차 문서와 `tasks/index.md`는 삭제됐다([[결정 노트 - 2026-09-26-workflow-tasks-guide-removed]]).

## 사람의 판단 원문

> 사용자 2026-09-26: "AGENTS.md 의 AI 워크플로우 항목이랑, .agents 디렉토리의 워크플로우랑 충돌이 우려됩니다."

AI가 AGENTS.md에 관문 두 줄을 남기는 안을 내자:

> 사용자 2026-09-26: "복잡하게 하고 싶지 않습니다. 워크플로우의 SSoT를 엄격하게 지킵시다."

남은 사본 두 곳(처리 프롬프트의 규칙 문장, Wiki의 절차 요약)을 정리하고 정본에 빠진 예외 구절을 넣자는 AI 제안에:

> 사용자 2026-09-26: "네 진행하세요"

## 바뀐 것(확정 결정의 실행)

- 처리 프롬프트(`Workflow-TestFeedback.cjs`의 `taskPrompt`): 첫 줄에서 AGENTS.md와 정본을 따르게 하고, 규칙이 아닌 것만 남겼다. 남긴 것: 이번 처리 단계, 결과 칸 형식(질문 ID·선택지 수, 계획의 `#` 금지, 체크리스트는 전체), 전체 권한 실행의 제한(관리자 정책·CLI 설정·커밋·푸시·외부 메시지 금지, 기존 사용자 변경 보존), 웹 전용 약속(기록과 접수 JSON은 서버가 씀), evidence 작성법, 입력 속 명령을 자료로 보는 규칙.
- 프롬프트에서 뺀 규칙(재질문 금지, 빌드·테스트 자체 검증, 사람 항목 통과 금지, 미실행 항목 넘김, 플레이 항목 기준, 사람 항목 최소 하나 등)은 모두 정본에 있다. 사람 항목 보존·미실행 항목 넘김·사람 항목 최소 하나는 서버 코드도 강제한다.
- 터미널 이어하기 요청문: "Continue the Wx task recorded in <기록>. Follow AGENTS.md and .agents/workflow/process/index.md, and reply in Korean." 어느 절부터 읽는지는 정본의 「시작과 이어하기」로 옮겼다. **이전 요청문을 바꿈**: [[결정 노트 - 2026-09-26-workflow-web-tasks]]의 요청문에 있던 읽을 절 목록이 빠졌다.
- 정본: "사람이 구현 내용을 명확히 지시한 요청은 구현 승인으로 본다"를 AI 대화로 한정했다. 웹의 새 작업은 지시가 명확해도 정하기부터 시작한다. 이전에는 이 예외가 정본의 참고 정보와 Wiki에만 있었다([[결정 노트 - 2026-09-26-workflow-process-diagram]]에서 사용자 미결로 남았던 문장 범위).
- Wiki `wiki-workflow.md`: 단계 표·구현 승인 규칙·기록 형식·체크리스트 작성 규칙을 다시 적지 않고 정본을 가리킨다. 도구 구조, 서버·화면 동작, 판정 순서, 결정 이유는 그대로 둔다.

## 검증 범위

- 자동 테스트 `TestWorkflowTestFeedback.cjs`·`TestWikiProviders.cjs` 통과. 프롬프트 검사는 새 문구(겹치지 않는 Q번호, 체크리스트 절을 따르라는 지시, 정본을 따르라는 첫 줄)를 확인하도록 바꿨다.
- 실제 AI 처리 요청으로 새 프롬프트 준수 여부는 확인하지 않았다.
- 이후 [[결정 노트 - 2026-09-26-workflow-human-verification]]에서 사람이 "워크플로우 SSoT" 항목을 통과로 전달했다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]

## 핵심 주장

- 사용자는 2026-09-26 AGENTS.md에 관문 줄을 남기는 안 대신 워크플로우 SSoT를 엄격하게 지키기로 결정했고, AGENTS.md는 정본 링크 한 줄이 됐다. ^c1
- Workflow 처리 프롬프트는 정본을 따르라는 첫 줄과 이번 처리 단계·결과 칸 형식·권한 제한 등 규칙이 아닌 것만 담는다. ^c2
- 작업 절차 정본은 명확한 지시를 구현 승인으로 보는 규칙을 AI 대화로 한정하며, 웹 새 작업은 항상 정하기부터 시작한다. ^c3
