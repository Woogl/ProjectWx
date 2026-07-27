# PersistenceExamples — 영속성 예제 모음

> Epic이 제공하는 서드파티/예제 플러그인. World Partition 맵 상태를 SaveGame으로 직렬화해 재접속 시 마지막 맵·위치로 복귀시키는 흐름을 중심으로, 일반 액터·Instanced Actors·Mass·GAS·StateTree·Chaos를 어떻게 영속화하는지 카테고리별 데모로 보여준다.

## 책임
**담당**
- `PersistenceUtils`(LevelStreamingPersistence) 위에서 각 서브시스템별 저장/복원 패턴 시연
  - 맵 배치 WP 액터의 property-path 영속화 (`UPersistentTimerComponent`)
  - Instanced Actors의 다중 비주얼 상태·체력·희소 델타 영속화 (`UExampleInstancedActorComponent`)
  - Mass 엔티티 ↔ 하이드레이트 액터 간 상태 왕복 (`UNPCMassActorComponent`, `NPCMassFragments`)
  - 활성 GameplayEffect 부분 스냅샷 저장/복원 (`UPersistedAbilitySystemComponent`)
  - StateTree 활성 상태 GUID 저장/강제 전이 (`UPersistedStateTreeComponent`)
  - GeometryCollection(Chaos) 파괴 상태·JSON 직렬화 데모

**경계 (비담당)**
- 실제 저장/복원 파이프라인·직렬화·PersistedObject 인터페이스는 전부 `PersistenceUtils` 플러그인에 위임 (이 모듈은 이를 소비만 함)
- Instanced Actors / Mass 프레임워크 코어는 엔진 플러그인(`InstancedActors`, `MassGameplay`/`MassAI`)에 위임

## 의존성
- **주요 의존**: `PersistenceUtils`(핵심 — `IPersistedObject`, `FPersistableActorReference`, LevelStreamingPersistence 세팅), `InstancedActors`, `MassEntity`/`MassActors`/`MassRepresentation` 및 `MassAIBehavior`/`MassNavigation`, `GameplayAbilities`, `GameplayStateTreeModule`, `Chaos`/`GeometryCollectionEngine`
- 규칙: 서드파티/예제 플러그인 — 「WxCore 외 Wx 플러그인 참조 금지」 규칙 대상 아님(규칙 무관)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UExampleInstancedActorComponent` | IA 다중 비주얼 상태 + GAS 체력 동기화 + 희소 영속화 컴포넌트 | `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorComponent.h` |
| `UExampleInstancedActorsSubsystem` / `AExampleInstancedActorsManager` | 커스텀 IA 서브시스템·매니저 오버라이드 (config로 주입) | `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorsSubsystem.h` |
| `UNPCMassActorComponent` | 하이드레이트 `ACharacter` ↔ Mass 엔티티 상태 브리지 | `Source/PersistenceExamples/Public/Mass/NPCMassActorComponent.h` |
| `FNPCMassSnapshot` / `FNPC*Fragment` | Mass 엔티티에 저장되는 NPC 상태·태그 프래그먼트 | `Source/PersistenceExamples/Public/Mass/NPCMassFragments.h` |
| `UPersistedAbilitySystemComponent` | `Gameplay.Effect.Persistable` 태그 GE만 스냅샷·복원하는 ASC | `Source/PersistenceExamples/Public/GAS/PersistedAbilitySystemComponent.h` |
| `UPersistentTimerComponent` | property-path 등록만으로 WP 액터 필드 영속화하는 최소 예제 | `Source/PersistenceExamples/Public/LevelStreamingPersistence/PersistentTimerComponent.h` |
| `UPersistedStateTreeComponent` | 활성 leaf 상태 GUID 읽기·강제 전이용 StateTree 컴포넌트 | `Source/PersistenceExamples/Public/AI/PersistedStateTreeComponent.h` |
| `ATestActorForJson` | JSON 직렬화 데모용 액터 | `Source/PersistenceExamples/Public/Json/TestActorForJson.h` |

## 확장 포인트 / 규약
- 새 영속 오브젝트: `IPersistedObject`(PersistenceUtils)를 구현하고 `PrePersistObject`/`PostRestoreObject`에서 스냅샷을 채우며, 대상 `UPROPERTY(SaveGame)`를 `DefaultEngine.ini`의 `LevelStreamingPersistenceSettings`에 등록한다 (`UPersistedAbilitySystemComponent`가 표준 패턴).
- GE 영속화 opt-in: GE 에셋 태그에 `Gameplay.Effect.Persistable` 추가 (`GetPersistableEffectTag()`).
- IA 커스터마이징: `UExampleInstancedActorComponent`를 exemplar에 붙여 비주얼 상태·체력을 데이터 주도로 구성하고, 서브시스템/매니저 클래스 교체는 `Config/DefaultInstancedActors.ini`로 주입한다 (헤더 doc-comment 참조).
- Mass NPC: BP를 `APersistenceLabCharacter` 파생으로 만들고 `UNPCMassActorComponent`의 `OnHydrated`/`OnFlushPending`을 바인딩해 상태를 왕복시킨다.

## 여기서부터 읽어라
1. `PersistenceExamples.uplugin` — 어떤 엔진 플러그인(PersistenceUtils/IA/Mass/StateTree/GAS)에 의존하는지, 데모 범위 파악
2. `Source/PersistenceExamples/Public/InstancedActors/ExampleInstancedActorComponent.h` — 가장 밀도 높은 진입점. IA + Mass + GAS 영속화가 한 컴포넌트에 모여 있음
3. `Source/PersistenceExamples/Public/Mass/NPCMassActorComponent.h` + `NPCMassFragments.h` — 액터/Mass 이중 표현의 상태 왕복 authority 모델
4. `Source/PersistenceExamples/Public/GAS/PersistedAbilitySystemComponent.h` — `IPersistedObject` 구현 표준 패턴(스냅샷 → LSP 직렬화 → 지연 복원)
5. `Source/PersistenceExamples/Public/LevelStreamingPersistence/PersistentTimerComponent.h` — 코드 최소의 영속화 예제, 개념 입문용

## 관련
- 상위: 독립 예제 플러그인. `PersistenceUtils`(LevelStreamingPersistence)를 소비하며, 프로젝트의 `Wx*` 플러그인과는 직접 연결되지 않는 참고용 레퍼런스

---
*문서 기준 커밋 `21e2e76` · 생성일 2026-07-27 · 소스 52파일 — `/readme-writer`로 갱신*
