---
title: "WX Wiki 운영과 재생성"
category: reference
sources:
  - "raw/notes/2026-09-22-current-workflow.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, workflow]
aliases: []
confidence: medium
volatility: warm
summary: "프로젝트 로컬 Wiki는 최신 근거를 수집한 뒤 편찬하며, 날짜·정적 확인·인간 검증을 구분한다."
---

# WX Wiki 운영과 재생성

프로젝트 로컬 Wiki는 최신 근거를 수집한 뒤 편찬하며, 날짜·정적 확인·인간 검증을 구분한다.

## 정본과 탐색

저장소의 `.wiki/_index.md`에서 시작한다. `raw/`는 수집 당시 근거, `wiki/`는 편찬 문서, `_index.md`는 재생성 가능한 색인이다. config.md와 schema.md는 범위·근거 해석 가이드다. 개인 Hub나 사용자별 캐시 절대경로를 팀 공유 문서에 저장하지 않는다.

기존 코드·설정·기획은 원래 위치에서 관리한다. `Docs/Programmer/`는 읽기·인용만 한다. Workflow의 사람 판단·확정본은 tasks에 남기고 Wiki에서는 필요한 결정·이유·제약만 연결한다.

## 전체 재생성 절차

1. 현재 코드·설정·기획을 읽고 기존 주장·결정·미결정을 대조한다. HEAD와 미커밋 작업 트리를 구분한다.
2. 선택한 근거의 버전·해시·확인 범위를 새 raw 자료로 수집한다. 코드 전체를 Wiki에 복제하지 않는다.
3. 전체 편찬으로 각 주제·개념 문서를 다시 작성하고 출처와 양방향 관련 링크를 연결한다.
4. 대체된 자료 정리는 사용자 합의 범위에서 수행한다. 당시 판단의 유일한 근거와 현재 미결정을 이름이 오래되었다는 이유만으로 버리지 않는다.
5. 순정 Lint로 구조·출처·색인을 검사하고 저장소 링크 검사와 뷰어 내보내기를 수행한다.

LLM Wiki의 `compile --full`은 raw 전체를 다시 읽는 작업이다. 최신 원자료 수집 없이 실행하면 오래된 자료를 다시 편찬할 뿐이다. Codex에서는 설치된 `wiki` 스킬에 자연어 또는 `$wiki`로 요청한다. Claude Code의 `/wiki:*` 명령 표기는 Codex 전용 슬래시 명령 등록을 뜻하지 않는다.

## 검증 상태를 읽는 법

`created`·`updated`는 문서 날짜다. `verified`는 인간 확인 근거가 있을 때만 기록한다. 이번 재생성은 파일별 정적 조사를 기록했고 새로운 인간 확인일을 부여하지 않았다. 순정 편찬 안내의 기본 날짜 부여 대신 프로젝트 schema의 근거 해석을 따른다.

현재 raw 조사 기록은 모델의 정적 관찰과 저장소 원문 발췌이므로 독립적인 실행 시험 증거가 아니다. 해시는 읽은 버전의 식별값이며 그 파일 전체의 품질 인증이 아니다. `confidence`와 최신성 점수도 빌드·에셋·멀티플레이 성공을 보증하지 않는다.

## 변경 후 점검

- 순정 플러그인의 `llm-wiki lint --local`로 구조·출처·색인을 확인한다.
- `pwsh -NoProfile -File .agents/scripts/CheckWikiLinks.ps1`로 저장소 링크를 확인한다.
- `pwsh -NoProfile -File .agents/scripts/Export-Wiki.ps1`로 뷰어를 갱신한다.

로그는 `.wiki/log.md`에 누적하고 재사용할 지식은 기존 주제에 통합한다. 문서 작업만으로 게임 코드 변경·커밋·푸시 권한이 생기지 않는다.

## 관련 문서

- [[editor-tools|편집기 도구 — WxEditor·WxToolset·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·BoxComponentVisualizer](../references/editor-tools.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[wiki-workflow|Wiki·Workflow 도구 구조]] ([Wiki·Workflow 도구 구조](../references/wiki-workflow.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-workflow.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. 인간 검증일 `verified`는 새로 부여하지 않았다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
