# WxCore — 코드 리뷰

> 선언·상수·계약만 담는 foundation 모듈답게 여전히 매우 깨끗하다 — 🔴 결함은 없고, 실행 코드는 `WxInteractable.cpp`의 순수 로직 세 함수뿐이며 지난 리뷰의 규칙 6 지적(헤더 인라인 정의 2건)은 `WxSavable.cpp` 신설과 `CanBeInteractedBy`의 cpp 이관으로 해소됐다. 남은 지적은 계약 조회·진단의 도달 범위와 이전 리뷰에서 미수정으로 넘어온 설정·시그니처 항목이다. 소스 10파일(.h/.cpp)을 전부 통독했고, Gameplay Tag 87개는 선언↔정의 집합 대조(불일치 0)와 변수명↔문자열 치환 규칙 전수 검사(위반 0)를 스크립트로 확인했으며, Wx 모듈 중 WxCore 밖 태그 선언은 0건으로 선언 독점이 유지된다. 계약 준수 여부는 소비처(WxWorld 스캐너·기믹 ST 컴포넌트, WxGame 상호작용 어빌리티·적 캐릭터, WxDialogue NPC, WxInventory 픽업, WxSave 서브시스템)까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `Find`의 컴포넌트 갈래가 인자 메시와 무관하다 — 액터당 구현 컴포넌트 1개라는 암묵 제약
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:24`
- **범주**: 설계/구조
- **문제**: `Find(Mesh)`는 소유 액터가 계약을 구현하지 않으면 `Owner->FindComponentByInterface(UWxInteractable::StaticClass())`가 돌려주는 **첫 번째** 컴포넌트를 답한다 — 인자로 받은 `Mesh`가 그 컴포넌트의 영역인지는 검사하지 않는다. 이 계약은 "한 액터에 상호작용 영역이 여럿"(엘리베이터 등)을 정면으로 지원하는데(`WxInteractable.h:61`·`:77`), 그 여러 영역이 **한 컴포넌트에 속할 때만** 성립한다는 제약이 코드 어디에도 적혀 있지 않다. 한 액터에 기믹 컴포넌트를 두 개 붙이면 두 번째 컴포넌트가 관리하는 메시는 첫 번째 컴포넌트에게 질의되어 `IsInteractionMeshActive`가 false를 답하고, 스캐너 후보에서 조용히 빠진다(`WxInteractionScannerComponent.cpp:194`). 실패가 닫히는 방향이라 보안 구멍은 아니지만, 경고·ensure 한 줄 없이 "그 영역만 상호작용이 안 되는" 증상으로만 드러나 원인 추적이 어렵다.
- **제안**: 제약을 코드로 드러낸다 — `Find`가 후보 컴포넌트를 순회하며 `IsInteractionMeshActive(Mesh)`가 true인 것을 고르게 하거나(호출부 변경 없음), 그럴 필요가 없다면 "액터당 구현 컴포넌트는 하나"를 `Find` 주석에 명시하고 개발 빌드에서 두 개 이상이면 `ensure`로 드러낸다.
- **확신도**: 중간 (현재 콘텐츠는 액터당 기믹 컴포넌트 1개라 실제 증상은 없다 — 의도된 단순화일 수 있으나 근거가 코드에 없다).

### 2. 🟡 `IsMeshInRange`의 `ensure`는 정작 노리는 케이스에 도달하지 못한다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:36`(근거 주석은 `WxInteractable.h:40-43`)
- **범주**: 버그/정확성 (진단 도달 범위)
- **문제**: 헤더는 "영역 메시의 쿼리 콜리전이 꺼져 있으면 상호작용이 통째로 사라지며, 개발 빌드는 이 전제가 깨지면 진입부의 ensure로 드러낸다"고 약속한다. 그런데 `IsMeshInRange`의 유일한 호출부는 서버 어빌리티 한 곳이고(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:85`), 그 경로에 들어오려면 클라 스캐너의 `OverlapMultiByObjectType`(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:166`)에 먼저 잡혀 후보·선택까지 통과해야 한다. 월드 오버랩 쿼리는 쿼리 콜리전이 켜진 컴포넌트만 반환하므로, **쿼리 콜리전을 끈 메시는 애초에 후보가 되지 않아 이 함수에 도달할 수 없다** — 즉 ensure가 잡으려던 "디자이너 설정 누락"이야말로 ensure가 관측할 수 없는 유일한 경우다. 부수적으로 `ensureMsgf`는 기본 설정에서 호출부당 1회만 발화하므로, 설령 도달하더라도 두 번째 이후의 다른 위반 메시는 침묵한다.
- **제안**: 검증 지점을 대상이 등록되는 시점으로 옮긴다 — 기믹 영역 등록(`WxStateTreeTask_EnableInteraction` 경로)이나 구현체의 `BeginPlay`에서 영역 메시의 `IsQueryCollisionEnabled()`를 확인하는 편이 실제 누락을 잡는다. `IsMeshInRange`의 ensure는 남겨도 무해하지만, 헤더 주석의 "이 전제가 깨지면 ensure로 드러난다"는 문구는 사실과 다르므로 함께 정정한다.
- **확신도**: 높음 (호출부 전수 확인 — `IsMeshInRange` 호출은 `WxAbility_Interact.cpp:85` 하나뿐).

### 3. 🟡 ini에만 남은 `WxInteractable` 콜리전 채널 — 헤더에 대응 상수가 없어 다음 채널 추가 시 함정이 된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:16`(불변식 선언은 같은 파일 `:9`), 근거 `Config/DefaultEngine.ini:38`
- **범주**: 중복/복잡도 (데드 설정)
- **문제**: 헤더는 자신을 "DefaultEngine.ini의 채널 등록 순서와 일치해야" 하는 커스텀 채널의 단일 정의처로 선언하지만 상수는 `ECC_WxAttack`(`ECC_GameTraceChannel1`) 하나뿐이고, ini에 등록된 `ECC_GameTraceChannel2 = "WxInteractable"`에는 대응 상수가 없다. 이 채널은 죽어 있다 — C++ 참조 0건, 상호작용 감지는 채널이 아니라 전 오브젝트 쿼리로 바뀌었고(`WxInteractionScannerComponent.cpp:166`의 `FCollisionObjectQueryParams::AllObjects`), 에셋 덤프에서도 이 채널을 ObjectType으로 쓰는 컴포넌트가 없다(BP에 남은 것은 전 채널 응답 배열의 잔여 항목뿐). 위험은 다음에 새 Wx 채널을 추가하는 사람이 헤더만 보고 `ECC_GameTraceChannel2`가 비어 있다고 판단하는 경우다 — 그 슬롯은 죽은 이름 "WxInteractable"과 `DefaultResponse=Ignore`를 그대로 물고 있어, 에디터 UI의 이름·기본 응답이 의도와 어긋난 채 원인을 짚기 어려운 버그가 된다. 2026-08-03 리뷰에서 동일 지적, 미수정.
- **제안**: `Config/DefaultEngine.ini:38` 등록 줄을 삭제한다. 슬롯을 유지할 이유가 있다면 반대로 `ECC_WxInteractable` 상수를 헤더에 두어 점유 사실을 코드로 드러낸다 — 한쪽에만 있는 지금 상태가 문제다.
- **확신도**: 높음 (ini·헤더·코드·에셋 덤프 대조).

### 4. 🟢 계약 안에서 같은 "영역 메시"가 두 타입으로 오간다 — 구현체가 매번 캐스트한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:57` vs `:35`, `:63`, `:73`, `:79`
- **범주**: 설계/구조
- **문제**: 영역 메시를 물을 땐 `const UPrimitiveComponent*`(`:48`, `:57`), 되받을 땐 `const UActorComponent*`(`:35`, `:63`, `:73`, `:79`)로 타입이 갈린다. 실제 호출부는 예외 없이 `UPrimitiveComponent*`를 넘기므로(`WxInteractionScannerComponent.cpp:86`·`:184`·`:194`, `WxAbility_Interact.cpp:70`·`:92`·`:97`) 넓은 타입에서 얻는 이득이 없고, 메시가 필요한 구현체만 손해를 본다 — `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:115`·`:122`가 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))`를 두 번 반복한다. 2026-08-03 리뷰에서 동일 지적, 미수정.
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*`로 좁힌다(호출부 변경 없음, 구현체의 캐스트 제거). 넓은 타입이 "영역이 장차 비-프리미티브 컴포넌트가 될 수 있다"는 의도적 여지라면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음).

### 5. 🟢 액터→컴포넌트 구현체 조회 규약이 두 계약에서 서로 다른 자리에 산다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9` vs `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:27`(대응 함수 없음), 중복 구현은 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:89`
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것, 아니면 `FindComponentByInterface`"라는 동일한 조회 규약이 두 벌 있다 — `IWxInteractable::Find`는 계약과 함께 WxCore에 있고, 같은 규약의 `UWxSaveWorldSubsystem::FindSavable`(`:89-104`)은 소비 도메인인 WxSave에 있으며 인터페이스 타입만 다른 사실상 동일 코드다. 두 계약 모두 WxCore가 소유하고 컴포넌트 갈래를 쓰는 이유(호스트 액터를 순수 BP로 두기)도 주석까지 같은데 조회 지점만 갈려 있어, 세 번째 계약을 추가할 때 따를 기준이 없다. 발견 1의 "액터당 구현체 하나" 제약도 두 곳에 각각 복제된다.
- **제안**: `IWxSavable::Find(AActor*)`를 WxCore에 두고 WxSave가 그것을 호출하도록 통일한다(또는 반대 방향으로). 어느 쪽이든 두 계약이 같은 모양이면 된다.
- **확신도**: 낮음(의도된 설계일 수 있음 — WxSave만 액터 단위 순회를 하므로 소비처 보관이 자연스럽다는 반론이 가능하다).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(선언↔정의·치환 규칙 스크립트 전수 대조), `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxActorTarget.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수·데드 코드 판정용)**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`, `.claude/asset_dump/`
- **이번 리뷰에서 문제없음을 확인한 항목**: 모듈 경계(`WxCore.Build.cs`는 엔진 모듈 5개뿐, `WxCore.uplugin`의 Plugins 의존 0 — foundation 규칙 준수), CLAUDE.md 코딩 규칙(첫 줄 Copyright 10/10, `Wx` prefix 일관, `BlueprintCallable`·람다·`FORCEINLINE`·헤더 인라인 정의 0건 — 지난 리뷰의 규칙 6 지적은 해소됨, 델리게이트 콜백 없음), Gameplay Tag 87개(선언 87 = 정의 87, 이름 집합 완전 일치, 변수명↔문자열 치환 규칙 위반 0), 태그 선언 독점(Wx 모듈 중 WxCore 밖 `UE_DECLARE/DEFINE_GAMEPLAY_TAG` 0건 — `Plugins/PersistenceExamples`는 서드파티 샘플로 규칙 대상 아님), `WxCollisionChannels.h`의 응답 서술은 실제 코드와 일치(`WxCharacterBase.cpp:26`·`:30`이 메시 Overlap·캡슐 Ignore로 override)
- **미검토 / 한계**: `Gimmick.*`(9개)·`Quest.Fail`·`Ability.Pattern.8/9`는 코드 참조가 0인데, 에셋 덤프(`.claude/asset_dump/StateTrees/*.json`)가 ST 상태의 Tag 필드를 직렬화하지 않아 사용 여부를 텍스트로 확증하지 못했다 — 헤더 `:104-108`의 설계 근거상 ST 상태 라벨로 쓰이는 것이 정상이므로 데드로 판정하지 않았으나, `Ability.Pattern.8/9`는 예약 슬롯인지 잔재인지 확인이 필요하다. `IsMeshInRange`의 실기 반응(스켈레탈 피직스 애셋 유무별)은 정적 분석으로만 확인했고 PIE 실측은 하지 않았다.

---
*문서 기준 커밋 `1e9b745c` · 리뷰일 2026-08-05 · 소스 10파일 — `/module-review`로 갱신*
