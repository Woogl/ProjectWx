---
type: source
title: "결정 노트 - 2026-09-26-workflow-tasks-guide-removed"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "사용자 요청으로 대시보드 머리의 기록 작성 규칙 링크를 없애고 그 링크만 가리키던 기록 폴더 안내 문서 tasks/index.md를 삭제한 노트."
source_type: decision-note
source_id: src-d7837fe2ee4e72b34ef3
sha256: 52d6e73ac517d5b60109160bed4f53adce2cdc89d8158e861336475bc47eadde
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-workflow-tasks-guide-removed.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-workflow-tasks-guide-removed.md"
raw_copy: ".raw/captured/52d6e73ac517d5b60109160bed4f53adce2cdc89d8158e861336475bc47eadde.md"
claim_ids:
  - clm-050b874ca8-c1
  - clm-050b874ca8-c2
key_claims:
  - "2026-09-26 사용자 요청으로 Workflow 대시보드 머리의 기록 작성 규칙 링크가 제거되고 .agents/workflow/tasks/index.md가 삭제됐다."
  - "tasks/index.md 삭제 뒤 Workflow 서버와 화면은 기록 폴더의 모든 .md 파일을 작업 기록으로 다룬다."
---

# 결정 노트 - 2026-09-26-workflow-tasks-guide-removed

- 원본: `.wiki/raw/notes/2026-09-26-workflow-tasks-guide-removed.md`
- 원자료 사본: `.raw/captured/52d6e73ac517d5b60109160bed4f53adce2cdc89d8158e861336475bc47eadde.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `.wiki/raw/notes/2026-09-26-workflow-tasks-guide-removed.md`(frontmatter `ingested: 2026-09-26`, `source: "MANUAL"`).
- 규칙은 작업 절차 한 장에만 둔다는 SSoT 결정([[결정 노트 - 2026-09-26-workflow-ssot-copies]])의 연장이다.

## 사람의 판단 원문

> 사용자 2026-09-26: "기록 작성 규칙 버튼 제거해주세요. 안쓰는 문서라면 문서까지도 제거하구요."

## 관찰과 변경

- `.agents/workflow/tasks/index.md`는 같은 날 이미 "작업 현황 목록은 이 문서에 적지 않는다, 규칙은 작업 절차에 있다"는 안내 두 줄로 줄어 있었다. 쓰는 곳은 대시보드의 기록 작성 규칙 링크, `.agents/workflow/index.md` 목차의 작업 현황 줄, Wiki `wiki-workflow`의 한 문장뿐이었다. `--tasks` 명령과 상태 줄 규칙은 작업 절차 문서에도 있어 정보 손실이 없다고 판단했다.
- 대시보드 머리는 제목과 새 작업 버튼만 남았다. 목차 줄을 지우고, 서버의 기록 경로 검사·작업 목록 생성과 화면 기록 목록에서 `index.md`를 제외하던 조건도 지웠다. 이제 기록 폴더의 모든 `.md`가 작업 기록이다.
- 문서는 `git rm -f`로 지웠다(사용자가 직접 요청한 삭제라 자동 모드에서도 허용). 과거 작업 기록과 원자료의 `tasks/index.md` 언급은 이력이라 그대로 둔다.

## 검증 범위

- 자동 테스트 TestWikiViewer(대시보드 머리가 제목·새 작업 버튼뿐), TestWorkflowTestFeedback(안내 문서 없는 기록 폴더), 링크 검사로 확인했다. 사람의 화면 확인은 이 노트에 없다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-26 사용자 요청으로 Workflow 대시보드 머리의 기록 작성 규칙 링크가 제거되고 .agents/workflow/tasks/index.md가 삭제됐다. ^c1
- tasks/index.md 삭제 뒤 Workflow 서버와 화면은 기록 폴더의 모든 .md 파일을 작업 기록으로 다룬다. ^c2
