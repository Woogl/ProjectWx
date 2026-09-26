---
type: source
title: "결정 노트 - 2026-09-25-checkpoint-savegame"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "체크포인트"
  - "SaveGame"
summary: "UWxCheckpointSubsystem을 없애고 체크포인트 기록·부활 조회·새 게임 초기화를 USaveGame 디스크 슬롯으로 옮긴 작업의 정적 확인 기록"
source_type: decision-note
source_id: src-4874675dea229793c590
sha256: a5d37955a5a0b5eebbceeb2c8e5a75e18ed84780c545b27776a526f034bf3c8e
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-checkpoint-savegame.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-checkpoint-savegame.md"
raw_copy: ".raw/captured/a5d37955a5a0b5eebbceeb2c8e5a75e18ed84780c545b27776a526f034bf3c8e.md"
claim_ids:
  - clm-17113bdd5b-c1
  - clm-17113bdd5b-c2
  - clm-17113bdd5b-c3
  - clm-17113bdd5b-c4
key_claims:
  - "2026-09-25 작업 트리에서 UWxCheckpointSubsystem은 제거되고 체크포인트 기록·부활 조회·새 게임 초기화가 USaveGame 슬롯(WxCheckpoint, PIE는 WxCheckpoint_PIE)으로 처리된다."
  - "부활 시 저장 없음·로드 실패·다른 맵·유효하지 않은 Transform이면 PlayerStart 경로로 돌아간다."
  - "체크포인트 SaveGame은 체크포인트 한 개만 저장하고 캐릭터·인벤토리·장치 상태는 저장하지 않는다."
  - "체크포인트 SaveGame 전환 노트는 정적 확인이며 인게임 실행 검증이 아니다."
---

# 결정 노트 - 2026-09-25-checkpoint-savegame

- 원본: `.wiki/raw/notes/2026-09-25-checkpoint-savegame.md`
- 원자료 사본: `.raw/captured/a5d37955a5a0b5eebbceeb2c8e5a75e18ed84780c545b27776a526f034bf3c8e.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 제목 "체크포인트 SaveGame 전환", 출처 `MANUAL`, 수집일 2026-09-25.
- 2026-09-25 작업 트리의 코드 **정적 조사(빌드·실행 검증 아님)**다. 인게임 실행 검증은 아니며, 노트 날짜 기준이라 현재 코드와 다를 수 있다. 근거 파일마다 SHA-256을 기록했다.

## 요청(사용자)

- 노트가 적은 사용자 요청: `UWxCheckpointSubsystem` 제거, SaveGame으로 처리.

## 구현 관찰(정적)

- `UWxCheckpointSaveGame`(`Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSaveGame.h`)은 `USaveGame`을 상속하고 SaveGame 프로퍼티로 레벨 패키지와 부활 Transform을 저장한다. Standalone에서만 동작하며 PIE 접두사를 제거한 맵이 일치할 때만 위치·회전을 반환한다.
- 기록은 `SaveGameToSlot`, 조회는 `LoadGameFromSlot`, 새 게임 초기화는 `DoesSaveGameExist`/`DeleteGameInSlot`. 일반 슬롯 `WxCheckpoint`, PIE 슬롯 `WxCheckpoint_PIE`, 사용자 인덱스 0. PIE도 실행 간 저장이 유지된다.
- StateTree 기록 태스크(`WxStateTreeTask_RecordCheckpoint`)는 복원·비플레이어·멀티플레이에서 저장하지 않는다. 저장 실패는 경고와 Failed로 전달한다.
- 부활(`Source/WxGame/Framework/WxRespawnLibrary.cpp`)은 저장 없음·로드 실패·다른 맵·유효하지 않은 Transform이면 PlayerStart 경로로 돌아간다.
- 새 게임(`Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`)은 삭제 실패 시 상태 문구를 표시하고 맵 이동을 시작하지 않는다.

## 범위 밖과 한계

- 저장 내용은 체크포인트 한 개다. 캐릭터·인벤토리·장치 상태나 이어하기 UI는 저장·구현하지 않는다.
- 새 게임 요청 후 맵 이동이 실패하면 초기화된 체크포인트는 복구하지 않는다(기존 초기화 시점과 같음).

## 검증 범위

- 정적 코드 확인뿐이며 빌드·인게임 저장/부활 동작은 이 노트에서 검증하지 않았다.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[상호작용과 장치]]
- [[기획서 - Respawn_System]]

## 핵심 주장

- 2026-09-25 작업 트리에서 UWxCheckpointSubsystem은 제거되고 체크포인트 기록·부활 조회·새 게임 초기화가 USaveGame 슬롯(WxCheckpoint, PIE는 WxCheckpoint_PIE)으로 처리된다. ^c1
- 부활 시 저장 없음·로드 실패·다른 맵·유효하지 않은 Transform이면 PlayerStart 경로로 돌아간다. ^c2
- 체크포인트 SaveGame은 체크포인트 한 개만 저장하고 캐릭터·인벤토리·장치 상태는 저장하지 않는다. ^c3
- 체크포인트 SaveGame 전환 노트는 정적 확인이며 인게임 실행 검증이 아니다. ^c4
