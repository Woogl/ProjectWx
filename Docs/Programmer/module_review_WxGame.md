# WxGame — 코드 리뷰

> 조립 모듈로서의 경계가 잘 지켜져 있고, 프레임워크·게임플로우·MVVM 브리지 어디든 "왜 이렇게 했는지"가 주석으로 붙어 있어 읽기 쉽다. 남은 지적은 뷰모델 해제 시점과 등록 주기에서 되돌리지 않는 부작용에 몰려 있고, 새로 잡힌 것은 처형 프롬프트가 형제 구현들과 달리 코드 하드코딩이라는 점 하나다. 이번 리뷰는 `Source/WxGame` 65파일을 전부 읽고 MVVM·Character·FrontEnd·Framework·Inventory 의 핵심 cpp 를 깊게 봤으며, 판정에 필요한 계약(사망 어빌리티 수명, 스캐너 호출 주기, 뷰모델 베이스의 `Deinitialize` 의미, 형제 `GetInteractionPrompt` 구현)은 WxCombat·WxWorld·WxUI·WxDialogue·WxInventory 쪽 코드로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 획득 토스트 뷰모델이 교체될 때 이전 인스턴스를 해제하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:114`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:117`
- **범주**: 버그/정확성
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 새 `UWxViewModel_InventoryItem` 을 만들어 `LastAcquiredItem` 에 덮어쓰는데, 밀려나는 이전 인스턴스에 `Deinitialize()` 를 부르지 않는다. 그 인스턴스는 `Initialize` 에서 인벤토리 델리게이트 4종(`OnInventoryStackChanged`·`OnInventorySlotChanged`·`OnInventoryChargeChanged`·`OnInventoryContentsChanged`)을 구독한 상태라(`Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:56`~`:59`), 아무 위젯도 보지 않는데 인벤토리 변경마다 `RefreshFromSource` 를 계속 돈다. 그 안의 `RefreshChargeIcon` → `SetIcon` 은 `UWxViewModel::RequestImageAsync` 로 이어지므로 죽은 VM 하나마다 변경 1회당 스트리밍 요청까지 새로 건다. 참조가 끊긴 VM 은 결국 GC 가 수거하고 그때 `BeginDestroy` → `Deinitialize` 로 구독이 풀리므로 영구 누수는 아니지만, GC 주기 안에서는 획득 횟수에 비례해 죽은 구독자가 쌓여 인벤토리 변경 한 번의 비용이 선형으로 늘어난다. 드랍이 잦은 전투에서 가장 크게 나타난다. 같은 파일 `UnbindSource` 는 `LastAcquiredItem` 을 정확히 `Deinitialize()` 하고 있어(`:60`~`:62`) 해제 계약은 이미 인지된 것이며, 교체 경로에서만 빠졌다.
- **제안**: `UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, AcquisitionVM)` 직전에 기존 `LastAcquiredItem` 이 있으면 `Deinitialize()` 한다(`UnbindSource` 와 같은 처리).
- **확신도**: 높음

### 2. 🟡 위젯별로 만드는 뷰모델 셋에 `DestroyInstance` 가 없어 구독이 GC 까지 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:83`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:51`, `Source/WxGame/MVVM/WxViewModel_Quest.h:60`
- **범주**: 설계/구조
- **문제**: 같은 모듈의 `UWxViewModelResolver_Inventory`(`WxViewModel_Inventory.cpp:232`)·`_Item`(`WxViewModel_InventoryItem.cpp:257`)·`_BossCharacter`(`WxViewModelResolver_BossCharacter.cpp:21`)는 `DestroyInstance` 에서 `Deinitialize()` 를 불러 뷰 해제 시점에 구독을 끊는다. 반면 위 세 리졸버는 `CreateInstance` 에서 매번 `NewObject` 로 새 VM 을 만들면서 해제 훅이 없어, 위젯이 사라진 뒤에도 VM 이 GC 로 수거될 때까지 소스 구독이 살아 있다. 셋 다 Outer 가 위젯이 아니라 오래 사는 객체(PC·`UWxDialogueSessionComponent`·`UWxQuestComponent`)라 그만큼 수명 구분이 흐리다. 특히 `UWxViewModel_InteractionList` 는 스캐너의 `OnListChanged` 마다 `RebuildEntries` 로 프롬프트 수만큼 `UWxViewModel_Interaction` 을 새로 할당하므로(`WxViewModel_InteractionList.cpp:124`), 죽은 위젯의 VM 이 상호작용 후보가 바뀔 때마다 계속 할당한다. `UWxViewModel::BeginDestroy` 가 결국 정리하므로 누수는 아니지만 해제 시점이 정의되지 않은 것은 맞다. `_Ability`·`_PlayerCharacter` 는 ASC 를 Outer 로 공유하는 VM 을 `FindSharedViewModel` 로 돌려주므로 해제 훅이 없는 것이 의도된 설계이며 이 지적에서 제외한다.
- **제안**: 세 리졸버에 `DestroyInstance` 를 추가해 자기가 만든 인스턴스만 `Deinitialize()` 한다. 겸사겸사 (a) `UWxViewModel_InteractionList::Deinitialize()` 가 `StopObserving()` 을 부르지 않아 `OnAnyScannerReady` 구독이 `BeginDestroy` 까지 남는 점(`WxViewModel_InteractionList.cpp:47`), (b) 이 셋만 `ExpectedType` 검증 없이 베이스 클래스를 `NewObject` 하는 점(`_Inventory`·`_Item`·`_BossCharacter` 는 `IsChildOf`·`CLASS_Abstract` 를 확인한다)도 같이 맞춘다.
- **확신도**: 높음

### 3. 🟡 MetaHuman 부착물을 걷어내도 리더 메시의 표시·틱 옵션이 원래대로 돌아가지 않는다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:54`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더를 숨기고(`:54`) `VisibilityBasedAnimTickOption` 을 `AlwaysTickPoseAndRefreshBones` 로 올리는데(`:56`), `OnUnregister` 는 표시만 `SetVisibility(true)` 로 되돌리고(`:131`) 틱 옵션은 그대로 둔다. 리더가 살아남는 해제 경로(BP 리컴파일, MetaHuman 컴포넌트만 제거·재등록, 레벨 스트리밍 아웃)에서는 부착물이 사라진 뒤에도 화면 밖 본 리프레시 비용이 계속 든다. 표시 복원도 "원래 보이는 상태였다"를 가정해 무조건 `true` 로 되돌리므로, BP 에서 리더를 숨겨 둔 캐릭터라면 해제 후 원치 않게 드러난다. 월드가 통째로 파괴되는 경우에는 둘 다 영향이 없다.
- **제안**: 리더를 건드리기 전에 원래 표시 상태와 틱 옵션을 멤버에 저장하고, 그 변경을 적용한 등록 주기의 `OnUnregister` 에서 둘 다 복원한다.
- **확신도**: 높음

### 4. 🟢 델리게이트 콜백 여섯 개가 `Handle` 접두사 규칙을 따르지 않는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:53`~`:58` (바인딩은 `Source/WxGame/Character/WxPlayerCharacter.cpp:110`, `:114`, `:123`, `:128`, `:129`), `Source/WxGame/MVVM/WxAbilitySlotSwitcher.h:68` (바인딩은 `Source/WxGame/MVVM/WxAbilitySlotSwitcher.cpp:148`)
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. 모듈의 나머지 바인딩 34곳은 전부 이를 지키는데(`HandleRagdollTagChanged`·`HandleInventoryReady`·`HandleTargetActorChanged`·`HandlePostLoadMap` …), `BindAction` 에 물린 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 와 `FTickerDelegate::CreateUObject` 에 물린 `FlushRefresh` 만 예외다. 엔진 이름에 맞춰야 하는 `Jump`(`ACharacter::Jump` 오버라이드)·`ACharacter::StopJumping` 은 제외한다.
- **제안**: 여섯 콜백의 선언·정의·바인딩을 `Handle` 접두사로 통일한다. `FlushRefresh` 는 같은 파일의 `HandleTagChanged`·`HandleAbilitySpecDirtied` 와 나란히 놓이므로 함께 맞추는 편이 읽기에 낫다.
- **확신도**: 높음

### 5. 🟢 기준 이동속도 계산이 두 곳에 그대로 복제돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:208`, `Source/WxGame/Character/WxCharacterBase.cpp:221`
- **범주**: 중복/복잡도
- **문제**: `GetDefault<AWxCharacterBase>(GetClass())->GetCharacterMovement()->MaxWalkSpeed` 를 읽어 SPD 를 곱하는 두 줄이 초기 1회 적용(`InitAbilitySystem`)과 변경 콜백(`HandleSPDAttributeChanged`)에 그대로 두 번 있다. "기준값은 CDO 에서만 읽는다"는 이 계산의 함정이 두 곳에 걸쳐 있어(그 이유를 적은 주석은 `:220` 한쪽에만 있다), 한쪽만 고치면 이미 SPD 가 곱해진 인스턴스 값을 기준으로 삼는 버그가 조용히 들어온다.
- **제안**: `ApplyWalkSpeedFromSPD(float NewSPD)` 같은 private 헬퍼 하나로 모으고 두 지점이 그것만 호출하게 한다.
- **확신도**: 높음

### 6. 🟢 처형 프롬프트만 코드에 박힌 비로컬라이즈 문자열이다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:154`
- **범주**: 설계/구조
- **문제**: `AWxEnemyCharacter::GetInteractionPrompt` 가 `FText::FromString(TEXT("Finisher"))` 를 돌려준다. 같은 `IWxInteractable` 계약의 형제 구현은 전부 데이터 주도이거나 로컬라이즈 대상이다 — `AWxDevice` 는 저작 필드(`InteractionBinding.Prompt`)를, `AWxDialogueActor` 는 대화 컴포넌트의 값을, `AWxItemPickup` 은 `NSLOCTEXT` 포맷을 쓴다. `FText::FromString` 은 culture-invariant 라 번역 파이프라인에 잡히지 않고, 기획자가 문구를 바꾸려면 C++ 을 고쳐야 한다. 부수적으로 이 호출은 스캐너의 `GetPrompts()` 를 통해 후보가 남아 있는 동안 `ScanInterval`(기본 0.1초)마다 반복되며(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:80`, `:232`), 매번 새 `FText` 를 만들고 문자열 비교로만 걸러진다.
- **제안**: `AWxDevice` 처럼 `EditDefaultsOnly FText` 로 노출하거나, 최소한 `NSLOCTEXT` 로 바꿔 번역 대상으로 만든다.
- **확신도**: 높음

### 7. 🟢 소환물 상태 태그의 반납 게이트가 반납을 실제로 일으키는 신호와 다르다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:79`, `Source/WxGame/Character/WxEnemyCharacter.cpp:177`
- **범주**: 버그/정확성
- **문제**: `MasterStateTag` 는 `BeginPlay:58` 에서 주인 ASC 에 1 올리고, `HandleOwnerDeath:177`(사망 통지 = `Ability.Death` 태그 부여)에서 반납한다. 그런데 `EndPlay:79` 의 이중 반납 방지 게이트는 `IsAlive()` — 즉 HP 잔량이다. 반납을 일으키는 신호(사망 태그)와 반납 여부를 판단하는 신호(HP)가 달라, HP 가 0 이 되었지만 사망 어빌리티가 활성되지 않은 상태로 액터가 파괴되면(AbilitySet 에 `UWxAbility_Death` 가 빠진 BP, `Event.Death` 가 막힌 구성) 두 경로 모두 반납을 건너뛰어 주인의 태그 카운트가 영구히 1 남는다. `MaxCountPerMaster` 자체는 `UWxMinionSubsystem` 의 명부로 세므로 소환 상한은 영향받지 않지만, `State.Minion.*` 을 조건으로 삼는 어빌리티는 소환물이 없는데도 계속 있다고 본다. 정상 경로에서는 사망 어빌리티가 스스로 종료하지 않아(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:58`) 태그가 액터 수명 끝까지 유지되므로 이중 반납은 발생하지 않는다.
- **제안**: `EndPlay` 게이트를 `IsAlive()` 대신 `GetAbilitySystemComponent()->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)` 의 부정으로 바꿔, 반납 트리거와 같은 신호를 보게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 발동 조건이 BP 오구성에 한정된다)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/MVVM/WxAbilitySlotSwitcher.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`.
- **훑은 파일**: 나머지 전부(모듈 65파일을 모두 읽었다) — `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h/.cpp`, `Source/WxGame/Framework/WxGameMode.h/.cpp`, `Source/WxGame/Framework/WxGameState.h/.cpp`, `Source/WxGame/Controller/WxPlayerController.h/.cpp`, `Source/WxGame/Player/WxPlayerState.h/.cpp`, `Source/WxGame/Character/WxNpc.h/.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Input/WxInputConfig.h/.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.h/.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.h/.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h/.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.h/.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h/.cpp` 및 각 헤더.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE 재현은 하지 않았다. 아래는 후보로 파고들었으나 코드 근거로 기각·보류한 것들이라, 다음 세션이 같은 길을 다시 파지 않도록 남긴다.
  - 모듈 전체에서 `FORCEINLINE`·헤더 인라인 정의·람다·Copyright 첫 줄 누락은 0건이다. `BlueprintCallable` 7건은 Blueprint Function Library(`WxRespawnLibrary`·`WxFrontEndLibrary`)/`BlueprintSetter`(`SetCurrentCategory`)/승인된 뷰모델 Command 예외(4건, `.claude/worklog/2026-07-23-상호작용-입력-WBP-VM커맨드-이관.md`)에 해당한다. 오버라이드 40종을 훑어 `Super::` 누락도 찾지 못했다 — `AWxCharacterBase::CanJumpInternal_Implementation:117` 의 조기 반환은 주석으로 사유가 달린 의도된 분기이고, `AWxNpc::CanInteract` 는 `AWxDialogueActor` 에 해당 오버라이드가 없어 `IWxInteractable` 순수 가상의 첫 구현이다.
  - `AWxCharacterBase` 의 사망 처리에는 이중 진입이 없다 — `UWxAbility_Death` 가 스스로 `EndAbility` 하지 않아 `Ability.Death` 가 액터 수명 끝까지 유지되고, `HandleDeathTagChanged` 는 양수 전이를 한 번만 본다. `PostInitializeComponents:73` 의 선(先)사망 보정은 구독자가 아직 없어 무동작이고, 그 자리를 `AWxEnemyCharacter::BeginPlay:66` 이 메운다. `State.Ragdoll` 도 제거하는 코드가 저장소 전체에 없어 진입 전용 처리가 일관된다.
  - `UWxAbilitySlotSwitcher` 의 수명 정리는 맞물려 있다 — `Shutdown` 이 ASC 구독과 티커를 함께 걷고 `Destruct`·`BeginDestroy` 양쪽에서 불린다. `FlushRefresh` 는 항상 `false` 를 돌려줘 티커가 1회성이며, 핸들도 진입 즉시 비운다. `Construct:89` 가 ASC 변경을 보고 `CurrentAbility` 를 비우므로 폰 교체 후에도 `PickAbility` 의 CDO 포인터 비교가 조기 반환에 걸리지 않는다.
  - `UWxGameFlowSubsystem::RequestNewGame:52` 의 `GetSubsystem<UWxCheckpointSubsystem>()` 무검사 역참조는 안전 — 그 서브시스템이 `ShouldCreateSubsystem` 을 오버라이드하지 않아 항상 생성된다.
  - `AWxCharacterBase::InitAbilitySystem` 이 초기 1회 적용에서 `CombatAttributeSet->GetSPD()` 를 읽는 것은 안전 — `UWxCombatAttributeSet` 생성자가 `InitSPD(1.f)` 하므로 복제 전에도 배율이 1 이다. 재빙의로 여러 번 불려도 기준값을 CDO 에서 읽으므로 배율이 누적되지 않는다.
  - `AWxEnemyCharacter::OnSpawnedBy:112` 가 캐릭터를 스포너에 `AttachToActor` 하는 것은 이동 복제를 `AttachmentReplication` 경로로 옮기는 구조지만, 프로젝트가 사실상 단일 플레이(서버가 곧 클라)라 실측 없이 결함으로 판정하지 않았다. 스포너 역참조는 `OwningSpawner` 로도 이미 있으니 순회 링크만 필요하다면 부착을 걷을 여지는 있다.
  - `UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement:65` 가 `ASC->GetAnimatingAbility()` 로 `bWantsToCrouch` 를 끄는 것은 클라/서버 몽타주 상태가 어긋나면 이동 예측 불일치를 낼 수 있는 구조지만, 같은 이유로 실측 없이 판정하지 않았다.
  - `AWxCharacterBase` 가 `WeaponActor` 만 널 검사하고(`:76`, `:144`) 다른 생성자 서브오브젝트는 무검사로 역참조하는 비대칭은, 클래스 주석(`WxCharacterBase.h:28`)이 말하는 "초기화 경계 확인"과 어긋나 보이나 실제로 널이 될 경로를 찾지 못해 지적하지 않았다.
  - BP/WBP 내부 구조와 데이터 에셋 내용은 범위 밖이다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `4a8e5d4b` · 리뷰일 2026-09-11 · 소스 65파일 — `/module-review`로 갱신*
