# PersistenceUtils — 코드 리뷰

> Epic Games 제공 서드파티 영속화 유틸리티 플러그인. 전반적으로 성숙하고 방어적으로 작성돼 있으며 수명주기·재진입 처리가 꼼꼼하다. 심각한 버그는 없고, 셧다운 정리 비대칭 1건과 데드코드/문서 불일치 1건이 눈에 띈다. Framework(World/Game 서브시스템)·MassPersistence·References 3개 핵심 축과 모듈 부트스트랩까지 cpp를 깊게 봤다. Wx 네이밍·Copyright 규칙은 벤더링된 Epic 코드라 적용 대상이 아니라고 판단.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 ShutdownModule 이 StartupModule 과 다른 UClass 로 언바인드 (정리 누락)
- **위치**: `Plugins/PersistenceUtils/Source/PersistenceUtils/Private/PersistenceUtils.cpp:47`, `:57` (바인드) vs `:209`, `:210` (언바인드)
- **범주**: 버그/정확성 — 대칭성 결함
- **문제**: `StartupModule`은 `LSP.OnPrePersistObject(UObject::StaticClass())`, `LSP.OnPostRestoreObject(UObject::StaticClass())`로 바인딩하는데, `ShutdownModule`은 `LSP.OnPrePersistObject(AActor::StaticClass()).Unbind()`, `LSP.OnPostRestoreObject(AActor::StaticClass()).Unbind()`로 언바인딩한다. LSP 의 이 접근자는 UClass 키별 델리게이트를 반환하므로, `UObject` 슬롯에 걸린 바인딩은 셧다운 시 해제되지 않고 `AActor` 슬롯 언바인드는 사실상 no-op 이 된다. (`USceneComponent` 두 건은 바인드/언바인드 클래스가 일치해 문제 없음.) 바인딩된 람다가 캡처 없는 static 이라 댕글링·크래시 위험은 없지만, 모듈 재로드/핫리로드 시 LSP 에 스테일 바인딩이 남는다.
- **제안**: 언바인드 클래스를 바인드와 동일하게 `UObject::StaticClass()`로 맞춘다.
- **확신도**: 중간 (LSP 접근자가 UClass 키별이라는 전제. 코드 자체의 바인드/언바인드 클래스 비대칭은 명백)

### 2. 🟡 FMassPersistenceSnapshotTag 데드코드 + README 문서 불일치
- **위치**: `Plugins/PersistenceUtils/Source/PersistenceUtils/Public/Data/PersistenceDataTypes.h:34-38` (정의), 참조처 없음
- **범주**: 중복/복잡도 — 데드코드 / 오해 유발 문서
- **문제**: `FMassPersistenceSnapshotTag`는 정의만 있고 모듈 전체에서 단 한 번도 참조되지 않는다(grep 결과 정의 라인이 유일). 헤더 주석과 README(35번 항목 포함)는 이 태그를 "스냅샷 대상 opt-in 마커"로 설명하지만, 실제 스냅샷 게이트는 `DoSnapshotWork`의 `AddRequirement<FPersistableEntityConfigFragment>`(MassPersistenceUtils.cpp:233)다. 즉 태그를 달아도 저장 대상 판정에 아무 영향이 없어, 소비 측이 문서대로 태그를 붙이면 조용히 저장에서 누락된다.
- **제안**: (a) 태그를 실제 쿼리 필터로 편입하거나, (b) 미사용이면 제거하고 README·주석을 `FPersistableEntityConfigFragment` 기준으로 정정한다.
- **확신도**: 중상 (미사용은 확실. 외부/미래용 예약일 가능성은 남김)

### 3. 🟡 Mass 프래그먼트 스냅샷이 raw memcpy 직렬화 — 비POD 프래그먼트에 위험
- **위치**: `Plugins/PersistenceUtils/Source/PersistenceUtils/Private/MassPersistence/MassPersistenceUtils.cpp:299`(저장), `:437`(복원)
- **범주**: 성능/안전 — 검증 없는 바이트 복사
- **문제**: 프래그먼트를 `MemWriter.Serialize(FragView.GetMemory(), FragType->GetStructureSize())`로 구조체 크기만큼 통째로 바이트 복사한다. `UScriptStruct::SerializeItem` 을 거치지 않으므로 `FString`/`TArray`/`TObjectPtr` 같은 비-trivially-copyable 멤버를 가진 프래그먼트는 힙 포인터가 그대로 저장되어 세션 간 복원 시 손상·해제후사용으로 이어진다. Origin 프래그먼트(`FPersistableEntityConfigFragment`)를 `TObjectPtr` 이유로 명시 제외(`:274-279`)한 데서 저자도 인지하고 있으나, allow-list 등록 책임을 사용자에게 넘길 뿐 POD 여부를 강제하지 않는다.
- **제안**: allow-list 등록 시(또는 저장 직전) 프래그먼트가 POD/trivially-copyable 인지 검증하거나, 커스텀 직렬화가 필요한 타입은 명시적으로 거부/경고한다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 사용자가 POD 프래그먼트만 등록한다는 전제)

### 4. 🟢 StartupModule 전체가 UE_DISABLE_OPTIMIZATION 로 감싸짐
- **위치**: `Plugins/PersistenceUtils/Source/PersistenceUtils/Private/PersistenceUtils.cpp:36`, `:198`
- **범주**: 성능/안전 — 디버깅 잔재 가능성
- **문제**: `FPersistenceUtilsModule::StartupModule` 전체가 `UE_DISABLE_OPTIMIZATION`/`UE_ENABLE_OPTIMIZATION` 로 감싸져 있어 Shipping/Development 빌드에서도 이 함수가 최적화 없이 컴파일된다. 부트스트랩 1회 호출이라 런타임 영향은 미미하나 디버깅 목적 잔재로 보인다.
- **제안**: 의도적(브레이크포인트 안정화 등)이 아니면 제거한다.
- **확신도**: 중간

### 5. 🟢 PIE 진입 시 하드코딩 슬롯("PIETestFile") 자동 로드
- **위치**: `Plugins/PersistenceUtils/Source/PersistenceUtils/Private/Framework/PersistenceGameSubsystem.cpp:16-25`
- **범주**: 설계/구조 — 테스트 스캐폴딩 상주
- **문제**: `Initialize`에서 `WITH_EDITOR && IsPlayInEditor()` 조건으로 `"PIETestFile"` 슬롯을 자동 로드/생성한다. 에디터 전용이라 배포엔 영향 없지만, 플러그인 코드에 개발용 슬롯명이 박혀 있어 여러 세이브 테스트 시나리오와 충돌할 수 있다.
- **제안**: 슬롯명을 세팅(`UPersistenceUtilsSettings`)으로 노출하거나 개발용 토글로 감싼다.
- **확신도**: 낮음 (개발 편의용 의도가 명확)

## 검토 범위
- **깊게 본 파일**: `Private/Framework/PersistenceWorldSubsystem.cpp`, `Private/Framework/PersistenceGameSubsystem.cpp`, `Private/MassPersistence/MassPersistenceUtils.cpp`, `Private/References/PersistableActorReferenceManager.cpp`, `Private/References/ResolvePersistableActorReferenceAction.cpp`, `Private/PersistenceUtils.cpp`, `Private/CellTransformer/InstancedActorsCellTransformerBase.cpp`, `Public/Framework/PersistenceWorldSubsystem.h`, `Public/References/PersistableActorReferenceManager.h`
- **훑은 파일**: `Private/Framework/SaveFilePersistenceUtils.cpp`, `Private/References/PersistableActorReference.cpp`, `Private/References/PersistableReferencedActorComponent.cpp`, `Private/MassPersistence/PersistedMassSpawner.cpp`, `Private/MassPersistence/PersistableEntityConfigTrait.cpp`, `Private/PersistenceUtilsSettings.cpp`, `Public/Data/PersistenceDataTypes.h`, `PersistenceUtils.Build.cs`, `README.md`
- **미검토 / 한계**: 소형 stub cpp(`PersistenceDataTypes.cpp`·`PersistenceSaveGame.cpp`·`PersistenceUtilsDelegates.cpp` — 본문 사실상 없음)와 대응 헤더 일부는 표면만 확인. finding 1은 엔진 `LevelStreamingPersistence` 델리게이트 접근자의 UClass-키 시맨틱 전제에 의존하며, 해당 엔진 헤더 자체는 미검증. Mass raw 직렬화(finding 3)의 실제 위험도는 프로젝트가 등록한 프래그먼트 타입 구성에 좌우되어 정적으로 단정 불가.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 33파일 — `/module-review`로 갱신*
