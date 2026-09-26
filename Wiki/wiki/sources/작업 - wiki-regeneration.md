---
type: source
title: "작업 - wiki-regeneration"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "Wiki"
  - "문서"
summary: "옛 .wiki LLM Wiki를 현재 코드·설정·기획 기준으로 전면 재생성하고 대체된 legacy 문서를 정리한 2026-09-22 완료 작업 기록"
source_type: task-record
source_id: src-04b23a4a80e0c6d02fec
sha256: 1db48fbd85c15b2d492e2b08e34b0a80db202c09fa30483b7544184dd0e8eed4
authority: primary
independence_key: ".agents/workflow/tasks/wiki-regeneration.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/wiki-regeneration.md"
raw_copy: ".raw/captured/1db48fbd85c15b2d492e2b08e34b0a80db202c09fa30483b7544184dd0e8eed4.md"
claim_ids:
  - clm-3fad528269-c1
  - clm-3fad528269-c2
  - clm-3fad528269-c3
key_claims:
  - "wiki-regeneration 작업은 2026-09-22 옛 .wiki를 재생성해 기사 17개·원자료 16개로 정리하고 legacy 19개를 삭제했다."
  - "wiki-regeneration 작업의 검증은 lint·링크 검사·뷰어 테스트·렌더링에 한정되며 빌드·게임 실행·바이너리 에셋 내부는 검증하지 않았다."
  - "피니시 대상 우선순위(Q-001), 피니시 위치 조정 주체(Q-002), 현광 자원 예외 공통화(Q-003)는 wiki-regeneration 시점에 미확정으로 보존되었다."
---

# 작업 - wiki-regeneration

- 원본: `.agents/workflow/tasks/wiki-regeneration.md`
- 원자료 사본: `.raw/captured/1db48fbd85c15b2d492e2b08e34b0a80db202c09fa30483b7544184dd0e8eed4.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

2026-09-22 요청된 옛 `.wiki/`(LLM Wiki) 전체 재생성 작업 기록이다. 상태는 완료다. 게임 코드·에셋 변경과 커밋·푸시는 범위 밖이었다.

## 범위와 기준 (확정 결정)

- 기록된 요청 내용: 현재 코드·설정·기획 기준 전체 Wiki 재생성, 대체된 legacy 정리.
- 기준은 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`와 현재 작업 트리다. 선행 Wiki/Workflow 이관 변경이 있어 HEAD만을 현재 내용으로 보지 않는다.
- 문서 작성일과 인간 검증일을 구분하고, C++ 정적 확인은 빌드·실행·바이너리 에셋 검증을 대신하지 않는다.
- 사용자 추가 지시에 따라 두쫀쿠는 수집·편찬 대상에서 제외했다.

## 진행 결과

- 순정 ingest/compile/index/lint 규약과 프로젝트 config/schema, 기존 16개 기사 구성을 확인했다.
- 모듈 선언·의존성, 공개 계약·핵심 실행 경로, 기획 불일치와 기존 미결정을 대조해 파일 해시·원문 발췌를 가진 새 원자료 15개를 수집했다.
- 기존 16개 기사를 재작성하고 편집기 도구 기사 1개를 추가했다. 참조가 교체된 legacy 19개를 삭제해 최종 원자료 16개·기사 17개가 됐다. 순정 렌더러로 색인 7개를 재생성했다.
- 뷰어 표시 오류 최소 수정: 공용 `readRoute`가 Workflow 전용 `workflowGuideDocument`를 호출하던 경로를 `isWorkflow` 분기로 제한했다.
- 두쫀쿠 원본 파일이 이미 제거된 상태여서 작업 목차에 남은 깨진 인계 링크만 정리했다.

## 검증 범위

- 문서 검사(게임 동작 검증 아님): 순정 LLM Wiki lint 모든 등급 0건, `CheckWikiLinks.ps1` 37개 문서 링크 오류 0, `TestWikiViewer.cjs`·`TestWikiSpaces.cjs`·`TestWikiDiagrams.cjs` 통과, Edge 실제 렌더링에서 17개 기사와 Mermaid 다이어그램 6개 표시 통과(pageerror 없음).
- 빌드·게임 실행·멀티플레이·바이너리 에셋 내부는 미검증이며 기사마다 범위를 표시했다.

## 미결정·충돌

- Q-001 피니시 대상 우선순위, Q-002 피니시 위치 조정 주체, Q-003 현광 자원 예외의 공통화는 근거 확인 없이 확정하지 않는다.

## 관련 주제

- [[Wiki 운영]]
- [[그로기·경직·피니시]]
- [[캐릭터 스탯과 전투 자원]]

## 핵심 주장

- wiki-regeneration 작업은 2026-09-22 옛 .wiki를 재생성해 기사 17개·원자료 16개로 정리하고 legacy 19개를 삭제했다. ^c1
- wiki-regeneration 작업의 검증은 lint·링크 검사·뷰어 테스트·렌더링에 한정되며 빌드·게임 실행·바이너리 에셋 내부는 검증하지 않았다. ^c2
- 피니시 대상 우선순위(Q-001), 피니시 위치 조정 주체(Q-002), 현광 자원 예외 공통화(Q-003)는 wiki-regeneration 시점에 미확정으로 보존되었다. ^c3
