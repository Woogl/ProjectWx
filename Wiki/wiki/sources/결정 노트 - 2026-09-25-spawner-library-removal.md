---
type: source
title: "결정 노트 - 2026-09-25-spawner-library-removal"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "스포너"
  - "리스폰"
summary: "사용자 요청으로 UWxSpawnerLibrary를 제거하고 AWxSpawner::RespawnAll C++ 전용 함수로 플레이어 부활·StateTree 일괄 재생성을 연결한 기록"
source_type: decision-note
source_id: src-7e48d3401b0c2337cc4c
sha256: 1d02ce6dd9dd0745ab450091cc06a519b1cf808d55d894a8abf22a4403113f53
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-spawner-library-removal.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-spawner-library-removal.md"
raw_copy: ".raw/captured/1d02ce6dd9dd0745ab450091cc06a519b1cf808d55d894a8abf22a4403113f53.md"
claim_ids:
  - clm-cfeba1667d-c1
  - clm-cfeba1667d-c2
  - clm-cfeba1667d-c3
key_claims:
  - "2026-09-25 사용자 요청으로 UWxSpawnerLibrary가 제거되고 일괄 재생성은 UFUNCTION 없는 C++ 전용 AWxSpawner::RespawnAll(const UWorld*)로 옮겨졌다."
  - "AWxSpawner::RespawnAll은 로드된 스포너 중 Manual을 제외하고 수집하며 클라이언트 월드는 건너뛴다."
  - "에셋 안의 WxSpawnerLibrary·TryRespawnAll 참조 부재는 바이너리 문자열 검색으로만 확인했고 실제 에셋 로드 검증은 아니다."
---

# 결정 노트 - 2026-09-25-spawner-library-removal

- 원본: `.wiki/raw/notes/2026-09-25-spawner-library-removal.md`
- 원자료 사본: `.raw/captured/1d02ce6dd9dd0745ab450091cc06a519b1cf808d55d894a8abf22a4403113f53.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-spawner-library-removal.md`(제목 "SpawnerLibrary 제거와 C++ 일괄 재생성", source `MANUAL`, ingested 2026-09-25)를 요약한다. 코드 변경과 정적 조사(빌드·실행 검증 아님) 기록이며, 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사용자 요청

노트 서술: 2026-09-25 사용자 요청 — UWxSpawnerLibrary를 제거하고 가능하면 BP 작업이 없도록 처리(원문 인용 표기 없음).

## 조사 관찰

- 기존 C++ 호출부는 `WxRespawnLibrary`와 `StateTreeTask_RespawnSpawners` 두 곳이었다.
- Content/Plugins의 uasset·umap 바이너리 문자열 검색에서 `WxSpawnerLibrary`·`TryRespawnAll`은 발견되지 않았다. 실제 에셋 로드 검증은 아니다.

## 구현 관찰

- 일괄 재생성은 `AWxSpawner::RespawnAll(const UWorld*)`로 옮겼다. UFUNCTION이 없는 C++ 전용 API다.
- 두 호출부를 직접 연결했고, 플레이어 부활 성공 후 스트리밍 완료 시점과 StateTree 라이브 진입 시점은 유지한다.
- 현재 로드된 스포너 중 Manual을 제외하고 수집해 Respawn한다. 클라이언트 월드는 건너뛰고 개별 Respawn의 권한·영구 처치 제한도 유지한다.
- 재생성 콜백에서 다른 스포너가 사라질 수 있어 목록은 약참조로 보관하고 호출 직전에 유효성을 확인한다.
- BP에서 별도의 스포너 재생성 노드를 호출할 필요가 없다.
- 근거 파일: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RespawnSpawners.cpp`.

## 범위 밖

플레이어 부활 요청 진입점 자체의 UI 연결은 이번 범위가 아니다.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[적 AI와 몬스터]]
- [[모듈 구조와 코드 정리]]
- [[기획서 - Respawn_System]]

## 핵심 주장

- 2026-09-25 사용자 요청으로 UWxSpawnerLibrary가 제거되고 일괄 재생성은 UFUNCTION 없는 C++ 전용 AWxSpawner::RespawnAll(const UWorld*)로 옮겨졌다. ^c1
- AWxSpawner::RespawnAll은 로드된 스포너 중 Manual을 제외하고 수집하며 클라이언트 월드는 건너뛴다. ^c2
- 에셋 안의 WxSpawnerLibrary·TryRespawnAll 참조 부재는 바이너리 문자열 검색으로만 확인했고 실제 에셋 로드 검증은 아니다. ^c3
