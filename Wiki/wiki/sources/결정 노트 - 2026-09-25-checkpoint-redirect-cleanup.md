---
type: source
title: "결정 노트 - 2026-09-25-checkpoint-redirect-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "체크포인트"
  - "리다이렉트"
summary: "ST_CheckPoint를 SaveCheckpoint 구조체로 리세이브한 뒤 CoreRedirects 두 항목을 지우고 새 프로세스에서 재로드·컴파일·저장을 확인한 기록"
source_type: decision-note
source_id: src-ce810cd749d77af9dee5
sha256: b0d65d7de88adb6f16cb61a12ae0dbec34231b0a6fc45a7f6ed4df7f162fea2d
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-checkpoint-redirect-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-checkpoint-redirect-cleanup.md"
raw_copy: ".raw/captured/b0d65d7de88adb6f16cb61a12ae0dbec34231b0a6fc45a7f6ed4df7f162fea2d.md"
claim_ids:
  - clm-3962508bf0-c1
  - clm-3962508bf0-c2
  - clm-3962508bf0-c3
key_claims:
  - "2026-09-25 ST_CheckPoint는 SaveCheckpoint 구조체 이름으로 리세이브됐고 DefaultEngine.ini의 체크포인트 StructRedirects 두 항목이 제거됐다."
  - "리다이렉트 제거 후 새 에디터 프로세스에서 ST_CheckPoint의 로드·StateTree 컴파일·저장이 종료 코드 0으로 성공했다."
  - "체크포인트 리다이렉트 제거 작업은 인게임 플레이로 검증되지 않았다."
---

# 결정 노트 - 2026-09-25-checkpoint-redirect-cleanup

- 원본: `.wiki/raw/notes/2026-09-25-checkpoint-redirect-cleanup.md`
- 원자료 사본: `.raw/captured/b0d65d7de88adb6f16cb61a12ae0dbec34231b0a6fc45a7f6ed4df7f162fea2d.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 제목 "체크포인트 구조체 리다이렉트 제거", 출처 `MANUAL`, 수집일 2026-09-25.
- 사용자 요청에 따른 작업이다(요청 원문은 노트에 없음). 노트 날짜 기준이라 현재 설정과 다를 수 있다.
- 앞선 이름 변경 기록 <!--wl-->결정 노트 - 2026-09-25-save-checkpoint-rename의 리다이렉트 추가 설명은 마이그레이션 당시 기록이며, 이 노트 시점 설정에는 리다이렉트가 없다.

## 작업 내용

- 기존 `RecordCheckpoint`/`RecordCheckpointInstanceData` 리다이렉트를 적용한 상태에서 `ST_CheckPoint`를 ResavePackages로 저장했다. `Content`/`Plugins`의 uasset·umap 문자열 검색에서 이전 구조체를 참조한 패키지는 이 하나였다.
- `Config/DefaultEngine.ini`의 두 StructRedirects와 빈 `[CoreRedirects]` 섹션을 제거했다.

## 검증 범위

- 새 `UnrealEditor-Cmd` 프로세스로 같은 패키지를 다시 로드·컴파일·저장했다. 두 실행 모두 종료 코드 0, StateTree 컴파일과 저장 성공.
- 이전 이름은 에셋 검색에서 발견되지 않고 새 `SaveCheckpoint` 이름이 저장돼 있다. 공통 경고 1건은 MCP 플러그인 라이선스 안내이며 로드·컴파일 오류는 없다.
- 실행 로그: `Saved/Logs/CheckpointResave.log`, `Saved/Logs/CheckpointWithoutRedirects.log`.
- 인게임 플레이는 검증하지 않았다.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[모듈 구조와 코드 정리]]

## 핵심 주장

- 2026-09-25 ST_CheckPoint는 SaveCheckpoint 구조체 이름으로 리세이브됐고 DefaultEngine.ini의 체크포인트 StructRedirects 두 항목이 제거됐다. ^c1
- 리다이렉트 제거 후 새 에디터 프로세스에서 ST_CheckPoint의 로드·StateTree 컴파일·저장이 종료 코드 0으로 성공했다. ^c2
- 체크포인트 리다이렉트 제거 작업은 인게임 플레이로 검증되지 않았다. ^c3
