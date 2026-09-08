# WxGame — 코드 리뷰

> 조립 모듈로서의 경계가 잘 지켜져 있고, 프레임워크 클래스·게임플로우·MVVM 브릿지 모두 함정 주석이 붙어 있을 만큼 의도가 명확하다. 남은 과제는 뷰모델 해제 시점이 정의되지 않은 두 곳과, 등록 주기에서 되돌리지 않는 부작용 하나뿐이다. 이번 리뷰는 `Source/WxGame` 66파일 전부를 훑고 Character·MVVM·FrontEnd·Framework·Inventory 의 핵심 cpp 를 깊게 봤으며, 판정에 필요한 계약은 WxUI·WxInventory·WxCombat·WxWorld·WxDialogue 쪽 코드로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 획득 토스트 뷰모델이 교체될 때 이전 인스턴스를 해제하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:114`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:117`
- **범주**: 버그/정확성
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 새 `UWxViewModel_InventoryItem` 을 만들어 `LastAcquiredItem` 에 덮어쓰는데, 밀려나는 이전 인스턴스에 `Deinitialize()` 를 부르지 않는다. 그 인스턴스는 `Initialize` 에서 인벤토리 델리게이트 4종(`OnInventoryStackChanged`·`OnInventorySlotChanged`·`OnInventoryChargeChanged`·`OnInventoryContentsChanged`)을 구독한 상태라(`WxViewModel_InventoryItem.cpp:56`~`:59`), 아무 위젯도 보지 않는데 인벤토리 변경마다 `RefreshFromSource` 를 계속 돈다. 그 안의 `RefreshChargeIcon` → `SetIcon` 은 `UWxViewModel_Item::SetIcon` 을 타고 `RequestImageAsync` 로 이어지므로, 죽은 VM 하나마다 변경 1회당 스트리밍 요청까지 새로 건다. 획득 횟수에 비례해 죽은 구독자가 쌓이므로 드랍이 잦은 전투일수록 인벤토리 변경 한 번의 비용이 선형으로 늘어난다. 같은 파일 `UnbindSource` 는 `LastAcquiredItem` 을 정확히 `Deinitialize()` 하고 있어(`:60`~`:63`) 해제 계약은 이미 인지된 것이며, 교체 경로에서만 빠졌다.
- **제안**: `UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, AcquisitionVM)` 직전에 기존 `LastAcquiredItem` 이 있으면 `Deinitialize()` 한다(`UnbindSource` 와 같은 처리).
- **확신도**: 높음

### 2. 🟡 위젯별로 만드는 뷰모델 셋에 `DestroyInstance` 가 없어 구독이 GC 까지 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:83`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:51`, `Source/WxGame/MVVM/WxViewModel_Quest.h:60`
- **범주**: 설계/구조
- **문제**: 같은 모듈의 `UWxViewModelResolver_Inventory`·`_Item`·`_BossCharacter` 는 `DestroyInstance` 에서 `Deinitialize()` 를 불러 뷰 해제 시점에 구독을 끊는다. 반면 위 세 리졸버는 `CreateInstance` 에서 매번 `NewObject` 로 새 VM 을 만들면서 해제 훅이 없어, 위젯이 사라진 뒤에도 VM 이 GC 로 수거될 때까지 소스 구독이 살아 있다. 셋 다 Outer 가 위젯이 아니라 오래 사는 객체(PC·`UWxDialogueSessionComponent`·`UWxQuestComponent`)라 수거가 늦다. 특히 `UWxViewModel_InteractionList` 는 스캐너의 `OnListChanged` 마다 `RebuildEntries` 로 프롬프트 수만큼 `UWxViewModel_Interaction` 을 새로 할당하므로(`WxViewModel_InteractionList.cpp:124`), 죽은 위젯의 VM 이 상호작용 후보가 바뀔 때마다 계속 할당한다. `UWxViewModel::BeginDestroy` 가 결국 정리하므로 누수는 아니지만 해제 시점이 정의되지 않은 것은 맞다. `_Ability`·`_PlayerCharacter` 는 ASC 를 Outer 로 공유하는 VM 이라 해제 훅이 없는 것이 의도된 설계이며 이 지적에서 제외한다.
- **제안**: 세 리졸버에 `DestroyInstance` 를 추가해 자기가 만든 인스턴스만 `Deinitialize()` 한다. 겸사겸사 (a) `UWxViewModel_InteractionList::Deinitialize()` 가 `StopObserving()` 을 부르지 않아 `OnAnyScannerReady` 구독이 `BeginDestroy` 까지 남는 점(`WxViewModel_InteractionList.cpp:47`), (b) 이 셋만 `ExpectedType` 검증 없이 베이스 클래스를 `NewObject` 하는 점(`_Inventory`·`_Item`·`_BossCharacter` 는 `IsChildOf`·`CLASS_Abstract` 를 확인한다)도 같이 맞춘다.
- **확신도**: 높음

### 3. 🟡 MetaHuman 부착물을 걷어내도 리더 메시의 표시·틱 옵션이 원래대로 돌아가지 않는다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:54`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더를 숨기고(`:54`) `VisibilityBasedAnimTickOption` 을 `AlwaysTickPoseAndRefreshBones` 로 올리는데(`:56`), `OnUnregister` 는 표시만 `SetVisibility(true)` 로 되돌리고(`:131`) 틱 옵션은 그대로 둔다. 리더가 살아남는 해제 경로(BP 리컴파일, MetaHuman 컴포넌트만 제거·재등록, 레벨 스트리밍 아웃)에서는 부착물이 사라진 뒤에도 화면 밖 본 리프레시 비용이 계속 든다. 표시 복원도 "원래 보이는 상태였다"를 가정해 무조건 `true` 로 되돌리므로, BP 에서 리더를 숨겨 둔 캐릭터라면 해제 후 원치 않게 드러난다. 월드가 통째로 파괴되는 경우에는 둘 다 영향이 없다.
- **제안**: 리더를 건드리기 전에 원래 표시 상태와 틱 옵션을 멤버에 저장하고, 그 변경을 적용한 등록 주기의 `OnUnregister` 에서 둘 다 복원한다.
- **확신도**: 높음

### 4. 🟢 입력 액션 콜백 다섯 개가 `Handle` 접두사 규칙을 따르지 않는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:54`~`:59` (바인딩은 `Source/WxGame/Character/WxPlayerCharacter.cpp:113`, `:117`, `:126`, `:131`, `:132`)
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. 모듈의 다른 모든 바인딩(`HandleRagdollTagChanged`·`HandleInventoryReady`·`HandleSPDAttributeChanged`·`HandlePostLoadMap` …)은 이를 지키는데, `BindAction` 에 물린 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 만 예외다. 엔진 이름에 맞춰야 하는 `Jump`·`StopJumping` 은 제외한다.
- **제안**: 다섯 콜백의 선언·정의·바인딩을 `Handle` 접두사로 통일한다.
- **확신도**: 높음

### 5. 🟢 기준 이동속도 계산이 두 곳에 그대로 복제돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:211`, `Source/WxGame/Character/WxCharacterBase.cpp:224`
- **범주**: 중복/복잡도
- **문제**: `GetDefault<AWxCharacterBase>(GetClass())->GetCharacterMovement()->MaxWalkSpeed` 를 읽어 SPD 를 곱하는 두 줄이 초기 1회 적용(`InitAbilitySystem`)과 변경 콜백(`HandleSPDAttributeChanged`)에 그대로 두 번 있다. "기준값은 CDO 에서만 읽는다"는 이 계산의 함정이 두 곳에 걸쳐 있어, 한쪽만 고치면 이미 SPD 가 곱해진 인스턴스 값을 기준으로 삼는 버그가 조용히 들어온다.
- **제안**: `ApplyWalkSpeedFromSPD(float NewSPD)` 같은 private 헬퍼 하나로 모으고 두 지점이 그것만 호출하게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`.
- **훑은 파일**: 나머지 전부 — `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h/.cpp`, `Source/WxGame/Framework/WxGameState.h/.cpp`, `Source/WxGame/Controller/WxPlayerController.h/.cpp`, `Source/WxGame/Player/WxPlayerState.h/.cpp`, `Source/WxGame/Character/WxNpc.h/.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Character/Component/WxAIBehaviorComponent.h/.cpp`, `Source/WxGame/Cheat/WxCheatManager.h/.cpp`, `Source/WxGame/Input/WxInputConfig.h/.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.h/.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.h/.cpp`, `Source/WxGame/FrontEnd/Tests/WxFrontEndTests.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_*.h/.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h/.cpp`.
- **후보였으나 기각**: `AWxPlayerCharacter::SetupPlayerInputComponent` 가 어빌리티 입력을 `AbilitySystemComponent->GetAbilityInputActions()` 로 도는 것은 클라 스펙 복제 타이밍과 무관하다 — 그 함수는 granted spec 이 아니라 `AbilitySet` 에셋에서 IA 를 읽는다(`Plugins/WxCombat/.../WxAbilitySystemComponent.cpp:229`). `WxGameFlowSubsystem.cpp:52` 의 `GetSubsystem<UWxCheckpointSubsystem>()` 무검사 역참조는 그 서브시스템이 `ShouldCreateSubsystem` 을 오버라이드하지 않아 안전. `AWxNpc::CanInteract` 가 `Super` 를 부르지 않는 것은 `AWxDialogueActor` 에 해당 오버라이드가 아예 없어(인터페이스 기본 구현) 정상. `AWxEnemyCharacter` 의 `MasterStateTag` 는 사망 경로와 `EndPlay` 의 `IsAlive()` 게이트로 정확히 한 번만 반납되며 이중 반납 경로는 없다. `UWxViewModel_BossDisplay::RefreshDisplayedBoss:86` 의 `Character->` 역참조는 `ObservedWorld` 가 유효할 때만 도달하고 그 시점엔 `Character` 가 항상 생성돼 있다. `AWxCharacterBase::EnterRagdoll` 의 `DisableMovement` 는 `InitializeComponents` 이후인 `PostInitializeComponents` 에서 불리므로 CMC 기본 이동 모드에 덮이지 않는다. 모듈 전체에서 `FORCEINLINE`·헤더 인라인 정의·람다·Copyright 첫 줄 누락은 0건이고, `BlueprintCallable` 7건은 전부 Blueprint Function Library(2)/`BlueprintSetter`(1)/승인된 뷰모델 Command 예외(4, `.claude/worklog/2026-07-23-상호작용-입력-WBP-VM커맨드-이관.md`)에 해당한다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·자동화 테스트(`Wx.FrontEnd.InvalidContext`)를 실행하지 않았다. `UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement` 가 `ASC->GetAnimatingAbility()` 로 `bWantsToCrouch` 를 끄는 것은 클라/서버 몽타주 상태가 어긋나면 이동 예측 불일치를 낼 수 있는 구조지만, 프로젝트가 "서버가 곧 클라" 전제라 실측 없이 판정하지 않았다. `AWxPlayerCharacter::OnJumped_Implementation` 이 예측 키 없이 `JumpInvincibleEffect` 를 거는 경로도 리슨 서버 실측 없이는 확정할 수 없어 제외했다. 리스폰 흐름(`RequestRespawn`)의 위젯·뷰모델 재생성 순서는 코드로만 따라갔고 PIE 재현은 하지 않았다. BP/WBP 내부 구조와 데이터 에셋 내용은 범위 밖이다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `262e4cca` · 리뷰일 2026-09-08 · 소스 66파일 — `/module-review`로 갱신*
