---
type: source
title: "결정 노트 - 2026-09-25-save-checkpoint-rename"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "체크포인트"
  - "StateTree"
summary: "사용자 후속 요청으로 RecordCheckpoint API와 StateTree 태스크를 SaveCheckpoint로 이름을 바꾸고 CoreRedirects로 기존 구조체 경로를 이어 준 기록"
source_type: decision-note
source_id: src-78202ac612e06551e8ad
sha256: d551b3b1a714f6c12c9210250af529eaa3b3da3271e8fa481b3b379a8996f715
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-save-checkpoint-rename.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-save-checkpoint-rename.md"
raw_copy: ".raw/captured/d551b3b1a714f6c12c9210250af529eaa3b3da3271e8fa481b3b379a8996f715.md"
claim_ids:
  - clm-f6258770d4-c1
  - clm-f6258770d4-c2
  - clm-f6258770d4-c3
key_claims:
  - "2026-09-25 사용자 후속 요청으로 체크포인트 저장 API 이름이 RecordCheckpoint에서 SaveCheckpoint로 바뀌었다."
  - "StateTree 저장 태스크 구조체는 FWxStateTreeTask_SaveCheckpoint와 FWxStateTreeTask_SaveCheckpointInstanceData로 바뀌었고 표시명은 체크포인트 저장이다."
  - "이전 두 구조체 경로는 DefaultEngine.ini CoreRedirects로 새 경로에 매핑되었고 에셋 재저장은 하지 않았다."
---

# 결정 노트 - 2026-09-25-save-checkpoint-rename

- 원본: `.wiki/raw/notes/2026-09-25-save-checkpoint-rename.md`
- 원자료 사본: `.raw/captured/d551b3b1a714f6c12c9210250af529eaa3b3da3271e8fa481b3b379a8996f715.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-save-checkpoint-rename.md`(제목 "체크포인트 저장 명칭 통일", source `MANUAL`, ingested 2026-09-25)를 요약한다. 명칭 변경과 코드 관찰의 기록이며 빌드·실행 검증 결과는 노트에 없다(정적 조사). 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사용자 요청

노트 서술: 2026-09-25 사용자 후속 요청으로 `RecordCheckpoint`를 `SaveCheckpoint`로 변경했다(원문 인용 표기 없음). 저장 동작은 유지한다.

## 구현 관찰

- `UWxCheckpointSaveGame::SaveCheckpoint`가 저장 API다(`Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSaveGame.h`).
- `FWxStateTreeTask_SaveCheckpoint`와 `FWxStateTreeTask_SaveCheckpointInstanceData`로 구조체·생성자·헤더·cpp·generated include·호출부를 함께 바꿨다. 표시명은 `체크포인트 저장`이다(`Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SaveCheckpoint.h`).
- `Config/DefaultEngine.ini`의 CoreRedirects에 이전 두 구조체 경로를 새 경로로 매핑했다. 에셋 재저장은 수행하지 않았다.

## 미결정·충돌

- 이전 원자료 [[결정 노트 - 2026-09-25-checkpoint-savegame]]에 적힌 RecordCheckpoint 파일 경로는 변경 전 조사 시점의 경로다.
- 에셋을 재저장하지 않았으므로 기존 에셋은 CoreRedirects에 기대고 있다.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[결정 노트 - 2026-09-25-checkpoint-single-slot]]

## 핵심 주장

- 2026-09-25 사용자 후속 요청으로 체크포인트 저장 API 이름이 RecordCheckpoint에서 SaveCheckpoint로 바뀌었다. ^c1
- StateTree 저장 태스크 구조체는 FWxStateTreeTask_SaveCheckpoint와 FWxStateTreeTask_SaveCheckpointInstanceData로 바뀌었고 표시명은 체크포인트 저장이다. ^c2
- 이전 두 구조체 경로는 DefaultEngine.ini CoreRedirects로 새 경로에 매핑되었고 에셋 재저장은 하지 않았다. ^c3
