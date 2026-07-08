# PersistenceUtils — 맵 상태 영속화 유틸리티

> Epic Games(저자 Zhi Kang Shao)가 제공하는 서드파티 유틸리티 플러그인. 맵의 런타임 상태 — 일반 액터(Level Streaming Persistence), Instanced Actor(IAM 델타), Mass 엔티티 스냅샷 — 를 `USaveGame` 파일로 직렬화/복원하고, 맵 이동 시 플레이어 위치까지 이어준다.

## 책임
**담당**
- SaveGame 수명 관리: 슬롯 생성·로드·디스크 저장·리로드, 맵 간 인메모리 유지 (`UPersistenceGameSubsystem`, `USaveFilePersistenceUtils`).
- 세이브 플러시 오케스트레이션: 레벨 스트리밍 데이터·IAM·맵 이동 데이터를 동기 플러시하고, Mass 프래그먼트를 만지는 작업은 `FrameEnd` phase 경계로 지연 (`UPersistenceWorldSubsystem`).
- Mass 엔티티 스냅샷/복원: config 자산 단위로 지속 가능 엔티티를 직렬화·재스폰 (`UMassPersistenceUtils`, `UPersistableEntityConfigTrait`, `APersistedMassSpawner`).
- 세션 간 액터 참조 해석: `FPersistableActorReference`를 (레벨경로, 액터FName)으로 저장·재해석 (`UPersistableActorReferenceManager` 및 References/*).
- 쿡 타임 액터→Instanced Actor 변환 (`UInstancedActorsCellTransformerBase`).
- 저장 대상 오브젝트용 pre/post 훅 인터페이스 (`IPersistedObject`), 설정 (`UPersistenceUtilsSettings`).

**경계 (비담당)**
- 실제 프로퍼티 직렬화·델타 계산은 엔진 플러그인 `LevelStreamingPersistence` / `InstancedActors`에 위임.
- 게임별 저장 데이터 정의·저장 시점 결정은 소비 측 게임 코드 몫 (`FPersistenceUtilsDelegates::OnPreSave` 구독).

## 의존성
- **주요 의존**: 엔진 플러그인 `LevelStreamingPersistence`, `InstancedActors`(`.uplugin` 명시). Mass 스택 `MassEntity`/`MassCore`(Public), `MassActors`/`MassSpawner`/`MassSimulation`(Private). 기타 `Engine`, `DeveloperSettings`, `EngineSettings`, `GameplayDebugger`.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (Epic 예제 플러그인, Wx 모듈 미참조. Build.cs·`.uplugin` 확인)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UPersistenceGameSubsystem` | GameInstance 서브시스템 — SaveGame을 메모리에 보유, 맵 간 유지, 디스크 저장/로드 진입점 | `Source/PersistenceUtils/Public/Framework/PersistenceGameSubsystem.h` |
| `UPersistenceWorldSubsystem` | World 서브시스템 — 세이브 플러시/복원 오케스트레이션(레벨스트리밍·IAM·Mass·맵이동) 심장부 | `Source/PersistenceUtils/Public/Framework/PersistenceWorldSubsystem.h` |
| `USaveFilePersistenceUtils` | Blueprint 진입점 — StartNew/Load/Reload/Save/Travel 함수 라이브러리 | `Source/PersistenceUtils/Public/Framework/SaveFilePersistenceUtils.h` |
| `UPersistenceSaveGame` | 저장 컨테이너 — 맵별 `FWorldPersistenceEntry` + `FPersistenceTravelData` 보관 | `Source/PersistenceUtils/Public/Data/PersistenceSaveGame.h` |
| `UMassPersistenceUtils` | Mass 엔티티 스냅샷/복원 워커(phase-safe 지연 실행) | `Source/PersistenceUtils/Public/MassPersistence/MassPersistenceUtils.h` |
| `UPersistableEntityConfigTrait` | Mass 엔티티를 영속화 대상으로 opt-in하는 trait(+`FPersistableEntityConfigFragment`) | `Source/PersistenceUtils/Public/MassPersistence/PersistableEntityConfigTrait.h` |
| `UPersistableActorReferenceManager` | 세션 간 액터 참조 해석 레지스트리(즉시/지연 콜백) | `Source/PersistenceUtils/Public/References/PersistableActorReferenceManager.h` |
| `IPersistedObject` | LSP 저장/복원 전후 훅 인터페이스(PrePersist/PostRestore) | `Source/PersistenceUtils/Public/Framework/PersistedObjectInterface.h` |

## 확장 포인트 / 규약
- **Mass 엔티티 저장**: config 자산에 `UPersistableEntityConfigTrait`를 추가하고, `UPersistenceUtilsSettings::MassFragmentsToSerialize`에 직렬화할 프래그먼트 타입을 등록. 스폰은 `APersistedMassSpawner`(또는 서브클래스에서 `ShouldSpawnEntities()` 오버라이드) 사용. IAM 소유 엔티티는 `FMassPersistenceSnapshotTag`를 달지 않아 이중 저장 방지.
- **저장 시점 게임 상태 주입**: `FPersistenceUtilsDelegates::OnPreSave` 구독(디스크 쓰기 직전) 또는 World 서브시스템의 `OnPreFlushMassEntityData`/`OnPreFlushInstancedActorsData` 구독(Mass/IAM 직렬화 직전).
- **일반 액터 프로퍼티 저장**: `UPROPERTY(SaveGame)` + `IPersistedObject` 구현(전환 로직). 세션 간 재참조가 필요하면 `UPersistableReferencedActorComponent` 부착.
- **쿡 타임 변환**: `UInstancedActorsCellTransformerBase` 상속 후 `ShouldConvertActorToInstanced()`/`GetInstancedActorsManagerClass()` 오버라이드.

## 여기서부터 읽어라
1. `Source/PersistenceUtils/Public/Data/PersistenceDataTypes.h` — 저장 스키마 전체(맵/스트리밍레벨/IAM델타/Mass스냅샷/이동데이터). 데이터 모양을 먼저 잡아야 흐름이 보인다.
2. `Source/PersistenceUtils/Public/Framework/PersistenceWorldSubsystem.h` — 세이브/복원 타이밍의 핵심. 헤더 doc-comment가 phase 경계·teardown fail-safe·복원 순서를 서술.
3. `Source/PersistenceUtils/Public/Framework/PersistenceGameSubsystem.h` — SaveGame 수명과 디스크 I/O가 World 서브시스템 플러시와 어떻게 맞물리는지.

## 관련
- 상위(소비 측): ProjectWx 게임 모듈 및 이 플러그인의 저장 델리게이트/서브시스템을 구독하는 도메인 플러그인들.

---
*문서 기준 커밋 `9554c3c` · 생성일 2026-07-08 · 소스 34파일 — `/readme-writer`로 갱신*
