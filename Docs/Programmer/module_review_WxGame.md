# WxGame — 코드 리뷰

> 게임 조립 모듈답게 파일마다 책임이 얇고 경계가 또렷하며, 권위/복제 의도가 주석으로 성실히 남아 있어 전반적으로 건강하다. 다만 "서버에서만 돈다"는 전제가 실제 GAS 동작과 어긋나 클라 크래시로 이어지는 지점이 하나 있고, 싱글플레이 가정이 남은 코드와 문서-구현 불일치가 몇 군데 보인다. 이번 리뷰는 `Source/WxGame` 의 44개 소스(.h/.cpp)를 모두 열람하고, 캐릭터·컨트롤러·어빌리티·MVVM 의 cpp 로직까지 내려가 확인했다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 6 |

## 결과

### 1. 🔴 UseItem 어빌리티가 소유 클라이언트에서도 인벤토리 차감을 호출해 `check()` 실패
- **위치**: `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:74`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.h:13`
- **범주**: 버그/정확성
- **문제**: `HandleConsumeEvent` 주석("ServerInitiated 라 이 경로는 서버에서만 실행된다")과 노티파이 헤더 주석("실제 사용 처리는 ServerInitiated 어빌리티(서버 인스턴스)에서만 일어나므로 안전하다")의 전제가 틀렸다. `ServerInitiated` 는 `ServerOnly` 가 아니므로 엔진이 서버 활성화 직후 소유 클라에 활성화를 복제한다 — UE 5.8 `AbilitySystemComponent_Abilities.cpp:1895` 의 `if (!bIsLocal && Ability->GetNetExecutionPolicy() != ServerOnly)` → `ClientActivateAbilitySucceed` → 같은 파일 `:2464` 의 `InstancedAbility->CallActivateAbility(...)`. 즉 클라에서도 `ActivateAbility` 가 돌아 `UAbilityTask_WaitGameplayEvent` 가 걸리고, 몽타주도 클라에서 재생되어 `UWxAnimNotify_UseItem::Notify` 가 로컬로 `Event.UseItem` 을 송출한다. 그 결과 클라에서 `HandleConsumeEvent` → `UWxInventoryManagerComponent::FindInventory(Avatar)`(소유 클라엔 PC 의 인벤토리가 복제돼 있어 유효) → `UseItemByDef` 가 호출되고, 그 첫 줄 `check(GetOwner() && GetOwner()->HasAuthority())`(`Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryManagerComponent.cpp:494`)에 걸려 **비-Shipping 빌드에서 클라이언트가 크래시**한다. check 가 빠지는 Shipping 에서도 클라가 로컬로 충전을 한 번 더 깎아 서버와 어긋난다. 리슨 서버 호스트/스탠드얼론은 항상 권위라 재현되지 않고, 원격 클라 세션에서만 드러난다.
- **제안**: `HandleConsumeEvent` 진입부를 `if (!HasAuthority(&CurrentActivationInfo)) { return; }` 로 게이트한다. 두 주석도 "클라 인스턴스도 활성화된다"로 정정한다. 어빌리티를 `ServerOnly` 로 바꾸는 선택지는 클라에서 몽타주가 재생되지 않는 부작용이 있으니 게이트 쪽이 안전하다.
- **확신도**: 높음

### 2. 🟡 처치 보상이 항상 0번 플레이어에게 지급된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:69`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 `UGameplayStatics::GetPlayerController(this, 0)` 를 `GrantReward` 의 `DirectGrantTarget` 으로 넘긴다. 이 인자는 픽업 Fragment 가 없는 보상(골드 등 재화)을 직접 넣을 인벤토리 대상이다(`Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h:28`). 따라서 실제 처치자와 무관하게 서버의 0번 플레이어가 모든 재화를 가져간다. 모듈 전반이 서버 권위·소유 클라 복제를 세심히 지키는 것에 비해 여기만 싱글플레이 가정이 남아 있다.
- **제안**: 사망을 유발한 주체를 보관해 넘긴다. 데미지 경로의 마지막 instigator 를 `AWxCharacterBase` 에 기록하거나, 처형 경로가 이미 다루는 Interactor(`WxEnemyCharacter.cpp:129`)를 재사용해 그 컨트롤러를 `DirectGrantTarget` 으로 쓴다. 당분간 싱글플레이만 지원한다면 그 전제를 주석으로 못박아 두는 것만으로도 낫다.
- **확신도**: 중간

### 3. 🟡 더블 점프 2단 감쇠가 구현되어 있지 않다 (주석만 존재)
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:38`, `Source/WxGame/Character/WxCharacterMovementComponent.h:19`
- **범주**: 버그/정확성
- **문제**: `JumpMaxCount = 2` 위 주석이 "2단 Z속도 절반 적용은 `UWxCharacterMovementComponent::DoJump` 에서 처리한다"고 선언하지만, `UWxCharacterMovementComponent` 에는 `DoJump` 오버라이드가 없다(헤더는 `GetGravityZ`/`UpdateCharacterStateBeforeMovement` 둘뿐이고 cpp 도 동일). 저장소 전체를 검색해도 `DoJump` 정의가 없다. 따라서 2단 점프는 1단과 같은 `JumpZVelocity`(640)로 나가며, 의도한 점프 감각과 다르다.
- **제안**: 의도가 유효하면 `UWxCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)` 를 추가해 `CharacterOwner->JumpCurrentCount >= 1` 일 때 Z 속도를 감쇠시킨다(Super 호출 후 `Velocity.Z` 보정 또는 임시 `JumpZVelocity` 스케일). 폐기된 계획이면 주석을 지운다. 어느 쪽이든 지금은 코드와 주석이 어긋나 있다.
- **확신도**: 높음

### 4. 🟡 `InitAbilitySystem` 이 멱등하지 않아 재호출 시 이동속도가 복리로 부풀고 사망이 중복 방송된다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:188`
- **범주**: 버그/정확성
- **문제**: 이 함수는 (a) `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed` 로 기준 속도를 캡처하고 (b) SPD 어트리뷰트 변경과 `State.Dead` 태그 이벤트에 `AddUObject` 로 구독하는데, 둘 다 중복 방지가 없다. 진입점은 서버 `PossessedBy`(`:105`)와 클라 `AWxPlayerCharacter::OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:77`) 둘이며, 재빙의나 PlayerState 참조 재해석으로 두 번째 호출이 발생하면 이미 SPD 배율이 곱해진 `MaxWalkSpeed` 를 새 기준값으로 캡처해 속도가 복리로 커진다(500 → SPD 1.2 → 600 을 기준으로 다시 720). 동시에 `HandleDeathTagChanged` 가 2회 등록되어 `OnDeath` 가 두 번 방송된다. `AWxPlayerController::BindCharacterDeath` 가 `AddUniqueDynamic` 이라 핸들러 중복은 막지만, 방송 자체가 2회면 사망 화면이 두 장 쌓인다.
- **제안**: `BaseWalkSpeed` 는 생성자나 `PostInitializeComponents` 에서 1회만 캡처하고, `InitAbilitySystem` 에는 초기화 플래그(또는 재등록 전 `RemoveAll(this)`)로 재진입 가드를 둔다.
- **확신도**: 중간 (정상 흐름에선 1회만 호출되므로 잠재 결함)

### 5. 🟡 `OnRep_Pawn` 이 폰 유효성·중복을 검사하지 않고 HUD 를 다시 푸시한다
- **위치**: `Source/WxGame/Controller/WxPlayerController.cpp:66`, `Source/WxGame/Controller/WxPlayerController.cpp:96`
- **범주**: 버그/정확성
- **문제**: `OnRep_Pawn` 은 `BindCharacterDeath(GetPawn())`(내부에서 null 처리)와 달리 `PushGameHUD()` 를 무조건 호출한다. `APlayerController::OnRep_Pawn` 은 Pawn 이 nullptr 로 바뀔 때도 발화하므로, 언포제스·폰 파괴가 복제되면 **빙의 폰이 없는 상태에서 HUD 가 하나 더 푸시**된다. `UWxUIManagerSubsystem::PushContentToLayer` 는 `PushWidgetToLayerStack` 을 그대로 부를 뿐 중복 검사가 없어(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:86`) 레이어에 HUD 가 쌓인다. 게다가 그 HUD 의 리졸버는 생성 시점에 빙의 폰의 ASC 를 읽는데(`Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp:16`) 폰이 없으면 nullptr 를 돌려주므로 바인딩이 죽은 껍데기 HUD 가 최상단에 얹힌다.
- **제안**: `OnRep_Pawn` 에서 `GetPawn()` 이 유효할 때만 `PushGameHUD()` 를 부르고, `PushGameHUD` 안에서도 앞서 푸시한 인스턴스를 약참조로 들고 있다가 살아 있으면 스킵하도록 가드한다.
- **확신도**: 중간

### 6. 🟢 획득 알림 VM(`LastAcquiredItem`)이 교체·해제 시 `Deinitialize` 되지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:82`, 해제부 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:39`
- **범주**: 중복/복잡도
- **문제**: `HandleStackChanged` 는 `Delta > 0` 마다 Def 모드 `UWxViewModel_Item` 을 새로 만들어 `LastAcquiredItem` 에 대입하며, 이 Def 모드 `Initialize` 는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 에 바인딩한다(`Source/WxGame/MVVM/WxViewModel_Item.cpp:46`). 그런데 교체 시에도, `Deinitialize` 시에도(단순 nullptr 대입) 이전 VM 에 `Deinitialize()` 를 부르지 않는다. 바로 아래 `AllItems` 는 미보존 VM 을 일일이 `Deinitialize` 하며 대칭을 지키는데(`:128-134`) 획득 VM 만 예외다. `UWxViewModel::BeginDestroy` 가 `Deinitialize` 를 부르므로(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:7`) GC 시점에는 정리되어 영구 누수는 아니지만, 수거 전까지 무의미한 콜백을 계속 받고 수명 규약이 같은 파일 안에서 어긋난다.
- **제안**: 교체 직전과 `Deinitialize` 시 이전 `LastAcquiredItem` 에 `Deinitialize()` 를 호출해 `AllItems` 경로와 대칭을 맞춘다.
- **확신도**: 중간

### 7. 🟢 `RefreshAllItems` 의 `NewItems`/`Retained` 는 내용이 완전히 동일한 중복 배열
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:95`
- **범주**: 중복/복잡도
- **문제**: 같은 루프에서 `NewItems.Add(ChildVM); Retained.Add(ChildVM);`(`:123-124`)로 동일한 원소만 넣고, `Retained` 는 이후 `Retained.Contains(OldVM)`(`:130`) 판정에만 쓰인 뒤 버려진다. 두 배열은 항상 같으므로 `Retained` 는 순수 중복이다. 부수적으로 기존 VM 매칭 루프(`:108`)와 `Contains` 가 겹쳐 슬롯 수 N 에 대해 O(N²)이지만 인벤토리 규모상 실질 비용 문제는 아니다.
- **제안**: `Retained` 를 제거하고 `NewItems.Contains(OldVM)` 으로 판정한다. 슬롯 수가 커질 여지가 있으면 인스턴스→VM `TMap` 조회로 바꾼다.
- **확신도**: 높음

### 8. 🟢 GameMode 의 Pawn/PlayerState 컴포넌트 주입 분기는 받는 쪽이 없어 조용히 무효다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:42`
- **범주**: 설계/구조
- **문제**: `InitGame` 이 컴포넌트 베이스로 부착 대상을 추론해 `AGameStateBase`/`APawn`/`AController`/`APlayerState` 중 하나로 요청을 등록하지만, 실제로 `AddGameFrameworkComponentReceiver` 를 호출하는 액터는 `AWxGameState`(`Source/WxGame/Framework/WxGameState.cpp:12`)와 `AWxPlayerController`(`Source/WxGame/Controller/WxPlayerController.cpp:34`) 둘뿐이다. 따라서 `FrameworkComponents` 에 `UPawnComponent`/`UPlayerStateComponent` 파생을 넣으면 경고 로그도 없이 아무 일도 일어나지 않는다(추론 실패 경고는 네 종류 어디에도 안 걸릴 때만 뜬다).
- **제안**: 지원할 생각이면 `AWxCharacterBase`/`AWxPlayerState` 에도 receiver 등록을 추가하고, 아니면 두 분기를 제거해 "추론 불가" 경고로 떨어지게 한다.
- **확신도**: 중간 (선제 확장 코드일 수 있음)

### 9. 🟢 규칙 7 위반 — 뷰모델에 `BlueprintCallable` 4건
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:45`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:49`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:44`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:94`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 7 은 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리로 한정한다. `RequestInteract`/`RequestCycle`/`RequestAdvance` 는 WBP 가 뷰모델에 커맨드를 거는 용도라 규칙 밖이다. Wx 플러그인 쪽에서는 `BlueprintCallable` 이 사실상 Library(`WxSaveLibrary`·`WxUILibrary` 등)와 위젯 서브클래스에만 있어, 본 모듈의 뷰모델 커맨드가 유일한 예외군이다. (`UWxViewModel_Inventory::SetCurrentCategory` 는 `BlueprintSetter` 라 엔진 요구에 가깝다.)
- **제안**: 규칙을 지킬 거면 WBP 가 스캐너/세션 컴포넌트를 직접 호출하도록 경로를 바꾸거나 얇은 BP Function Library 를 경유시키고, 유지할 거면 `CLAUDE.md` 규칙 7 에 "뷰모델의 뷰→모델 커맨드 함수" 예외를 명시해 다음 세션이 다시 헷갈리지 않게 한다.
- **확신도**: 높음(위반 사실). 단 의도된 예외일 수 있음

### 10. 🟢 규칙 6 위반 가능 — 입력 바인딩 콜백에 `Handle` prefix 없음
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:60`
- **범주**: 규칙 위반
- **문제**: `Move`/`Look`/`ToggleCrouch`/`AbilityInputStarted`/`AbilityInputTriggered`/`AbilityInputReleased` 는 모두 `UEnhancedInputComponent::BindAction` 으로 델리게이트에 바인딩되는 콜백인데(`Source/WxGame/Character/WxPlayerCharacter.cpp:111-141`) `Handle` prefix 가 없다. 같은 클래스 계층의 다른 델리게이트 콜백(`HandleSPDAttributeChanged`, `HandleEquipVisualChanged` 등)은 규칙을 지키고 있어 모듈 내에서도 일관되지 않다. 저장소에서 `BindAction` 을 쓰는 곳은 이 파일뿐이라 비교할 선례가 없다.
- **제안**: 규칙을 문자 그대로 적용하면 `HandleMoveInput` 등으로 개명한다. 입력 핸들러를 규칙 6 대상에서 뺄 생각이면 `CLAUDE.md` 에 예외를 적어 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 11. 🟢 `IsAlive()` 가 널 검사보다 먼저 `AbilitySystemComponent` 를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:181`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:91`
- **범주**: 성능/안전
- **문제**: `IsAlive()` 는 `AbilitySystemComponent->GetSet<...>()` 를 널 검사 없이 역참조하는데, 같은 클래스의 `GetOwnedGameplayTags`(`:138`)·`CanJumpInternal_Implementation`(`:112`)은 `if (AbilitySystemComponent)` 로 방어한다. 게다가 `AWxEnemyCharacter::GetEligibleFinisherEventTag` 의 가드가 `if (!IsAlive() || !AbilitySystemComponent)` 순서라, ASC 가 널이라면 `!AbilitySystemComponent` 검사에 도달하기 전에 `IsAlive()` 내부에서 이미 역참조한다. 실제로 ASC 는 생성자에서 항상 만들어져 널이 되지 않으므로 크래시로 이어지진 않지만, 방어 의도와 실제 동작이 어긋나 읽는 사람을 오도한다.
- **제안**: `IsAlive()` 에 널 가드를 추가하거나, 반대로 ASC 가 항상 유효하다는 전제로 통일해 불필요한 널 검사들을 정리한다.
- **확신도**: 낮음(현재 경로에서 ASC 는 항상 유효 — 의도된 단순화일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`·`.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`·`.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`·`.h`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`·`.h`, `Source/WxGame/Controller/WxPlayerController.cpp`·`.h`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`·`.h`, `Source/WxGame/WorldObject/WxLaserCorridor.cpp`·`.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`·`.h`, `Source/WxGame/Framework/WxGameState.cpp`·`.h`, `Source/WxGame/Player/WxPlayerState.cpp`·`.h`, `Source/WxGame/Character/WxBossCharacter.cpp`·`.h`, `Source/WxGame/Input/WxInputConfig.cpp`·`.h`, `Source/WxGame/WxGame.cpp`·`.h` (44개 소스 전부 최소 1회 열람)
- **교차 확인(모듈 밖, 근거 확보용)**: `Plugins/WxInventory` 의 `UWxInventoryManagerComponent::UseItemByDef`·`FindInventory` 와 `UWxRewardLibrary::GrantReward`, `Plugins/WxWorld` 의 `AWxGimmick`(`OnInteracted` 가 `PURE_VIRTUAL` 이라 `AWxLaserCorridor` 의 `Super::` 미호출은 정상), `Plugins/WxUI` 의 `UWxUIManagerSubsystem::PushContentToLayer`·`UWxViewModel::BeginDestroy`, UE 5.8 `UAbilitySystemComponent::InternalTryActivateAbility`/`ClientActivateAbilitySucceedWithEventData_Implementation`
- **미검토 / 한계**: (1) 정적 분석만 수행했다 — 1번은 엔진 코드 경로로 확증했으나 실제 원격 클라 세션 재현은 하지 않았다. (2) `WxCombat` 쪽 ASC 확장(`GiveAbilitySet`, `GetAbilityInputActions`, `AbilityInputAction*`)은 호출 규약만 확인하고 내부는 보지 않았다. (3) BP/WBP 디폴트값(`GameHUDWidgetClass`·`RewardRow`·`InputConfig` 실제 에셋, `BehaviorTreeAsset` 등)은 범위 밖이라, 에셋 미지정으로만 가려져 있는 경로가 남아 있을 수 있다. (4) `AWxLaserCorridor` 가 조립하는 `ST_LaserCorridor` StateTree 에셋 로직은 확인하지 않았다. (5) 이전 리뷰(커밋 `9661edf`)의 "`WxAbility_UseItem` 주석이 `WxAbility_Interact` 의 넷 정책을 잘못 기술" 항목은 현재 코드에서 해당 문구가 사라져 해소된 것으로 판단해 제외했다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 44파일 — `/module-review`로 갱신*
