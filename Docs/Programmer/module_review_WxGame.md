# WxGame — 코드 리뷰

> 조립 모듈로서의 경계가 잘 지켜져 있고, 프레임워크·게임플로우·MVVM 브리지 모두 함정 주석이 붙어 있을 만큼 의도가 명확하다. 이번 리뷰에서 새로 들어온 `UWxAbilitySlotSwitcher`·블랙보드 옵저버 경로는 수명 정리(`Shutdown`/`UnregisterObserversFrom`)까지 맞물려 있어 깨끗했고, 남은 지적은 지난 리뷰에서 이월된 뷰모델 해제 시점 문제들과 등록 주기에서 되돌리지 않는 부작용이다. 이번 리뷰는 `Source/WxGame` 65파일 전부를 읽고 Character·MVVM(특히 신규 `WxAbilitySlotSwitcher`)·FrontEnd·Framework·Inventory 의 핵심 cpp 를 깊게 봤으며, 판정에 필요한 계약은 WxCombat·WxUI·WxWorld·WxInventory·WxDialogue 쪽 코드로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 획득 토스트 뷰모델이 교체될 때 이전 인스턴스를 해제하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:114`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:117`
- **범주**: 버그/정확성
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 새 `UWxViewModel_InventoryItem` 을 만들어 `LastAcquiredItem` 에 덮어쓰는데, 밀려나는 이전 인스턴스에 `Deinitialize()` 를 부르지 않는다. 그 인스턴스는 `Initialize` 에서 인벤토리 델리게이트 4종(`OnInventoryStackChanged`·`OnInventorySlotChanged`·`OnInventoryChargeChanged`·`OnInventoryContentsChanged`)을 구독한 상태라(`WxViewModel_InventoryItem.cpp:56`~`:59`), 아무 위젯도 보지 않는데 인벤토리 변경마다 `RefreshFromSource` 를 계속 돈다. 그 안의 `RefreshChargeIcon` → `SetIcon` 은 `UWxViewModel::RequestImageAsync` 로 이어지므로 죽은 VM 하나마다 변경 1회당 스트리밍 요청까지 새로 건다. 참조가 끊긴 VM 은 결국 GC 가 수거하고 그때 `BeginDestroy` → `Deinitialize` 로 구독이 풀리므로 영구 누수는 아니지만, GC 주기 안에서는 획득 횟수에 비례해 죽은 구독자가 쌓여 인벤토리 변경 한 번의 비용이 선형으로 늘어난다. 드랍이 잦은 전투에서 가장 크게 나타난다. 같은 파일 `UnbindSource` 는 `LastAcquiredItem` 을 정확히 `Deinitialize()` 하고 있어(`:60`~`:62`) 해제 계약은 이미 인지된 것이며, 교체 경로에서만 빠졌다.
- **제안**: `UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, AcquisitionVM)` 직전에 기존 `LastAcquiredItem` 이 있으면 `Deinitialize()` 한다(`UnbindSource` 와 같은 처리).
- **확신도**: 높음

### 2. 🟡 위젯별로 만드는 뷰모델 셋에 `DestroyInstance` 가 없어 구독이 GC 까지 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:83`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:51`, `Source/WxGame/MVVM/WxViewModel_Quest.h:60`
- **범주**: 설계/구조
- **문제**: 같은 모듈의 `UWxViewModelResolver_Inventory`·`_Item`·`_BossCharacter` 는 `DestroyInstance` 에서 `Deinitialize()` 를 불러 뷰 해제 시점에 구독을 끊는다. 반면 위 세 리졸버는 `CreateInstance` 에서 매번 `NewObject` 로 새 VM 을 만들면서 해제 훅이 없어, 위젯이 사라진 뒤에도 VM 이 GC 로 수거될 때까지 소스 구독이 살아 있다. 셋 다 Outer 가 위젯이 아니라 오래 사는 객체(PC·`UWxDialogueSessionComponent`·`UWxQuestComponent`)라 그만큼 수명 구분이 흐리다. 특히 `UWxViewModel_InteractionList` 는 스캐너의 `OnListChanged` 마다 `RebuildEntries` 로 프롬프트 수만큼 `UWxViewModel_Interaction` 을 새로 할당하므로(`WxViewModel_InteractionList.cpp:124`), 죽은 위젯의 VM 이 상호작용 후보가 바뀔 때마다 계속 할당한다. `UWxViewModel::BeginDestroy` 가 결국 정리하므로 누수는 아니지만 해제 시점이 정의되지 않은 것은 맞다. `_Ability`·`_PlayerCharacter` 는 ASC 를 Outer 로 공유하는 VM 을 `FindSharedViewModel` 로 돌려주므로 해제 훅이 없는 것이 의도된 설계이며 이 지적에서 제외한다.
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
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. 모듈의 다른 모든 바인딩(`HandleRagdollTagChanged`·`HandleInventoryReady`·`HandleTargetActorChanged`·`HandlePostLoadMap` …)은 이를 지키는데, `BindAction` 에 물린 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 와 `FTickerDelegate::CreateUObject` 에 물린 `FlushRefresh` 만 예외다. 엔진 이름에 맞춰야 하는 `Jump`·`StopJumping` 은 제외한다.
- **제안**: 여섯 콜백의 선언·정의·바인딩을 `Handle` 접두사로 통일한다. 같은 파일의 `HandleTagChanged`·`HandleAbilitySpecDirtied` 와 나란히 놓이므로 `FlushRefresh` 도 함께 맞추는 편이 읽기에 낫다.
- **확신도**: 높음

### 5. 🟢 기준 이동속도 계산이 두 곳에 그대로 복제돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:208`, `Source/WxGame/Character/WxCharacterBase.cpp:221`
- **범주**: 중복/복잡도
- **문제**: `GetDefault<AWxCharacterBase>(GetClass())->GetCharacterMovement()->MaxWalkSpeed` 를 읽어 SPD 를 곱하는 두 줄이 초기 1회 적용(`InitAbilitySystem`)과 변경 콜백(`HandleSPDAttributeChanged`)에 그대로 두 번 있다. "기준값은 CDO 에서만 읽는다"는 이 계산의 함정이 두 곳에 걸쳐 있어, 한쪽만 고치면 이미 SPD 가 곱해진 인스턴스 값을 기준으로 삼는 버그가 조용히 들어온다.
- **제안**: `ApplyWalkSpeedFromSPD(float NewSPD)` 같은 private 헬퍼 하나로 모으고 두 지점이 그것만 호출하게 한다.
- **확신도**: 높음

### 6. 🟢 소환물 상태 태그의 반납 게이트가 반납을 실제로 일으키는 신호와 다르다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:79`, `Source/WxGame/Character/WxEnemyCharacter.cpp:177`
- **범주**: 버그/정확성
- **문제**: `MasterStateTag` 는 `BeginPlay:58` 에서 주인 ASC 에 1 올리고, `HandleOwnerDeath:177`(사망 통지 = `Ability.Death` 태그 부여)에서 반납한다. 그런데 `EndPlay:79` 의 이중 반납 방지 게이트는 `IsAlive()` — 즉 HP 잔량이다. 반납을 일으키는 신호(사망 태그)와 반납 여부를 판단하는 신호(HP)가 달라, HP 가 0 이 되었지만 사망 어빌리티가 활성되지 않은 상태로 액터가 파괴되면(AbilitySet 에 `UWxAbility_Death` 가 빠진 BP, `Event.Death` 가 막힌 구성) 두 경로 모두 반납을 건너뛰어 주인의 태그 카운트가 영구히 1 남는다. `MaxCountPerMaster` 자체는 `UWxMinionSubsystem::Rosters` 로 세므로 소환 상한은 영향받지 않지만, `State.Minion.*` 을 조건으로 삼는 어빌리티는 소환물이 없는데도 계속 있다고 본다. 정상 경로에서는 사망 어빌리티가 스스로 종료하지 않아(`Plugins/WxCombat/.../WxAbility_Death.cpp:58`) 태그가 액터 수명 끝까지 유지되므로 이중 반납은 발생하지 않는다.
- **제안**: `EndPlay` 게이트를 `IsAlive()` 대신 `GetAbilitySystemComponent()->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)` 의 부정으로 바꿔, 반납 트리거와 같은 신호를 보게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 발동 조건이 BP 오구성에 한정된다)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/MVVM/WxAbilitySlotSwitcher.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`.
- **훑은 파일**: 나머지 전부(모듈 65파일을 모두 읽었다) — `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h/.cpp`, `Source/WxGame/Framework/WxGameMode.h/.cpp`, `Source/WxGame/Framework/WxGameState.h/.cpp`, `Source/WxGame/Controller/WxPlayerController.h/.cpp`, `Source/WxGame/Player/WxPlayerState.h/.cpp`, `Source/WxGame/Character/WxNpc.h/.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Input/WxInputConfig.h/.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.h/.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.h/.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h/.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.h/.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h/.cpp` 및 각 헤더.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE 재현은 하지 않았다. 아래는 후보로 파고들었으나 코드 근거로 기각·보류한 것들이라, 다음 세션이 같은 길을 다시 파지 않도록 남긴다.
  - `UWxAbilitySlotSwitcher::FindSourceName`(`:206`)이 `CurrentViewModel` 이 살아 있을 때만 슬롯을 찾는다는 점은 실패 시나리오를 만들지 못했다. 리졸버가 다시 불리지 않는 경우(`bSetManually`)라면 뷰가 그 예전 VM 을 강참조로 붙들고 있어 탐색이 성립하고, 뷰가 붙들지 않는 경우라면 리졸버가 다시 불려 `WatchSlot` 이 상태를 새로 세운다. 또 `PickAbility` 가 돌려주는 것은 CDO 라 폰이 바뀌어도 같은 포인터지만, `Construct`(`:89`~`:94`)가 ASC 변경을 보고 `CurrentAbility` 를 비우므로 조기 반환에 걸리지 않는다.
  - `AWxEnemyCharacter::OnSpawnedBy`(`:112`)가 캐릭터를 스포너에 `AttachToActor` 하는 것은 이동 복제를 `AttachmentReplication` 경로로 옮겨 시뮬 프록시가 `ACharacter` 의 이동 보간을 못 받게 만드는 구조지만, 프로젝트가 사실상 단일 플레이(서버가 곧 클라)라 실측 없이 결함으로 판정하지 않았다. 스포너 역참조는 `OwningSpawner` 로도 이미 있으니, 순회 링크만 필요하다면 부착을 걷을 여지는 있다.
  - `UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement`(`:60`)가 `ASC->GetAnimatingAbility()` 로 `bWantsToCrouch` 를 끄는 것은 클라/서버 몽타주 상태가 어긋나면 이동 예측 불일치를 낼 수 있는 구조지만, 같은 이유로 실측 없이 판정하지 않았다.
  - `UWxGameFlowSubsystem::RequestNewGame:52` 의 `GetSubsystem<UWxCheckpointSubsystem>()` 무검사 역참조는 안전 — 그 서브시스템이 `ShouldCreateSubsystem` 을 오버라이드하지 않아 항상 생성된다(`Plugins/WxWorld/.../WxCheckpointSubsystem.h`).
  - `AWxNpc::CanInteract` 가 `Super` 를 부르지 않는 것은 정상 — `AWxDialogueActor` 에 해당 오버라이드가 아예 없다. `AWxNpc` 의 메시가 모든 응답을 `Ignore` 로 두고도 감지·사거리 판정이 성립하는 것도 확인했다(스캐너는 `OverlapMultiByObjectType`, 어빌리티는 `OverlapComponent` — 둘 다 응답 매트릭스를 보지 않는다).
  - `AWxCharacterBase::InitAbilitySystem` 이 초기 1회 적용에서 `CombatAttributeSet->GetSPD()` 를 읽는 것은 안전 — `UWxCombatAttributeSet` 생성자가 `InitSPD(1.f)` 하므로 복제 전에도 배율이 1 이다.
  - `AWxEnemyCharacter` 의 사망 처리는 이중 진입이 없다 — `Ability.Death` 는 사망 어빌리티가 액터 수명 끝까지 들고 있어 `HandleDeathTagChanged` 가 한 번만 양수 전이를 보고, `BeginPlay:66` 의 보정 호출은 구독보다 먼저 온 사망만 잡는다.
  - 모듈 전체에서 `FORCEINLINE`·헤더 인라인 정의·람다·Copyright 첫 줄 누락은 0건이고, `BlueprintCallable` 7건은 Blueprint Function Library(2)/`BlueprintSetter`(1)/승인된 뷰모델 Command 예외(4, `.claude/worklog/2026-07-23-상호작용-입력-WBP-VM커맨드-이관.md`)에 해당한다.
  - BP/WBP 내부 구조와 데이터 에셋 내용은 범위 밖이다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 65파일 — `/module-review`로 갱신*
