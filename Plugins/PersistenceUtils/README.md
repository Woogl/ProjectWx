# PersistenceUtils — 맵 상태 세이브/로드 유틸리티

> Epic Games(저자 Zhi Kang Shao) 제공 서드파티 런타임 플러그인. 월드 파티션 맵의 런타임 상태 — 일반 액터(Level Streaming Persistence), Instanced Actor(IAM 델타), Mass 엔티티 스냅샷 — 를 `USaveGame` 파일로 직렬화/복원하고, 맵 이동 시 플레이어 위치까지 이어준다. 세 종류의 지속 데이터를 하나의 저장 흐름으로 묶는 것이 핵심.

## 책임
**담당**
- SaveGame 수명 관리: 슬롯 생성·로드·디스크 저장·리로드, 맵 간 인메모리 유지 (`UPersistenceGameSubsystem`, `USaveFilePersistenceUtils`).
- 세이브 플러시 오케스트레이션: 레벨 스트리밍 데이터·IAM·맵 이동 데이터를 동기 플러시하고, Mass 프래그먼트를 만지는 작업은 `FrameEnd` phase 경계로 지연 (`UPersistenceWorldSubsystem`).
- Mass 엔티티 스냅샷/복원: config 자산 단위로 지속 가능 엔티티를 직렬화·재스폰 (`UMassPersistenceUtils`, `UPersistableEntityConfigTrait`, `APersistedMassSpawner`).
- 세션 간 액터 참조 해석: `FPersistableActorReference`를 (레벨경로, 액터FName)으로 저장·재해석 (`UPersistableActorReferenceManager` 및 References/*).
- 쿡 타임 액터→Instanced Actor 변환 (`UInstancedActorsCellTransformerBase`).
- 저장 대상 오브젝트용 pre/post 훅 인터페이스 (`IPersistedObject`), 프로젝트 설정 (`UPersistenceUtilsSettings`).

**경계 (비담당)**
- 실제 프로퍼티 직렬화·델타 계산은 엔진 플러그인 `LevelStreamingPersistence` / `InstancedActors` / `MassEntity`에 위임 — 이 모듈은 그 hook과 flush 타이밍만 제공.
- 게임별 저장 데이터 정의·저장 시점 결정은 소비 측 게임 코드 몫 — [[WxSave]]가 이 플러그인을 래핑해 사용하는 형태(이 플러그인 자체는 Wx를 모른다).

## 의존성
- **주요 의존**: 엔진 플러그인 `LevelStreamingPersistence`, `InstancedActors`(`.uplugin` 명시). Mass 스택 `MassEntity`/`MassCore`(Public), `MassActors`/`MassSpawner`/`MassSimulation`(Private). 기타 `DeveloperSettings`, `EngineSettings`, `GameplayDebugger`. 서브시스템은 `UGameInstanceSubsystem`/`UWorldSubsystem` 기반.
- 규칙: Wx 도메인 플러그인이 아니므로 "WxCore 외 Wx 참조 금지" 규칙 **비대상**. Wx 플러그인 역참조 없음(Build.cs·`.uplugin`·소스 grep 확인) → **해당없음**. (`IsBetaVersion=true`)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `USaveFilePersistenceUtils` | BP 파사드 — StartNew/Load/Reload/Save/Travel 함수 라이브러리 | `Source/PersistenceUtils/Public/Framework/SaveFilePersistenceUtils.h` |
| `UPersistenceGameSubsystem` | GameInstance 서브시스템 — SaveGame 메모리 보유, 맵 간 이월, 디스크 write 완료 처리 | `Source/PersistenceUtils/Public/Framework/PersistenceGameSubsystem.h` |
| `UPersistenceWorldSubsystem` | World 서브시스템 — 세이브 플러시/복원 오케스트레이션(LSP·IAM·Mass·맵이동) 심장부 | `Source/PersistenceUtils/Public/Framework/PersistenceWorldSubsystem.h` |
| `UPersistenceSaveGame` | 저장 컨테이너 — 맵별 `FWorldPersistenceEntry` + `FPersistenceTravelData` | `Source/PersistenceUtils/Public/Data/PersistenceSaveGame.h` |
| `UMassPersistenceUtils` | Mass 엔티티 phase-safe 스냅샷/복원 워커 | `Source/PersistenceUtils/Public/MassPersistence/MassPersistenceUtils.h` |
| `UPersistableEntityConfigTrait` | Mass config에 지속 opt-in 마커 fragment 부착(+`FPersistableEntityConfigFragment`) | `Source/PersistenceUtils/Public/MassPersistence/PersistableEntityConfigTrait.h` |
| `UPersistableActorReferenceManager` | 세션 간 액터 참조 해석 레지스트리(즉시/지연 콜백, per-world) | `Source/PersistenceUtils/Public/References/PersistableActorReferenceManager.h` |
| `IPersistedObject` | LSP 저장 직전/복원 직후 per-object hook(PrePersist/PostRestore) | `Source/PersistenceUtils/Public/Framework/PersistedObjectInterface.h` |

## 확장 포인트 / 규약
- **Mass 엔티티 저장**: config 자산에 `UPersistableEntityConfigTrait`를 추가하고, `UPersistenceUtilsSettings::MassFragmentsToSerialize`에 직렬화할 프래그먼트 타입을 등록. 스폰은 `APersistedMassSpawner`(또는 서브클래스에서 `ShouldSpawnEntities()` 오버라이드) 사용. IAM 소유 엔티티는 `FMassPersistenceSnapshotTag`를 달지 않아 이중 저장 방지.
- **더블 스폰 방지**: `APersistedMassSpawner::bHasEverSpawned`를 LSP로 지속(헤더에 `DefaultEngine.ini` 예시 포함)하거나 `ShouldSpawnEntities()` 오버라이드.
- **저장 시점 게임 상태 주입**: `FPersistenceUtilsDelegates::OnPreSave` 구독(디스크 쓰기 직전) 또는 World 서브시스템의 `OnPreFlushMassEntityData`/`OnPreFlushInstancedActorsData` 구독(Mass/IAM 직렬화 직전).
- **일반 액터 프로퍼티 저장**: `UPROPERTY(SaveGame)` + `IPersistedObject` 구현(전환 로직). 세션 간 재참조가 필요하면 `UPersistableReferencedActorComponent` 부착 후 `FPersistableActorReference::SetFromActor`로 저장, `TryResolve`/`ResolveOrRegister`(레벨 LSP PostRestore까지 대기) 또는 BP 라텐트 노드 `UResolvePersistableActorReferenceAction`로 해석.
- **쿡 타임 변환**: `UInstancedActorsCellTransformerBase` 상속 후 `ShouldConvertActorToInstanced()`/`GetInstancedActorsManagerClass()` 오버라이드(에디터 전용).

## 여기서부터 읽어라
1. `Source/PersistenceUtils/Public/Data/PersistenceDataTypes.h` — 디스크에 실제로 담기는 자기서술 저장 스키마 전체(맵/스트리밍레벨/IAM델타/Mass스냅샷/이동데이터). 데이터 모양을 먼저 잡아야 흐름이 보인다.
2. `Source/PersistenceUtils/Public/Framework/PersistenceWorldSubsystem.h` — 세이브/복원 타이밍의 핵심. 헤더 doc-comment가 phase 경계·teardown fail-safe·복원 순서(레벨 가시화→IAM restore, FrameEnd→Mass 스냅샷)를 상세 서술.
3. `Source/PersistenceUtils/Public/Framework/SaveFilePersistenceUtils.h` — 외부에서 부르는 5개 진입점. 여기서 서브시스템으로 위임되는 경로를 잡고 시작.

## 관련
- 상위: 이 플러그인을 래핑/소비하는 [[WxSave]] 도메인(있다면). 이 플러그인 자체는 Wx를 모른다.
- 인접 엔진 플러그인: `LevelStreamingPersistence`, `InstancedActors`, `MassEntity` 계열.

---
*문서 기준 커밋 `c549ea2` · 생성일 2026-07-31 · 소스 33파일 — `/readme-writer`로 갱신*
