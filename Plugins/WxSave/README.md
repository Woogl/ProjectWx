# WxSave — 월드 영속화 및 세이브/로드

> Level Streaming Persistence를 중심으로 맵 액터, Instanced Actors, Mass 엔티티와 플레이어 재개 상태를 하나의 SaveGame에 저장한다.

## 책임

**담당**

- `UWxSaveGameSubsystem`: 활성 슬롯 수명, 새 포맷 판정, 비동기 디스크 I/O, 저장 맵 ServerTravel.
- `UWxSaveWorldSubsystem`: 월드 라이프사이클에서 LSP·IAM·Mass 스냅샷을 플러시하고 복원.
- `UWxPlayerSpawnComponent`: 저장된 재개 지점과 GAS 어트리뷰트를 새 플레이어 폰에 적용.
- `UWxSaveLibrary`와 `FWxStateTreeTask_SaveGame`: 기존 UI와 체크포인트 에셋이 사용하는 BP/StateTree 진입점.
- `UWxPersistableEntityConfigTrait`과 `AWxPersistedMassSpawner`: 일반 Mass 엔티티의 저장 opt-in과 중복 스폰 방지.

**경계**

- 저장할 LSP 속성은 `Config/DefaultEngine.ini`의 `LevelStreamingPersistenceSettings.Properties` 허용 목록이 결정한다.
- `IWxSavable`은 식별자나 직렬화 계약이 아니라, LSP가 액터 또는 소유 컴포넌트 속성을 복원한 뒤 호출하는 후처리 계약이다.
- 액터 파괴 및 런타임 액터 재생성은 기본 비활성이다. 필요한 클래스만 LSP 설정에 명시적으로 추가한다.
- Mass는 `UWxPersistableEntityConfigTrait`이 붙은 EntityConfig만 대상으로 하며, `UWxSaveSettings.MassFragmentsToSerialize`에 등록된 fragment만 저장한다.

## 저장 데이터

`UWxSaveGame::SavedStatePerMap`은 안정화된 맵 패키지 이름을 키로 다음 데이터를 보관한다.

| 데이터 | 생성·복원 주체 |
| --- | --- |
| `StreamingLevelData` | `ULevelStreamingPersistenceManager` |
| `SavedStatePerStreamingLevel` | 스트리밍 레벨별 `AInstancedActorsManager` 델타 |
| `MassEntitySnapshots` | EntityConfig별 허용 fragment 원시 스냅샷 |

플레이어 맵·폰 트랜스폼·컨트롤 회전은 `TravelData`, GAS base 어트리뷰트는 `PlayerStats`에 저장된다.

## 핵심 타입

| 타입 | 역할 |
| --- | --- |
| `UWxSaveGameSubsystem` | 슬롯·디스크 I/O·트래블 |
| `UWxSaveWorldSubsystem` | LSP/IAM/Mass 플러시·복원 |
| `UWxSaveGame` | 디스크 직렬화 데이터 |
| `UWxSaveSettings` | teardown 자동 플러시, 플레이어 복원, Mass fragment 허용 목록 |
| `UWxMassPersistence` | Mass FrameEnd 경계 스냅샷·복원 |
| `UWxPersistableEntityConfigTrait` | Mass EntityConfig 영속화 opt-in |
| `AWxPersistedMassSpawner` | 복원 세션의 Mass 중복 스폰 방지 |
| `UWxPlayerSpawnComponent` | 플레이어 재개 지점·스탯 적용 |
| `UWxSaveLibrary` | 기존 BP 저장 API |
| `FWxStateTreeTask_SaveGame` | 체크포인트 비동기 저장 태스크 |

## 확장 규약

- 맵 액터/컴포넌트 속성: LSP 설정의 `Properties`에 전체 property path를 추가한다. 복원 후 런타임 동기화가 필요하면 호스트 액터가 `IWxSavable`을 구현한다.
- 이동 가능한 맵 배치 Pawn 루트: `RelativeLocation`·`RelativeRotation`만 저장되며, Mass가 소유한 액터는 제외된다.
- Instanced Actors: `IA.DeferSpawnEntities=1`이 필수다. IAM 등록 뒤 deferred entity spawn 전에 델타를 복원한다.
- Mass: EntityConfig에 `UWxPersistableEntityConfigTrait`을 추가하고, 원래 스포너 대신 `AWxPersistedMassSpawner`를 사용한다.
- 저장 완료 대기: `SaveToFile` 접수 뒤 `IsSaveInProgress()`와 일회성 `OnSaveCompleted`를 사용한다.

## 포맷 전환

현재 저장 포맷은 버전 `2`다. 구 `ActorRecords`/`SaveId` 기반 슬롯은 로드 시 새 저장으로 초기화하며 변환하거나 이중 기록하지 않는다.

## 읽는 순서

1. `Public/WxSaveGame.h`
2. `Public/WxSaveGameSubsystem.h`
3. `Public/WxSaveWorldSubsystem.h`
4. `Public/WxMassPersistence.h`
5. `Config/DefaultEngine.ini`의 LSP/WxSave 설정

---

*갱신일 2026-08-30 · 소스 22파일*
