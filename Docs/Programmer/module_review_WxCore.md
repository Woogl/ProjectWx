# WxCore — 코드 리뷰

> 선언·상수·계약만 담는 foundation 모듈이라 여전히 매우 깨끗하다 — 🔴 결함은 없고, 실행 코드는 `WxInteractable.cpp` 한 파일(순수 로직 12줄)뿐이며 지난 리뷰의 최대 지적(`IsMeshInRange`의 무증상 실패)은 `ensureMsgf`로 이미 닫혔다. 남은 것은 계약 시그니처의 타입 분열과 그 주변 설정·규칙 정리다. 소스 9개(.h/.cpp)를 전부 통독했고, Gameplay Tag 87개는 선언↔정의 1:1 대응과 변수명↔문자열 치환 규칙을 스크립트로 전수 대조해 불일치 0을 확인했다. 태그 87개 전부가 코드 또는 에셋에서 실제로 참조되므로 데드 태그는 없고(`Gimmick.*`·`Quest.Fail`·`Ability.Pattern.*`는 코드 참조 0이지만 ST/BP `.uasset`에서 확인됨), `DefaultGameplayTags.ini`는 존재하지 않으며 WxCore 밖 태그 선언도 0건이라 선언 독점이 유지된다. `IWxInteractable`·`IWxSavable` 계약은 소비처(WxWorld 스캐너·기믹 ST 컴포넌트, WxGame 상호작용 어빌리티·적 캐릭터, WxDialogue NPC, WxInventory 픽업, WxSave 서브시스템)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 ini에만 남은 `WxInteractable` 콜리전 채널 — 헤더에 미러링되지 않아 다음 채널 추가 시 함정이 된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:16` (기준 계약은 같은 파일 `:9`), 근거 `Config/DefaultEngine.ini:38`
- **범주**: 중복/복잡도 (데드 설정)
- **문제**: 헤더는 스스로를 "DefaultEngine.ini의 채널 등록 순서와 일치해야" 하는 커스텀 채널의 단일 정의처로 선언하지만, 실제 상수는 `ECC_WxAttack`(`ECC_GameTraceChannel1`) 하나뿐이고 ini에 등록된 `ECC_GameTraceChannel2 = "WxInteractable"`은 어디에도 대응 상수가 없다. 이 채널은 지금 완전히 죽어 있다 — C++ 참조 0건이고, 상호작용 감지는 채널이 아니라 전 오브젝트 쿼리로 바뀌었으며(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:161`의 `FCollisionObjectQueryParams::AllObjects`), Content 전체 `.uasset`을 문자열 스캔해도 "WxInteractable" 채널을 쓰는 에셋이 없다. 문제는 다음에 새 Wx 채널을 추가하는 사람이 헤더만 보고 `ECC_GameTraceChannel2`가 비어 있다고 판단하는 경우다 — 그 순간 에디터 콜리전 UI에는 죽은 이름 "WxInteractable"과 `DefaultResponse=Ignore`가 붙은 채널이 재사용되어, 이름과 기본 응답이 의도와 어긋난 채 원인을 짚기 어려운 버그가 된다. 2026-07-31 리뷰에서 동일 지적, 미수정.
- **제안**: `Config/DefaultEngine.ini:38` 등록 줄을 삭제한다. 슬롯을 유지할 이유가 있다면 반대로 `ECC_WxInteractable` 상수를 헤더에 함께 두어 점유 사실을 코드로 드러낸다 — 둘 중 하나여야 하고 지금처럼 한쪽에만 있으면 안 된다.
- **확신도**: 높음 (ini·헤더·코드·Content 전수 대조).

### 2. 🟢 계약 안에서 같은 "영역 메시"가 두 타입으로 오간다 — 구현체가 매번 캐스트한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:57` vs `:35`, `:63`, `:73`, `:79`
- **범주**: 설계/구조
- **문제**: 영역 메시를 물을 땐 `const UPrimitiveComponent*`(`:48`, `:57`), 되받을 땐 `const UActorComponent*`(`:35`, `:63`, `:73`, `:79`)로 타입이 갈린다. 실제 호출부는 예외 없이 `UPrimitiveComponent*`를 넘기므로(`WxInteractionScannerComponent.cpp:87`·`:189`, `WxAbility_Interact.cpp:92`·`:97`) 넓은 타입에서 얻는 이득이 없고, 메시가 필요한 구현체만 손해를 본다 — `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:115`·`:122`가 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))`를 두 번 반복한다. 아울러 `Find`(`:35`)는 `const` 컴포넌트를 받아 비const 구현체 포인터를 돌려주므로 const 문맥에서도 상태를 바꾸는 `OnInteracted` 호출이 컴파일된다(`WxInteractionScannerComponent.cpp:76`의 `GetPrompts() const`가 실제로 const 경로에서 `Find`를 부른다 — 지금은 읽기만 해서 무해). 2026-07-31 리뷰에서 동일 지적, 미수정.
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*`로 좁힌다(호출부 변경 없음, 구현체의 `Cast` 제거). 넓은 타입이 "영역이 장차 비-프리미티브 컴포넌트가 될 수 있다"는 의도적 여지라면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음).

### 3. 🟢 헤더 인라인 함수 정의 2건 — 코딩 규칙 6 위반
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:73`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:40`
- **범주**: 규칙 위반
- **문제**: `CanBeInteractedBy`는 `{ return true; }`, `OnSaveRestored`는 `{}`를 헤더에서 바로 정의한다. CLAUDE.md 코딩 규칙 6("인라인 함수 정의를 금지한다")에 대한 문자적 위반이고, 하필 HEAD 커밋 `14a77aef`가 WxUI 헤더의 인라인 정의를 cpp로 옮긴 직후라 모듈 간 기준이 갈린 상태다. 다만 "옵셔널 훅의 기본 구현"이라는 같은 형태가 WxWorld에도 있으므로(`Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h:26`) 규칙의 암묵적 예외로 굳어져 있을 가능성도 있다 — 지금은 어느 쪽인지 문서에 없다.
- **제안**: 두 기본 구현을 cpp로 내리거나(`WxInteractable.cpp`에 추가 / `WxSavable.cpp` 신설), 반대로 "인터페이스 옵셔널 훅의 기본 구현은 규칙 6의 예외"를 CLAUDE.md에 한 줄 명시해 기준을 하나로 만든다. 세 곳이 같은 결론을 따르는 것이 요점이다.
- **확신도**: 중간 (규칙 위반은 확실, 의도된 예외인지가 미정).

### 4. 🟢 액터→컴포넌트 구현체 조회 규약이 두 계약에서 서로 다른 자리에 산다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9` vs `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:27`(대응 함수 없음), 중복 구현은 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:89`
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것, 아니면 `FindComponentByInterface`"라는 동일한 조회 규약이 두 벌 있다 — `IWxInteractable::Find`는 계약과 함께 WxCore에 있고, 같은 규약의 `UWxSaveWorldSubsystem::FindSavable`은 소비 도메인인 WxSave에 있다. 두 계약 모두 WxCore가 소유하고 컴포넌트 갈래를 쓰는 이유(호스트 액터를 순수 BP로 두기)도 주석까지 동일한데 조회 지점만 갈려 있어, 세 번째 계약을 추가할 때 어느 쪽 관례를 따를지 기준이 없다.
- **제안**: `IWxSavable::Find(AActor*)`를 WxCore에 두고 WxSave가 그것을 호출하도록 통일한다(또는 반대 방향으로 통일). 어느 쪽이든 두 계약이 같은 모양이면 된다.
- **확신도**: 낮음(의도된 설계일 수 있음 — WxSave만 액터 단위 순회를 하므로 소비처 보관이 자연스럽다는 반론이 가능하다).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(스크립트 전수 대조 + 태그별 참조처 조사), `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxActorTarget.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수·데드 코드 판정용)**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`, `.claude/asset_dump/` 및 `Content/` 원본 `.uasset` 문자열 스캔
- **이번 리뷰에서 문제없음을 확인한 항목**: 모듈 경계(`WxCore.Build.cs`는 엔진 모듈 5개뿐 — `UniversalObjectLocator`는 UE 5.8 런타임 모듈이라 `.uplugin` 의존 불필요, Plugins 의존 0), foundation 규칙(도메인 컨텐츠·데이터 타입 0 — `Gimmick.*` 태그는 태그 선언 독점 규칙에 따른 의도적 예외이며 헤더 `:104-108`에 근거가 적혀 있음, `FWxActorTarget`은 WxDialogue·WxQuest·WxUI 3개 도메인이 실제로 공유), CLAUDE.md 코딩 규칙(첫 줄 Copyright 9/9, `Wx` prefix 일관, `BlueprintCallable`·람다·델리게이트 콜백 0건 — 규칙 6은 발견 3번), 태그 선언 독점(WxCore 밖 `UE_DECLARE/DEFINE_GAMEPLAY_TAG` 0건, `DefaultGameplayTags.ini` 부재), 데드 태그 0건
- **미검토 / 한계**: 태그 사용처 판정 중 코드 참조가 0인 26개는 `.uasset` 문자열 매칭으로만 확인했다 — 매칭된 에셋이 그 태그를 유의미하게 소비하는지(예: 실제 전이 조건인지 죽은 잔재인지)까지는 파고들지 않았다. `IsMeshInRange`의 실기 반응(스켈레탈 피직스 애셋 유무별)은 정적 분석으로만 확인했고 PIE 실측은 하지 않았다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 9파일 — `/module-review`로 갱신*
