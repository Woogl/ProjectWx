# WxSave — 월드 영속화 및 세이브/로드

> Level Streaming Persistence를 중심으로 맵 액터, Instanced Actors, Mass 엔티티와 플레이어 재개 상태를 하나의 SaveGame에 저장한다.

## 책임

**담당**

- `UWxSaveGameSubsystem`: 활성 슬롯 수명, 새 포맷 판정, 비동기 디스크 I/O, 저장 맵 ServerTravel.
- `UWxSaveWorldSubsystem`: 월드 라이프사이클에서 LSP·IAM·Mass 스냅샷을 플러시하고, 저장 재개용 `APlayerStartPIE`를 월드 초기화 단계에 제공.
- `UWxPersistableActorReferenceManager`: 이전 세션 레벨 경로·액터 이름을 현재 런타임 액터로 변환하고 레벨별 지연 복원을 완료.
- `UWxSaveLibrary`와 `FWxStateTreeTask_SaveGame`: 기존 UI와 체크포인트 에셋이 사용하는 BP/StateTree 진입점.
- `UWxPersistableEntityConfigTrait`과 `AWxPersistedMassSpawner`: 일반 Mass 엔티티의 저장 opt-in과 중복 스폰 방지.

**경계**

- 저장할 LSP 속성은 `Config/DefaultEngine.ini`의 `LevelStreamingPersistenceSettings.Properties` 허용 목록이 결정한다.
- `IWxSavable`은 식별자나 바이트 직렬화 계약이 아니라, LSP 직전의 상태 투영과 속성 복원 후처리 계약이다.
- 맵 배치 액터의 파괴는 모두 보존한다. 런타임 액터 재생성은 LSP 설정의 명시적 허용 목록에 든 적·픽업만 대상으로 하며, 투사체 등 일시 액터는 제외한다.
- Mass는 `UWxPersistableEntityConfigTrait`이 붙은 EntityConfig만 대상으로 하며, `UWxSaveSettings.MassFragmentsToSerialize`에 등록된 fragment만 저장한다.

## 저장 데이터

`UWxSaveGame::SavedStatePerMap`은 안정화된 맵 패키지 이름을 키로 다음 데이터를 보관한다.

| 데이터 | 생성·복원 주체 |
| --- | --- |
| `StreamingLevelData` | `ULevelStreamingPersistenceManager` |
| `SavedStatePerStreamingLevel` | 스트리밍 레벨별 `AInstancedActorsManager` 델타 |
| `MassEntitySnapshots` | EntityConfig별 허용 fragment 원시 스냅샷 |

플레이어 맵·폰 트랜스폼·컨트롤 회전은 `TravelData`에 저장된다. GAS base 어트리뷰트와 `Effect.Savable` GE는 `AWxWorldSettings::PlayerPersistenceState`가 LSP 데이터로 운반한다. LSP가 직접 복원하는 AttributeSet도 저장된 base 값을 ASC API로 다시 적용한 뒤 GE modifier를 복원한다.

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
| `FWxPersistableActorReference` | 세션 안전 레벨 액터·플레이어 참조 |
| `UWxPersistableReferencedActorComponent` | 런타임 액터의 이전 세션 식별자를 현재 인스턴스에 등록 |
| `UWxPersistableActorReferenceManager` | 이전 세션 액터 레지스트리와 레벨별 완료 콜백 |
| `UWxSaveLibrary` | 기존 BP 저장 API |
| `FWxStateTreeTask_SaveGame` | 체크포인트 비동기 저장 태스크 |

## 확장 규약

- 맵 액터/컴포넌트 속성: LSP 설정의 `Properties`에 전체 property path를 추가한다. 저장 전 투영이나 복원 후 런타임 동기화가 필요하면 대상 오브젝트 또는 호스트 액터가 `IWxSavable`을 구현한다.
- GameplayEffect: 세션을 넘어야 하는 GE에 Asset Tag `Effect.Savable`을 명시한다. 클래스·인스티게이터·남은 시간·레벨·스택·SetByCaller가 복원된다.
- 런타임 액터: LSP `RuntimeRespawnedActorClasses`에 명시적으로 등록한다. 세션을 넘는 다른 저장 데이터가 해당 액터를 참조하면 `UWxPersistableReferencedActorComponent`도 추가한다.
- 스포너 적: `AWxSpawner::bIsKilled`만 영속 상태로 두고, 살아서 복원된 `AWxEnemyCharacter`는 이전 세션 스포너 참조와 기존 attachment 관계로 수명 관리에 다시 합류한다. Pawn `Owner`는 Controller 빙의가 사용하므로 영속 수명 추적에 쓰지 않는다.
- 픽업: `AWxItemPickup::PersistedState`가 동적 아이템 정의·수량·물리 속도를 운반한다. 획득되어 파괴된 픽업은 다음 저장에 런타임 액터 레코드가 남지 않는다.
- 플레이어: 폰은 LSP 런타임 재스폰 대상에 넣지 않는다. `AWxWorldSettings`가 플레이어 상태를 운반하고, 스폰 위치는 `UWxSaveWorldSubsystem`이 만든 `APlayerStartPIE`를 엔진 `ChoosePlayerStart`가 선택한다.
- 이동 가능한 맵 배치 Pawn 루트: `RelativeLocation`·`RelativeRotation`만 저장되며, Mass가 소유한 액터는 제외된다.
- Instanced Actors: `IA.DeferSpawnEntities=1`이 필수다. IAM 등록 뒤 deferred entity spawn 전에 델타를 복원한다.
- Mass: EntityConfig에 `UWxPersistableEntityConfigTrait`을 추가하고, 원래 스포너 대신 `AWxPersistedMassSpawner`를 사용한다.
- 저장 완료 대기: `SaveToFile` 접수 뒤 `IsSaveInProgress()`와 일회성 `OnSaveCompleted`를 사용한다.

## 포맷 전환

현재 저장 포맷은 버전 `5`이다. 이전 버전 슬롯은 변환하지 않고 새 슬롯으로 초기화한다.

## 읽는 순서

1. `Public/WxSaveGame.h`
2. `Public/WxSaveGameSubsystem.h`
3. `Public/WxSaveWorldSubsystem.h`
4. `Public/WxMassPersistence.h`
5. `Config/DefaultEngine.ini`의 LSP/WxSave 설정

---

*갱신일 2026-08-31*
