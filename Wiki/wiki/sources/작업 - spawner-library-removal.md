---
type: source
title: "작업 - spawner-library-removal"
created: 2026-09-27
updated: 2026-09-27
status: developing
tags:
  - "source"
  - "작업-기록"
  - "체크포인트"
  - "적"
summary: "UWxSpawnerLibrary를 없애고 AWxSpawner::RespawnAll로 일괄 재생성을 옮긴 작업 기록으로, 2026-09-27 헤드리스 에셋 로드와 부활 재생성·영구 처치 확인 뒤 체크리스트 4/4 통과로 완료됐다."
source_type: task-record
source_id: src-2dd920527f56cb3f8771
sha256: 1b1a8ca3d6f0d5dc12a937dd3cd18b0856aa1815fa6ddc7317d5559c3e51e908
authority: primary
independence_key: ".agents/workflow/tasks/spawner-library-removal.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/spawner-library-removal.md"
raw_copy: ".raw/captured/1b1a8ca3d6f0d5dc12a937dd3cd18b0856aa1815fa6ddc7317d5559c3e51e908.md"
claim_ids:
  - clm-d50f102421-c1
  - clm-d50f102421-c2
  - clm-d50f102421-c3
key_claims:
  - "2026-09-27 헤드리스 에디터 임시 자동화 테스트에서 ST_CheckPoint·BP_CheckPoint·퀘스트 StateTree와 스포너가 배치된 외부 액터 패키지 23개가 오류 없이 로드됐고, 경고 8건은 스포너와 무관한 없어진 GameplayTag(Quest.Fail·Event.Device.Triggered) 참조였다."
  - "2026-09-27 헤드리스 게임 테스트에서 처치한 일반 스포너의 적은 사망·부활(RequestRespawn의 RespawnAll) 뒤 새로 스폰되고 시체가 정리됐으며, bNeverRevive 스포너의 적은 부활 뒤에도 다시 나타나지 않았다."
  - "AWxSpawner::RespawnAll과 부활·StateTree 호출부의 코드 리뷰는 woogle이 2026-09-27 통과시켰다."
---

# 작업 - spawner-library-removal

- 원본: `.agents/workflow/tasks/spawner-library-removal.md`
- 원자료 사본: `.raw/captured/1b1a8ca3d6f0d5dc12a937dd3cd18b0856aa1815fa6ddc7317d5559c3e51e908.md` (수집 2026-09-27, 재확인 기한 2027-03-25)

## 개요

`UWxSpawnerLibrary`를 없애고 일괄 재생성을 C++ 전용 `AWxSpawner::RespawnAll`로 옮긴 작업 기록이다. 2026-09-25 구현 내용은 옛 결정 노트에도 있고, 이 기록은 2026-09-27 헤드리스 확인까지 담는다. 상태는 완료(체크리스트 4/4 통과)다.

## 요청과 결정

- 요청: UWxSpawnerLibrary 제거, BP 작업 최소화.
- 구현: 플레이어 부활·StateTree의 C++ 호출부를 `AWxSpawner::RespawnAll`로 전환하고 라이브러리 헤더/cpp를 지웠다. 로드된 스포너만 재생성하고 Manual은 제외하며, 서버 권한과 영구 처치 제한을 유지한다. 재생성 중 파괴된 스포너를 건너뛰도록 약참조 목록을 쓴다.

## 검증 범위

- 2026-09-25: Source/Plugins/Config와 에셋 바이너리 문자열 검색에서 옛 라이브러리 참조 없음, WxEditor Win64 Development 빌드 성공.
- AI(2026-09-27 임시 자동화 테스트, 확인 뒤 삭제):
  - 에셋 로드: ST_CheckPoint·BP_CheckPoint·ST_QuestStep_KillEnemies·ST_Quest_Main1·ST_Quest_Main2와 스포너가 배치된 외부 액터 패키지 23개(LV_DevCombat·LV_OpenWorld·SiegeCannonEmplacement01)를 새 헤드리스 에디터 프로세스로 로드해 오류 0. 경고 8건은 없어진 GameplayTag(`Quest.Fail`·`Event.Device.Triggered`) 참조. 같은 날 실제 BP_CheckPoint 상호작용에서 「스포너 리스폰」 태스크도 오류 없이 실행됐다.
  - 부활 시 적 재생성: 헤드리스 게임(LV_DevCombat -game -nullrhi)에서 BP_Template(Enemy) 일반 스포너의 적을 치트로 처치 → 플레이어 사망 → 사망 화면에서 부활하자 새 적이 스폰되고 시체는 정리됐다. 로직 단위 테스트로 처치 통지·재생성·Manual 제외도 확인했다.
  - 영구 처치 제한: bNeverRevive 스포너의 적은 부활 뒤에도 나타나지 않고 처치 상태가 유지됐다. 살아 있는 영구 대상은 RespawnAll에 새 인스턴스로 되돌려지는 것을 로직 단위 테스트로 확인했다.
- 사람: 코드 리뷰 woogle 2026-09-27 통과.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[결정 노트 - 2026-09-25-spawner-library-removal]]
- [[작업 - checkpoint-savegame]]

## 핵심 주장

- 2026-09-27 헤드리스 에디터 임시 자동화 테스트에서 ST_CheckPoint·BP_CheckPoint·퀘스트 StateTree와 스포너가 배치된 외부 액터 패키지 23개가 오류 없이 로드됐고, 경고 8건은 스포너와 무관한 없어진 GameplayTag(Quest.Fail·Event.Device.Triggered) 참조였다. ^c1
- 2026-09-27 헤드리스 게임 테스트에서 처치한 일반 스포너의 적은 사망·부활(RequestRespawn의 RespawnAll) 뒤 새로 스폰되고 시체가 정리됐으며, bNeverRevive 스포너의 적은 부활 뒤에도 다시 나타나지 않았다. ^c2
- AWxSpawner::RespawnAll과 부활·StateTree 호출부의 코드 리뷰는 woogle이 2026-09-27 통과시켰다. ^c3
