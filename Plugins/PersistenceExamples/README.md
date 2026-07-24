# PersistenceExamples — 지속성(저장/로드) 예제 모음

> Epic Games(Zhi Kang Shao) 제공 서드파티 예제 플러그인. World Partition 맵 상태를 여러 SaveGame 파일로 직렬화하고, 로드 시 플레이어를 마지막 맵/위치로 복귀시키는 데모다. 액터, InstancedActor, Mass, GAS, GeometryCollection, StateTree, JSON 등 각 지속성 패턴별 최소 예제를 담고 있다. ProjectWx 고유 코드가 아니라 참고용 레퍼런스다. (`IsBetaVersion`)

## 책임
**담당**
- `LevelStreamingPersistence`(LSP)·`PersistenceUtils` 프레임워크를 각 시스템에 붙이는 방법을 보여주는 예제 액터/컴포넌트 제공.
- 지속성 패턴별 데모: 맵 배치 액터 프로퍼티(`PersistentTimer*`), 크로스 세션 액터 참조(`FlagPole` + `PersistableReferencedActorComponent`), GAS 어트리뷰트/이펙트(`GAS/*`), GeometryCollection 파괴 상태(`PersistedGeometryCollectionComponent`), InstancedActor 인스턴스 상태(`InstancedActors/*`), Mass 엔티티↔액터 왕복(`Mass/*`, `NPCMassActorComponent`), StateTree 상태 복원(`PersistedStateTreeComponent`), JSON 직렬화(`Json/*`).

**경계 (비담당)**
- 실제 직렬화/저장 파일 관리·LSP 런타임·`IPersistedObject`·`FPersistableActorReference` 정의 → `PersistenceUtils` 플러그인(엔진 예제 제공)에 위임. 이 모듈은 소비자.
- 인스턴스화/Mass 시뮬레이션 코어 → `InstancedActors`, `MassEntity`/`MassAI` 엔진 플러그인.

## 의존성
- **주요 의존**: `PersistenceUtils`(LSP 프레임워크, `IPersistedObject`·`FPersistableActorReference`·`UPersistableActorReferenceManager`), `InstancedActors`, `MassEntity`/`MassCommon`/`MassRepresentation`/`MassActors`/`MassAIBehavior`/`MassNavigation`, `GameplayStateTreeModule`/`StateTreeModule`, `GameplayAbilities`, `GeometryCollectionEngine`/`Chaos`, `Json`/`JsonUtilities`, `UMG`, `AIModule`.
- 규칙: Epic Games 제공 외부 예제 플러그인 — Wx 참조 규칙 무관 (소스 전체에 `Wx` 심볼 0건, 순수 엔진/Epic 예제 의존만 있음).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UPersistentTimerComponent` | 맵 배치·공간분할 액터의 프로퍼티(`TimeAlive`) 지속성 최소 예제. DefaultEngine.ini에 프로퍼티 경로 등록 | `Source/PersistenceExamples/Public/LevelStreamingPersistence/PersistentTimerComponent.h` |
| `AFlagPole` | `PersistableReferencedActorComponent`로 크로스 세션 안정 ID 부여 → 타 액터가 `FPersistableActorReference`로 참조 | `Source/PersistenceExamples/Public/LevelStreamingPersistence/FlagPole.h` |
| `UPersistedAbilitySystemComponent` | `IPersistedObject` ASC. asset 태그에 `Gameplay.Effect.Persistable`이 있는 활성 GE만 옵트인 스냅샷/복원(로시) | `Source/PersistenceExamples/Public/GAS/PersistedAbilitySystemComponent.h` |
| `UPersistedGeometryCollectionComponent` | `IPersistedObject`. 파괴 조각별 트랜스폼·broken 상태를 캡처해 복원 | `Source/PersistenceExamples/Public/GeometryCollection/PersistedGeometryCollectionComponent.h` |
| `ADestructibleCrate` | 위 GAS(ASC/HealthSet)+GeometryCollection을 조합한 파괴 상자 데모 액터. `IAbilitySystemInterface` 구현 | `Source/PersistenceExamples/Public/GAS/DestructibleCrate.h` |
| `UExampleInstancedActorsSubsystem` / `AExampleInstancedActorsManager` / `UExampleInstancedActorsData` | IA 프레임워크 서브클래싱 3종 세트. 디하이드레이트 인스턴스의 Visual State를 late-join 클라에 복원. `Config/DefaultInstancedActors.ini`로 결선 | `Source/PersistenceExamples/Public/InstancedActors/` |
| `UNPCMassActorComponent` | 하이드레이트된 `ACharacter`↔백킹 Mass 엔티티 브리지. 프래그먼트 왕복·GAS health 자동 동기화 | `Source/PersistenceExamples/Public/Mass/NPCMassActorComponent.h` |
| `UPersistedStateTreeComponent` | `UStateTreeAIComponent` 서브클래스. 활성 leaf state GUID 읽기/강제 전이로 StateTree 상태 지속성 지원(호스트는 `APersistenceAIController`) | `Source/PersistenceExamples/Public/AI/PersistedStateTreeComponent.h` |

## 확장 포인트 / 규약
- 새 타입에 액터 프로퍼티 지속성을 붙이려면: `SaveGame` UPROPERTY 선언 + `Config/DefaultEngine.ini`의 `LevelStreamingPersistenceSettings`에 프로퍼티 경로 등록(`PersistentTimerComponent` 참고).
- 복잡한 상태(파괴 포즈·GE 등)는 `IPersistedObject`를 구현하고 `PrePersistObject_Implementation`/`PostRestoreObject_Implementation`에서 스냅샷↔재적용(`PersistedGeometryCollectionComponent`·`PersistedAbilitySystemComponent` 참고).
- IA/Mass는 코드가 아니라 `Config/DefaultInstancedActors.ini`·엔티티 config(`UMassRepresentationTrait`의 `HighResSpawnedActor`)로 결선한다 — 헤더 doc-comment의 지침을 따를 것.
- GE를 지속 대상으로 만들려면 asset의 GameplayEffectAssetTags에 `Gameplay.Effect.Persistable` 부여.

## 여기서부터 읽어라
1. `PersistenceExamples.uplugin` — 의존 플러그인 목록으로 어떤 지속성 프레임워크를 예시하는지 한눈에 파악.
2. `Source/PersistenceExamples/Public/LevelStreamingPersistence/PersistentTimerComponent.h` — 가장 단순한 LSP 프로퍼티 지속성 패턴(입문용).
3. `Source/PersistenceExamples/Public/GAS/DestructibleCrate.h` — LSP + `IPersistedObject`(GAS·GeometryCollection) 조합 데모, 실전 결선 예.
4. `Source/PersistenceExamples/Public/Mass/NPCMassActorComponent.h` — 액터↔Mass 엔티티 하이드레이션/디하이드레이션 왕복의 핵심 흐름·authority 모델.

## 관련
- 상위: [[PersistenceUtils]] (LSP 프레임워크·`IPersistedObject`·액터 참조 관리 — 이 예제들이 소비하는 실제 지속성 엔진)

---
*문서 기준 커밋 `c275320` · 생성일 2026-07-24 · 소스 52파일 — `/readme-writer`로 갱신*
