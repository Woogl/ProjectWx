# PersistenceExamples — 코드 리뷰

> Epic Games 제공 서드파티 지속성 예제 플러그인으로, 전반적으로 코드 품질이 높고 방어적 널 체크·주석·authority 가드가 촘촘하다. 이번 리뷰는 GAS/GeometryCollection/InstancedActors/Mass/StateTree 지속성 경로의 핵심 cpp를 깊게 보고 나머지 액터·컴포넌트는 훑었으며, 발견은 소수의 개선·사소 항목뿐이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 unsupported 버전 로드 시 "skipping record" 로그와 달리 레코드를 계속 읽는다
- **위치**: `Plugins/PersistenceExamples/Source/PersistenceExamples/Private/InstancedActors/ExampleInstancedActorComponent.cpp:253`
- **범주**: 버그/정확성
- **문제**: `Ar.IsLoading() && Version < FExampleInstancedActorVersion::InitialVersion` 분기에서 "skipping record" 경고만 찍고 `return` 하지 않는다. 이후 코드는 그대로 `Record.EnterArray(TEXT("VisualizationOverrides"), NumVizOverrides)` 및 HealthOverrides 배열을 읽어 들인다. Version 0(버전 도입 이전 포맷)처럼 레이아웃이 다른 레코드에서는 배열 카운트/원소가 현재 레이아웃과 불일치해 잘못된 값을 스테이징하거나 아카이브를 오독할 수 있다. 라인 252의 주석은 "IAM caller가 IAC size header로 레코드 본문을 seek past 한다"고 밝히나, 그렇다면 이 함수 내부의 읽기는 불필요하고, seek가 없다면 오독 위험이 남는다 — 어느 쪽이든 경고 문구("skipping")와 실제 동작이 어긋난다.
- **제안**: 버전 미달 분기에서 실제로 `return` 하거나(호출자 seek에 의존), 최소한 로그 문구를 실제 동작(계속 읽음/부분 파싱)에 맞춘다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 호출자의 size-header seek 계약에 의존)

### 2. 🟢 JSON 예제에서 직렬화/역직렬화를 중복 수행하고 첫 결과를 버린다
- **위치**: `Plugins/PersistenceExamples/Source/PersistenceExamples/Private/Json/TestActorForJson.cpp:21`
- **범주**: 중복/복잡도
- **문제**: 라인 21의 `bSaveSuccess`, 라인 25의 `bLoadSuccess`가 계산되지만 어디서도 사용되지 않고, 동일한 `UStructToJsonObjectString`(라인 27)·`JsonObjectStringToUStruct`(라인 34) 호출이 곧바로 반복된다. 즉 같은 변환을 두 번 수행하며 첫 결과(성공 플래그)는 폐기된다. 예제 코드라 실동작에는 무해하나 데드 변수 + 중복 작업이다.
- **제안**: 첫 호출의 성공 플래그를 그대로 분기에 사용하고 중복 호출을 제거한다.
- **확신도**: 높음

### 3. 🟢 IAC health flush 델리게이트가 AddWeakLambda 대신 raw this 캡처 AddLambda를 쓴다
- **위치**: `Plugins/PersistenceExamples/Source/PersistenceExamples/Private/InstancedActors/ExampleInstancedActorComponent.cpp:183`
- **범주**: 성능/안전
- **문제**: `OnPreFlushInstancedActorsData.AddLambda([this]()...)`가 raw `this`를 캡처한다. 형제 클래스 `UNPCMassActorComponent::BeginPlay`(`NPCMassActorComponent.cpp:41`)는 동일 패턴을 `AddWeakLambda(this, ...)`로 처리한다. 이 컴포넌트는 `EndPlay`에서 `PreIAMFlushHandle`을 제거하므로 정상 수명주기에서는 안전하지만, EndPlay를 거치지 않는 파괴 경로가 생기면 dangling this로 델리게이트가 실행될 여지가 있다. 일관성·안전성 측면에서 WeakLambda가 낫다.
- **제안**: `AddWeakLambda(this, ...)`로 통일한다.
- **확신도**: 낮음(EndPlay 제거로 실질 위험은 낮음 — 의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Private/GAS/PersistedAbilitySystemComponent.cpp`, `Private/GeometryCollection/PersistedGeometryCollectionComponent.cpp`, `Private/Mass/NPCMassActorComponent.cpp`, `Private/InstancedActors/ExampleInstancedActorComponent.cpp`, `Private/InstancedActors/ExampleInstancedActorsData.cpp`, `Private/AI/PersistedStateTreeComponent.cpp`, `Private/GAS/HealthAttributeSet.cpp`
- **훑은 파일**: `Private/Mass/TimeAliveMassProcessor.cpp`, `Private/Mass/NPCActorTransformToMassTranslator.cpp`, `Private/Mass/NPCNavTargetEvaluator.cpp`, `Private/GAS/DamageExecutionCalculation.cpp`, `Private/GAS/DestructibleCrate.cpp`, `Private/GAS/HealthBarWidget.cpp`, `Private/Json/TestActorForJson.cpp`, `Private/LevelStreamingPersistence/*.cpp`, `Private/InstancedActors/{JumpBlock,ExampleInstancedActorsManager,ExampleInstancedActorsSubsystem,ExampleInstancedActorVersion}.cpp`, `PersistenceExamples.Build.cs`
- **미검토 / 한계**: 프래그먼트/헤더 전용 파일(`Mass/*Fragments.h`, `HealthFragment.cpp`)과 다수의 Public 헤더는 선언 확인 수준으로만 봄. `PersistenceUtils` 프레임워크(`IPersistedObject`·`UPersistableActorReferenceManager`)의 계약은 이 모듈 외부라 그 실제 seek/수명 보장은 검증하지 못함 — 발견 1의 확신도를 낮춤으로 둔 이유. 본 플러그인은 Epic 제공 예제(`// Copyright Epic Games`)이므로 Wx prefix·Copyright Woogle·Handle prefix 등 프로젝트 CLAUDE.md 네이밍 규칙은 적용 대상이 아니라고 보아 규칙 위반으로 계상하지 않음.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 52파일 — `/module-review`로 갱신*
