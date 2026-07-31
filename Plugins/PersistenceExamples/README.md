# PersistenceExamples — 퍼시스턴스 예제 모음

> Epic의 PersistenceUtils(LevelStreamingPersistence) 위에 얹은 저장/복원 데모 플러그인. World Partition 맵 상태를 SaveGame으로 직렬화하고 재접속 시 마지막 맵·위치로 복귀시키는 흐름을, 일반 액터·InstancedActors·Mass 엔티티·GAS/GeometryCollection·StateTree 각각의 관점에서 보여주는 예제 카탈로그다. `Wx` 접두어 없는 샘플 성격의 런타임 플러그인.

## 책임
**담당**
- PersistenceUtils의 LSP(LevelStreamingPersistence) 훅(`IPersistedObject`, `PersistableActorReference`, DefaultEngine.ini 프로퍼티 경로 등록)을 실제 액터/컴포넌트에서 쓰는 법 시연.
- 맵 배치 WP 액터의 property-path 영속화 최소 예제(`APersistentTimerActor` / `UPersistentTimerComponent`).
- InstancedActors 확장 예제: Subsystem/Manager/Data/Component 서브클래싱으로 dehydrated 인스턴스의 Visual State·Health를 스파스 델타로 영속·복제.
- Mass 하이브리드(actor-authoritative) 예제: 하이드레이션된 액터 ↔ Mass 엔티티 간 상태 왕복, 커스텀 Translator/Processor/Evaluator.
- GAS 영속 예제: 활성 GameplayEffect 스냅샷 저장·복원, HealthAttributeSet, DamageExecution, GeometryCollection(Chaos) 파괴 포즈 영속.
- StateTree(AI) 영속 예제: 활성 leaf state GUID 저장 후 재하이드레이션 시 강제 전이. JSON 직렬화 데모.

**경계 (비담당)**
- 실제 저장 파이프라인·맵 트래블·프로퍼티 직렬화·`IPersistedObject` 정의 자체는 `PersistenceUtils` 엔진 플러그인이 담당(이 모듈은 소비자).
- InstancedActors / Mass 프레임워크 코어는 엔진 플러그인(`InstancedActors`, `MassGameplay`/`MassAI`)에 위임.
- 이 프로젝트의 실제 게임 시스템(전투·인벤토리 등)과 무관 — 참조 관계 없음.

## 의존성
- **주요 의존**: `PersistenceUtils`(핵심 — `IPersistedObject`, `FPersistableActorReference`, LevelStreamingPersistence 세팅), `InstancedActors`, `MassEntity/MassActors/MassRepresentation` 및 `MassAIBehavior/MassNavigation/MassSignals`, `GameplayAbilities`, `GameplayStateTree`/`StateTreeModule`, `Chaos`/`GeometryCollectionEngine`, `Json`/`JsonUtilities`, `AIModule`, `UMG`.
- 규칙: Wx 도메인 규칙 비대상 — 예제 플러그인(해당없음). Wx 플러그인 역참조 없음.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UExampleInstancedActorComponent` | IA 인스턴스의 다중 Visual State·GAS Health를 스파스 델타로 영속. IA 예제의 중심 | `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorComponent.h` |
| `UExampleInstancedActorsData` | dehydrated 인스턴스의 Visual State를 lifecycle-phase 리플리케이션에 얹어 late-joiner에 복제 | `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorsData.h` |
| `UExampleInstancedActorsSubsystem` / `AExampleInstancedActorsManager` | 위 Data/Component를 쓰도록 IA subsystem·manager 클래스를 교체(Config로 주입) | `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorsSubsystem.h`, `.../ExampleInstancedActorsManager.h` |
| `UNPCMassActorComponent` / `FNPCMassSnapshot` | 하이드레이션된 `ACharacter` ↔ Mass 엔티티 브릿지(EndPlay/flush 시 fragment 기록, GAS Health 자동 동기) | `Source/PersistenceExamples/Public/Mass/NPCMassActorComponent.h`, `.../Mass/NPCMassFragments.h` |
| `UPersistedAbilitySystemComponent` | `Gameplay.Effect.Persistable` 태그가 붙은 활성 GE만 스냅샷 저장·복원(`IPersistedObject`) | `Source/PersistenceExamples/Public/GAS/PersistedAbilitySystemComponent.h` |
| `UPersistedGeometryCollectionComponent` | 파괴 조각별 transform·broken 상태를 영속하고 물리 프록시 재생성으로 복원 | `Source/PersistenceExamples/Public/GeometryCollection/PersistedGeometryCollectionComponent.h` |
| `UPersistedStateTreeComponent` / `APersistenceAIController` | StateTree 활성 leaf GUID 조회·강제 전이(영속용). 컨트롤러가 컴포넌트 호스팅 | `Source/PersistenceExamples/Public/AI/PersistedStateTreeComponent.h`, `.../AI/PersistenceAIController.h` |
| `APersistentTimerActor` / `AFlagPole` / `ADestructibleCrate` | LSP property-path·`PersistableActorReference`·GAS+GeometryCollection 영속을 각각 시연하는 대표 액터 | `Source/PersistenceExamples/Public/LevelStreamingPersistence/`, `.../GAS/DestructibleCrate.h` |

## Gameplay Tags
Native 태그는 cpp 내부 static 선언(`UE_DEFINE_GAMEPLAY_TAG_STATIC`)으로만 존재 — 외부 export용 헤더 선언은 없음.
- `Gameplay.Effect.Persistable` — GE 영속 opt-in 마커. `Source/.../GAS/PersistedAbilitySystemComponent.cpp` (`UPersistedAbilitySystemComponent::GetPersistableEffectTag()`로 노출).
- `Abilities.Parameters.Damage` — DamageExecution의 SetByCaller 키. `Source/.../GAS/DamageExecutionCalculation.cpp`.

## 확장 포인트 / 규약
- 새 영속 오브젝트: `IPersistedObject`를 구현해 `PrePersistObject`/`PostRestoreObject`에서 스냅샷을 채우고, 대상 `UPROPERTY(SaveGame)`를 `Config/DefaultEngine.ini`의 `LevelStreamingPersistenceSettings` 프로퍼티 경로에 등록해야 저장된다(`UPersistedAbilitySystemComponent`가 표준 패턴).
- 인스티게이터 등 액터 참조는 레벨 LSP post-restore flush 이후에 `UPersistableActorReferenceManager`로 resolve.
- GE 영속 opt-in: GE 에셋 태그에 `Gameplay.Effect.Persistable` 추가.
- IA 확장은 **Config로 활성화**된다: `Config/DefaultInstancedActors.ini`에서 subsystem/manager 클래스를 예제 클래스로 지정해야 하며, 이미 변환·저장된 인스턴스는 재변환 필요(헤더 doc-comment 참고).
- Mass 예제는 actor-authoritative 하이브리드: 액터가 하이드레이션 동안 authority, EndPlay/flush 시점에 fragment로 write-back. `FNPCHydratedTag`/`FNPCDeadTag`로 프로세서 필터.

## 여기서부터 읽어라
1. `PersistenceExamples.uplugin` — 의존 엔진 플러그인 목록으로 예제 스코프(PersistenceUtils·InstancedActors·Mass·GAS·StateTree·Chaos) 파악.
2. `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorComponent.h` — 이 모듈의 가장 밀도 높은 진입점. Visual State·Health 스파스 영속 규약이 여기 다 있다.
3. `Source/PersistenceExamples/Public/Mass/NPCMassActorComponent.h` + `Mass/NPCMassFragments.h` — actor↔Mass 이중 표현의 authority 모델과 fragment 계약.
4. `Source/PersistenceExamples/Public/GAS/PersistedAbilitySystemComponent.h` — `IPersistedObject` 기반 스냅샷→LSP 직렬화→지연 복원 패턴의 정석.
5. `Source/PersistenceExamples/Public/LevelStreamingPersistence/PersistentTimerComponent.h` — 코드 최소의 영속화 예제, 개념 입문용.

## 관련
- 상위/기반: `Plugins/PersistenceUtils`(LSP·PersistableActorReference·IPersistedObject 정의처)를 소비. 엔진 플러그인 `InstancedActors`, `MassGameplay`/`MassAI`, `GameplayAbilities`, `GameplayStateTree`.
- 프로젝트의 `Wx*` 플러그인과는 직접 연결되지 않는 참고용 레퍼런스(원저작: Epic Games / Zhi Kang Shao).

---
*문서 기준 커밋 `c549ea2` · 생성일 2026-07-31 · 소스 52파일 — `/readme-writer`로 갱신*
