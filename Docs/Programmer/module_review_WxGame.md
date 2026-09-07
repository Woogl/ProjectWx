# WxGame — 코드 리뷰

> Experience/GameFeature 제거(09-07) 이후 조립 모듈이 크게 가벼워졌고, 프레임워크 클래스·게임플로우·MVVM 브릿지 모두 함정 주석까지 붙어 있을 만큼 의도가 명확하다. 남은 과제는 뷰모델 해제 시점이 정의되지 않은 몇 곳과, 등록 주기에서 되돌리지 않는 부작용 하나다. 이번 리뷰는 `Source/WxGame` 전 영역(66파일)을 훑고 Character·MVVM·FrontEnd·Framework·Inventory 의 핵심 cpp 를 깊게 봤으며, 필요한 경우 WxUI·WxInventory·WxCombat·WxWorld 쪽 계약을 교차 확인했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

이전 리뷰(09-06)의 9건 중 Experience 파이프라인 관련 3건(액션 해제 비대칭·주입 실패 흡수·HUD 정책 혼입)은 해당 코드가 통째로 삭제되어 사라졌고, GameMode 의 FrontEnd 결합은 폰 클래스 조회 한 줄로 축소되어 의도된 설계로 확정되었으므로 제외했다. 뷰모델 Command 함수의 `BlueprintCallable` 지적도 승인된 예외(2026-07-23)라 철회한다. 나머지 5건은 아직 유효하며, 새로 1건(아래 1번)을 더했다.

## 결과

### 1. 🟡 획득 토스트 뷰모델이 교체될 때 이전 인스턴스를 해제하지 않는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:114`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:117`
- **범주**: 버그/정확성
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 새 `UWxViewModel_InventoryItem` 을 만들어 `LastAcquiredItem` 에 덮어쓰는데, 밀려나는 이전 인스턴스에는 `Deinitialize()` 를 부르지 않는다. 그 인스턴스는 `Initialize` 에서 인벤토리 델리게이트 4종(`OnInventoryStackChanged`·`OnInventorySlotChanged`·`OnInventoryChargeChanged`·`OnInventoryContentsChanged`)과 클래스 정적 델리게이트 2종을 구독한 상태라, 아무 위젯도 보지 않는데도 GC 가 수거할 때까지 인벤토리 변경마다 `RefreshFromSource`(스택 수 조회·충전량 조회·아이콘 갱신)를 계속 돌린다. 획득 횟수에 비례해 죽은 구독자가 쌓이므로, 전투 중 드랍이 잦을수록 인벤토리 변경 한 번의 비용이 선형으로 늘어난다. 같은 파일의 `UnbindSource` 는 `LastAcquiredItem` 을 정확히 `Deinitialize()` 하고 있어(`:60`~`:62`) 해제 계약 자체는 이미 인지된 것이며, 교체 경로에서만 빠졌다.
- **제안**: `UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, AcquisitionVM)` 직전에 기존 `LastAcquiredItem` 이 있으면 `Deinitialize()` 한다(`UnbindSource` 와 같은 처리).
- **확신도**: 높음

### 2. 🟡 위젯별로 만드는 뷰모델 셋에 `DestroyInstance` 가 없어 구독이 GC 까지 남는다

- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:83`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:51`, `Source/WxGame/MVVM/WxViewModel_Quest.h:60`
- **범주**: 설계/구조
- **문제**: 같은 모듈의 `UWxViewModelResolver_Inventory`·`_Item`·`_BossCharacter` 는 `DestroyInstance` 에서 `Deinitialize()` 를 불러 뷰 해제 시점에 구독을 끊는다. 반면 위 세 리졸버는 `CreateInstance` 에서 매번 `NewObject` 로 새 VM 을 만들면서 해제 훅이 없어, 위젯이 사라진 뒤에도 VM 이 GC 로 수거될 때까지 소스 구독이 살아 있다. 특히 `UWxViewModel_InteractionList` 는 스캐너의 `OnListChanged` 마다 `RebuildEntries` 로 프롬프트 수만큼 `UWxViewModel_Interaction` 을 새로 할당하므로(`WxViewModel_InteractionList.cpp:96`), 죽은 위젯의 VM 이 상호작용 후보가 바뀔 때마다 계속 할당한다. `UWxViewModel::BeginDestroy` 가 결국 정리하므로 누수는 아니지만, 해제 시점이 정의되지 않은 것은 맞다. `_Ability`·`_PlayerCharacter` 는 ASC 를 Outer 로 공유하는 VM 이라 해제 훅이 없는 것이 의도된 설계이며 이 지적에서 제외한다.
- **제안**: 세 리졸버에 `DestroyInstance` 를 추가해 자기가 만든 인스턴스만 `Deinitialize()` 한다. 겸사겸사 `UWxViewModel_InteractionList::Deinitialize()` 가 `StopObserving()` 을 부르지 않아 `OnAnyScannerReady` 구독이 `BeginDestroy` 까지 남는 점도 같이 맞춘다(`WxViewModel_InteractionList.cpp:47`).
- **확신도**: 높음

### 3. 🟡 MetaHuman 부착물을 걷어내도 리더 메시의 애니메이션 틱 옵션이 복원되지 않는다

- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더의 `VisibilityBasedAnimTickOption` 을 `AlwaysTickPoseAndRefreshBones` 로 올리지만(`:56`), `OnUnregister` 는 표시(`SetVisibility(true)`)만 되돌리고(`:131`) 틱 옵션은 그대로 둔다. 리더가 살아남는 해제 경로(BP 리컴파일, MetaHuman 컴포넌트만 제거·재등록, 레벨 스트리밍 아웃)에서는 부착물이 없어진 뒤에도 화면 밖 본 리프레시 비용이 계속 든다. 표시 복원도 "원래 보이는 상태였다"를 가정하고 무조건 `true` 로 되돌리므로, BP 에서 리더를 숨겨 둔 캐릭터라면 해제 후 원치 않게 드러난다. 월드가 통째로 파괴되는 경우에는 둘 다 영향이 없다.
- **제안**: 리더를 건드리기 전에 원래 표시 상태와 틱 옵션을 멤버에 저장하고, 그 변경을 적용한 등록 주기의 `OnUnregister` 에서 둘 다 복원한다.
- **확신도**: 높음

### 4. 🟢 입력 액션 콜백 다섯 개가 `Handle` 접두사 규칙을 따르지 않는다

- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:113`, `Source/WxGame/Character/WxPlayerCharacter.cpp:117`, `Source/WxGame/Character/WxPlayerCharacter.cpp:126`, `Source/WxGame/Character/WxPlayerCharacter.cpp:131`, `Source/WxGame/Character/WxPlayerCharacter.cpp:132`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. 모듈의 다른 모든 바인딩(`HandleRagdollTagChanged`·`HandleInventoryReady`·`HandleSPDAttributeChanged`·`HandlePostLoadMap` …)은 이를 지키는데, `BindAction` 에 물린 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 만 예외다. 엔진 이름에 맞춰야 하는 `Jump`·`StopJumping` 은 제외한다.
- **제안**: 다섯 콜백의 선언(`WxPlayerCharacter.h:54`~`:59`)·정의·바인딩을 `Handle` 접두사로 통일한다.
- **확신도**: 높음

### 5. 🟢 기준 이동속도 계산이 두 곳에 그대로 복제돼 있다

- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:211`, `Source/WxGame/Character/WxCharacterBase.cpp:224`
- **범주**: 중복/복잡도
- **문제**: `GetDefault<AWxCharacterBase>(GetClass())->GetCharacterMovement()->MaxWalkSpeed` 를 읽어 SPD 를 곱하는 두 줄이 초기 1회 적용(`InitAbilitySystem`)과 변경 콜백(`HandleSPDAttributeChanged`)에 그대로 두 번 있다. "기준값은 CDO 에서만 읽는다"는 이 계산의 함정이 두 곳에 걸쳐 있어, 한쪽만 고치면 이미 SPD 가 곱해진 인스턴스 값을 기준으로 삼는 버그가 조용히 들어온다.
- **제안**: `ApplyWalkSpeedFromSPD(float NewSPD)` 같은 private 헬퍼 하나로 모으고 두 지점이 그것만 호출하게 한다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h/.cpp`, `Source/WxGame/Framework/WxGameState.h/.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.h/.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Character/Component/WxAIBehaviorComponent.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/FrontEnd/Tests/WxFrontEndTests.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_*.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`.
- **경계 확인 / 후보였으나 기각**: `UAbilitySystemComponent::OnRegister` 가 `InitAbilityActorInfo(Owner, Owner)` 를 이미 수행하므로 `AWxCharacterBase::InitAbilitySystem` 이 `RefreshAbilityActorInfo()` 만 부르는 것은 정상이며, 클라이언트 AI 폰(PlayerState 없음)에서도 ActorInfo 는 유효하다 — 후보였으나 기각. `Plugins/WxWorld/.../WxCheckpointSubsystem` 은 `ShouldCreateSubsystem` 을 오버라이드하지 않아 `WxGameFlowSubsystem.cpp:101` 의 무검사 역참조는 안전. `Plugins/WxUI/.../MVVM/WxViewModel_Item.h` 가 `Deinitialize` 를 오버라이드하므로 `WxViewModel_InventoryItem::UnbindSource` 가 조부모인 `UWxViewModel::Deinitialize()` 를 명시 호출해 정적 표시 데이터를 지키는 것은 의도된 설계. `AWxEnemyCharacter` 의 소환물 태그는 사망 경로와 `EndPlay` 의 `IsAlive()` 게이트로 정확히 한 번만 반납되며 이중 반납 경로는 없다. `IsInRearCone` 의 각도 수식은 0·90·180 경계에서 모두 옳다. 모듈 전체에서 `FORCEINLINE`·인라인 정의·람다·Copyright 첫 줄 누락은 0건이고, `BlueprintCallable` 7건은 전부 Blueprint Function Library(2) / `BlueprintSetter`(1) / 승인된 뷰모델 Command 예외(4)에 해당한다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·자동화 테스트(`Wx.FrontEnd.InvalidContext`)를 실행하지 않았다. `UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement` 가 `ASC->GetAnimatingAbility()` 로 `bWantsToCrouch` 를 끄는 것은 클라/서버 몽타주 상태가 어긋나면 이동 예측 불일치를 낼 수 있는 구조지만, 프로젝트가 "서버가 곧 클라" 전제라 실측 없이 판정하지 않았다. 리스폰 흐름(`RequestRespawn`)의 위젯·뷰모델 재생성 순서도 코드로만 따라갔고 PIE 재현은 하지 않았다. BP/WBP 내부 구조와 데이터 에셋 내용은 범위 밖이다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `53bc7de6` · 리뷰일 2026-09-08 · 소스 66파일 — `/module-review`로 갱신*
