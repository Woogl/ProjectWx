---
type: source
title: "결정 노트 - 2026-09-25-checkpoint-single-slot"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "체크포인트"
  - "SaveGame"
summary: "사용자 결정으로 PIE와 일반 플레이가 WxCheckpoint 저장 슬롯 하나를 공유하고 슬롯 세분화는 나중으로 미룬 기록"
source_type: decision-note
source_id: src-3c11042aae540cb5d02a
sha256: 94bbb7943f1c6c95abd5b1952dbcbd9b41fabb3454d3f793a8baccf8c7674aa4
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-checkpoint-single-slot.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-checkpoint-single-slot.md"
raw_copy: ".raw/captured/94bbb7943f1c6c95abd5b1952dbcbd9b41fabb3454d3f793a8baccf8c7674aa4.md"
claim_ids:
  - clm-93e414f756-c1
  - clm-93e414f756-c2
  - clm-93e414f756-c3
key_claims:
  - "사용자는 2026-09-25에 체크포인트 저장 슬롯을 하나로 통일하고 세분화는 나중에 하기로 결정했다."
  - "노트 시점의 UWxCheckpointSaveGame::GetSlotName()은 World 인자 없이 항상 WxCheckpoint를 반환한다."
  - "기존 WxCheckpoint_PIE 저장 파일의 자동 이관·삭제는 구현되지 않았다."
---

# 결정 노트 - 2026-09-25-checkpoint-single-slot

- 원본: `.wiki/raw/notes/2026-09-25-checkpoint-single-slot.md`
- 원자료 사본: `.raw/captured/94bbb7943f1c6c95abd5b1952dbcbd9b41fabb3454d3f793a8baccf8c7674aa4.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-checkpoint-single-slot.md`(제목 "체크포인트 단일 슬롯 결정", source `MANUAL`, ingested 2026-09-25)를 요약한다. 사용자의 슬롯 통일 결정과, 그 결정에 따른 저장 코드의 정적 조사(빌드·실행 검증 아님)를 담는다. 노트 날짜 기준 관찰이므로 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-25: "하나로 통일하고 나중에 세분화한다."

## 확정 결정

- PIE와 일반 플레이가 `WxCheckpoint` 슬롯 하나를 공유한다. 슬롯 세분화는 추후 진행한다.
- 이 결정은 과거의 PIE 분리 설명을 대체한다.

## 구현 관찰(정적 조사)

- `UWxCheckpointSaveGame::GetSlotName()`은 World 인자 없이 항상 `WxCheckpoint`를 반환한다.
- 저장·조회·새 게임 초기화가 모두 같은 슬롯과 사용자 인덱스 0을 사용한다.
- 근거 파일: `Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSaveGame.cpp`.

## 미결정·충돌

- 기존 `WxCheckpoint_PIE` 파일의 자동 이관·삭제는 구현하지 않았다.
- 슬롯 세분화의 시점과 방식은 정해지지 않았다.

## 검증 범위

노트는 결정과 코드 관찰만 적고 빌드·플레이 확인 결과를 적지 않는다.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[결정 노트 - 2026-09-25-checkpoint-savegame]]
- [[결정 노트 - 2026-09-25-save-checkpoint-rename]]

## 핵심 주장

- 사용자는 2026-09-25에 체크포인트 저장 슬롯을 하나로 통일하고 세분화는 나중에 하기로 결정했다. ^c1
- 노트 시점의 UWxCheckpointSaveGame::GetSlotName()은 World 인자 없이 항상 WxCheckpoint를 반환한다. ^c2
- 기존 WxCheckpoint_PIE 저장 파일의 자동 이관·삭제는 구현되지 않았다. ^c3
