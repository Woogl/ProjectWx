# WxCore — 코드 리뷰

> 선언·상수·계약만 담는 foundation 모듈답게 여전히 매우 깨끗하다 — 🔴 결함은 없고, 실행 코드는 `WxInteractable.cpp` 의 세 함수(약 30줄)가 전부다. 지난 리뷰의 ini 데드 채널 지적(`ECC_GameTraceChannel2 = "WxInteractable"`)은 커밋 `cddbf837` 로 해소돼 목록에서 뺐고, 남은 두 건은 이전 리뷰에서 넘어온 계약 조회·시그니처 항목이다. 소스 10파일(.h/.cpp)을 전부 통독했고, Gameplay Tag 88개는 선언↔정의 집합 대조(불일치 0)와 변수명↔문자열 치환 규칙 전수 검사(위반 0)를 스크립트로 확인했으며, 프로젝트에 `DefaultGameplayTags.ini` 가 없고 WxCore 밖 Wx 모듈의 태그 선언도 0건이라 선언 독점이 유지된다. 계약 준수 여부는 소비처(WxWorld 스캐너·기믹 ST 컴포넌트, WxGame 상호작용 어빌리티·적 캐릭터, WxDialogue NPC, WxInventory 픽업, WxSave 월드 서브시스템)의 cpp 까지 내려가 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `IsMeshInRange` 의 `ensure` 는 잡으려는 케이스에 도달하지 못하고, 정작 헤더가 정당한 off-switch 로 문서화한 경로에서만 발화한다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:36` (근거 주석은 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:40-43`)
- **범주**: 버그/정확성 (진단 도달 범위 + 계약 자기모순)
- **문제**: 두 가지가 겹쳐 있다.
  1. **노리는 케이스에 도달할 수 없다.** 헤더는 "쿼리 콜리전이 꺼지면 상호작용이 통째로 사라지며, 개발 빌드는 진입부 ensure 로 드러낸다"고 약속하지만, `IsMeshInRange` 의 유일한 호출부는 서버 어빌리티 한 곳이고(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:85`), 그 경로에 들어오려면 클라 스캐너의 `OverlapMultiByObjectType`(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:166`)에 먼저 잡혀야 한다. 월드 오버랩은 쿼리 콜리전이 켜진 컴포넌트만 돌려주므로, ensure 가 잡으려던 "디자이너가 콜리전을 안 켠 영역 메시"는 애초에 후보가 되지 못해 이 함수에 닿지 않는다.
  2. **발화 가능한 유일한 경로가 정당한 사용법이다.** 같은 헤더 `:42` 는 "영역 메시를 NoCollision 으로 내리는 것은 그 대상의 상호작용을 통째로 끄는 것과 같다"고 적어 콜리전 내리기를 사실상 off-switch 로 안내하는데, `AWxNpc::SetInteractionEnabled`(`Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp:67-70`)가 실제로 그 방식을 쓴다. 그 결과 `AWxNpc` 는 ensure 를 피하려고 `IsInteractionMeshActive` 에 콜리전 검사를 손수 복제해 넣어야 했고(`WxNpc.cpp:46` — 주석이 "사거리 판정의 ensure 에 닿기 전에 걸러진다"고 이유를 명시), 이 방어를 잊은 구현체는 서버 검증 순서상 `IsInteractionMeshActive` 통과 → `IsMeshInRange` 진입으로 정상 플레이 중 ensure 를 맞는다. 즉 WxCore 의 진단 장치가 소비처에 방어 코드를 강제하는 형태로 새어 나가 있다.
- **제안**: ensure 를 영역이 등록되는 시점(기믹의 `WxStateTreeTask_EnableInteraction` 경로, 구현체 `BeginPlay` 등)으로 옮기면 실제 설정 누락을 잡는다. 옮기지 않고 남긴다면 `IsMeshInRange` 의 ensure 를 제거하고, 헤더 `:40-43` 의 "전제가 깨지면 ensure 로 드러난다"는 문구를 사실에 맞게 정정하면서 "콜리전으로 상호작용을 끄는 구현체는 `IsInteractionMeshActive` 에서도 같은 판정을 해야 한다"를 계약으로 명시한다.
- **확신도**: 높음 (`IsMeshInRange` 호출부 전수 확인 — `WxAbility_Interact.cpp:85` 하나뿐이고, `AWxNpc` 의 우회 주석이 문제를 직접 증언한다).

### 2. 🟡 `Find` 의 컴포넌트 갈래가 인자 메시와 무관하다 — 액터당 구현 컴포넌트 1개라는 암묵 제약
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:24`
- **범주**: 설계/구조
- **문제**: `Find(Mesh)` 는 소유 액터가 계약을 구현하지 않으면 `Owner->FindComponentByInterface(UWxInteractable::StaticClass())` 가 돌려주는 **첫 번째** 컴포넌트를 답한다 — 인자로 받은 `Mesh` 가 그 컴포넌트의 영역인지는 검사하지 않는다. 이 계약은 "한 액터에 상호작용 영역이 여럿"(엘리베이터 등)을 정면으로 지원하는데(`WxInteractable.h:61`·`:77`), 그 여러 영역이 **한 컴포넌트에 속할 때만** 성립한다는 제약이 코드 어디에도 적혀 있지 않다. 한 액터에 기믹 컴포넌트를 두 개 붙이면 두 번째 컴포넌트가 관리하는 메시가 첫 번째 컴포넌트에게 질의되어 `IsInteractionMeshActive` 가 false 를 답하고(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:102` 의 `InteractionRegions.Contains`), 스캐너 후보에서 조용히 빠진다(`WxInteractionScannerComponent.cpp:184-198`). 실패가 닫히는 방향이라 보안 구멍은 아니지만, 경고·ensure 한 줄 없이 "그 영역만 상호작용이 안 되는" 증상으로만 드러나 원인 추적이 어렵다.
- **제안**: 제약을 코드로 드러낸다 — `Find` 가 후보 컴포넌트를 순회하며 `IsInteractionMeshActive(Mesh)` 가 true 인 것을 고르게 하거나(호출부 변경 없음), 그럴 필요가 없다면 "액터당 구현 컴포넌트는 하나"를 `Find` 주석에 명시하고 개발 빌드에서 둘 이상이면 `ensure` 로 드러낸다.
- **확신도**: 중간 (현재 콘텐츠는 액터당 기믹 컴포넌트 1개라 실제 증상은 없다 — 의도된 단순화일 수 있으나 근거가 코드에 없다).

### 3. 🟢 계약 안에서 같은 "영역 메시"가 두 타입으로 오간다 — 구현체가 매번 캐스트한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:57` vs `:35`, `:63`, `:73`, `:79`
- **범주**: 설계/구조
- **문제**: 영역 메시를 물을 땐 `const UPrimitiveComponent*`(`:48`, `:57`), 되받을 땐 `const UActorComponent*`(`:35`, `:63`, `:73`, `:79`)로 타입이 갈린다. 실제 호출부는 예외 없이 `UPrimitiveComponent*` 를 넘기므로(`WxInteractionScannerComponent.cpp:86`·`:184`·`:194`, `WxAbility_Interact.cpp:70`·`:78`·`:85`·`:92`·`:97`) 넓은 타입에서 얻는 이득이 없고, 메시가 필요한 구현체만 손해를 본다 — `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:115`·`:122` 가 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))` 를 두 번 반복한다. 2026-08-03·08-05 리뷰에서 동일 지적, 미수정.
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*` 로 좁힌다(호출부 변경 없음, 구현체의 캐스트 제거). 넓은 타입이 "영역이 장차 비-프리미티브 컴포넌트가 될 수 있다"는 의도적 여지라면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음).

### 4. 🟢 액터→컴포넌트 구현체 조회 규약이 두 계약에서 서로 다른 자리에 산다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9` vs `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:27`(대응 함수 없음), 중복 구현은 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:89-104`
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것, 아니면 `FindComponentByInterface`" 라는 동일한 조회 규약이 두 벌 있다 — `IWxInteractable::Find` 는 계약과 함께 WxCore 에 있고, 같은 규약의 `UWxSaveWorldSubsystem::FindSavable` 은 소비 도메인인 WxSave 에 있으며 인터페이스 타입만 다른 사실상 동일 코드다(주석 문구까지 대응된다). 두 계약 모두 WxCore 가 소유하고 컴포넌트 갈래를 쓰는 이유(호스트 액터를 순수 BP 로 두기)도 같은데 조회 지점만 갈려 있어, 세 번째 계약을 추가할 때 따를 기준이 없다. 발견 2의 "액터당 구현체 하나" 제약도 두 곳에 각각 복제된다.
- **제안**: `IWxSavable::Find(AActor*)` 를 WxCore 에 두고 WxSave 가 그것을 호출하도록 통일한다(또는 반대 방향으로). 어느 쪽이든 두 계약이 같은 모양이면 된다.
- **확신도**: 낮음(의도된 설계일 수 있음 — WxSave 만 액터 단위 순회를 하므로 소비처 보관이 자연스럽다는 반론이 가능하다).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` + `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(선언↔정의·치환 규칙 스크립트 전수 대조), `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxActorTarget.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수·데드 코드 판정용)**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxNpc.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`, 엔진 `BaseEngine.ini`(Ragdoll 프로파일), `.claude/asset_dump/`
- **이번 리뷰에서 문제없음을 확인한 항목**:
  - 모듈 경계 — `WxCore.Build.cs` 는 엔진 모듈 5개(`Core`/`CoreUObject`/`Engine`/`GameplayTags`/`UniversalObjectLocator`)뿐, `WxCore.uplugin` 의 Plugins 의존 0. foundation 규칙 준수.
  - CLAUDE.md 코딩 규칙 — 첫 줄 Copyright 11/11(Build.cs 포함), `Wx` prefix 일관, `BlueprintCallable`·람다·`FORCEINLINE`·헤더 인라인 정의 0건, 델리게이트 콜백 없음.
  - Gameplay Tag — 선언 88 = 정의 88, 이름 집합 완전 일치, 변수명↔문자열 치환 규칙 위반 0. 프로젝트에 `Config/DefaultGameplayTags.ini` 자체가 없어 태그 소스가 이 파일 한 곳이며, WxCore 밖 Wx 모듈의 `UE_DECLARE/DEFINE_GAMEPLAY_TAG` 는 0건(`Plugins/PersistenceExamples` 는 서드파티 샘플로 규칙 대상 아님).
  - 지난 리뷰 지적 해소 — ini 에만 남아 있던 `ECC_GameTraceChannel2 = "WxInteractable"` 등록 줄이 제거돼 `Config/DefaultEngine.ini:39` 의 `WxAttack`(=`ECC_GameTraceChannel1`) 한 개만 남았고, 헤더 상수와 1:1 로 일치한다.
  - `IWxSavable` 계약 문서의 사실성 — "직렬화 대상은 액터와 그 컴포넌트 전체"(`WxSavable.h:12`)와 "무효 GUID 면 저장/복원 제외"(`:35`)는 실제 구현과 일치한다(`WxSaveWorldSubsystem.cpp:288-292`·`:351-355` 의 `IsValid` 게이트, `:313`·`:392` 의 컴포넌트 순회 직렬화, `:416` 의 `OnSaveRestored` 호출).
  - `FWxActorTarget` 크로스 모듈 사용 — 구조체에 `WXCORE_API` 가 없지만 UHT 가 `Z_Construct_UScriptStruct_FWxActorTarget` 를 `WXCORE_API` 로 내보내므로 타 모듈 UPROPERTY 사용(`WxQuestStateTreeNodes.h:131`, `WxDialogueStateTreeNodes.h:126`)은 링크 문제가 없다.
- **미검토 / 한계**: `Gimmick.*`(9개)·`Quest.Fail`·`Ability.Pattern.*`·`Ability.Skill.2/3/4`·`GameplayCue.AttackTelegraph.*` 는 C++ 참조가 0인데(전량 데이터 구동), `.claude/asset_dump/StateTrees/*.json`(5파일)이 ST 상태의 Tag 필드를 직렬화하지 않아 `Gimmick.*`·`Quest.Fail` 의 사용 여부를 텍스트로 확증하지 못했다 — 헤더 `:104-108` 의 설계 근거상 ST 상태 라벨로 쓰이는 것이 정상이므로 데드로 판정하지 않았다. 다만 `Ability.Pattern.8`·`Ability.Pattern.9` 는 코드·에셋 덤프 양쪽 모두 0건이라 예약 슬롯인지 잔재인지 확인이 필요하다(번호 연속 시리즈라 발견으로 올리지 않았다). 발견 1의 ensure 발화 시나리오는 정적 분석·주석 근거로만 확인했고 PIE 실측은 하지 않았다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 10파일 — `/module-review`로 갱신*
