# WxGame — 코드 리뷰

> Experience 부팅·캐릭터 계층·MVVM 접착의 책임 경계가 또렷하고 주석이 설계 근거를 잘 남겨 전반적으로 건강하다 — 🔴 는 이번에도 없다. 다만 직전 리뷰(`cf3a7a0`) 이후 WxGame 코드에는 백스탭 판정의 로컬 플레이어 가정을 걷어낸 리팩터(`IsInRearCone` 가 `Interactor` 를 인자로 받음)만 반영됐고, 나머지 발견은 전부 그대로 남아 이번 리뷰에서 재확인해 다시 싣는다. 커버리지는 66개 파일 전부를 훑고 Framework 8종·Character 7종·Controller 2종·Ability 2종·MVVM 10종의 cpp 까지 내려가 읽었으며, 이 모듈이 호출하는 도메인 지점(`UWxHUDComponent`·`UWxUIManagerSubsystem`·`UWxAbilitySystemComponent`·`UWxInventoryManagerComponent`·`AWxDialogueActor`)은 교차 확인만 했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 HUD 클래스 발행이 그 값을 읽는 액션 활성보다 늦다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:279-290`
- **범주**: 버그/정확성
- **문제**: `FinishExperienceLoad` 는 전체 액션을 먼저 활성화하고(`:281-287`) 그다음에 `WxPublishGameHUDClass` 로 HUD 지정을 UI 매니저에 싣는다(`:290`). 그런데 그 액션 중 하나가 `UWxHUDComponent`(WxUI)를 컨트롤러에 주입하고, 이 컴포넌트는 `BeginPlay` 에서 "주입이 빙의보다 늦은 경우"를 위해 현재 폰으로 즉시 따라잡기를 돈다(`Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp:26`) — 그 경로가 곧바로 `GetGameHUDClass()` 를 읽는다(`:66`). `AddComponentRequest` 로 만들어진 컴포넌트의 등록·BeginPlay 는 액션 활성 루프 안에서 동기로 일어나므로 이 읽기는 `:290` 보다 항상 앞선다. 서버는 로드 완료 전 폰 스폰을 막아 따라잡기 시점에 폰이 없어 무해하지만, 클라는 폰·빙의가 자기 로드 파이프라인과 무관하게 복제로 먼저 도착할 수 있다. 그러면 직전 세계의 `EndPlay` 가 비워 둔(`:70`) 지정을 읽어 push 가 무동작이 되고, `SetGameHUDClass` 에 재시도 후크가 없어(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:123-126`) 그 클라는 폰이 다시 바뀔 때까지 HUD 없이 남는다. `WxHUDComponent.cpp:65` 의 주석("빙의는 Experience 로드 완료 뒤라")이 서버 기준으로만 참인 것도 같은 함정이다.
- **제안**: `WxPublishGameHUDClass(this, CurrentExperience)` 를 액션 활성 루프 앞으로 옮긴다. HUD 지정은 순수 데이터라 액션 실행에 의존하지 않으므로 순서만 바꾸면 "읽을 때 이미 발행돼 있다"가 무조건 참이 된다.
- **확신도**: 중간

### 2. 🟡 `InitAbilitySystem` 이 재진입에 멱등하지 않다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:200-215`, `Source/WxGame/Character/WxCharacterBase.h:124`
- **범주**: 버그/정확성
- **문제**: 호출될 때마다 (1) `BaseWalkSpeed = MaxWalkSpeed` 를 다시 잡고(`:202`), (2) SPD 변경 델리게이트를 `AddUObject` 로 또 붙인다(`:207-208`). 진입점이 `PossessedBy`(서버, `:108`)와 `OnRep_PlayerState`(클라, `WxPlayerCharacter.cpp:64`) 두 곳인데, 후자는 널 가드 없이 곧장 호출한다 — 같은 모듈의 `OnRep_CurrentExperience` 가 명시하듯(`WxExperienceManagerComponent.cpp:164`) RepNotify 는 참조가 unmapped 로 먼저 도착했다가 매핑 후 다시 불릴 수 있고, `PlayerState` 도 같은 성질이다. 두 번째 호출 시점엔 이미 SPD 배율이 곱해진 `MaxWalkSpeed` 가 새 기준값이 되어 다음 SPD 변경부터 배율이 복리로 누적된다(클라만 `Base*SPD²`, 서버와 어긋남). `GiveAbilitySet` 은 `bAbilitySetGranted` 가드가 있어(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:21`) 이중 부여만은 해소된 상태다. 부수적으로 `BaseWalkSpeed` 는 헤더에서 초기화되지 않고(`WxCharacterBase.h:124`), 바인딩 직후 현재 SPD 를 1회 적용하지 않아 바인딩 전에 이미 배율이 복제돼 있던 클라 폰은 다음 변경까지 기준 속도로 남는다.
- **제안**: `BaseWalkSpeed` 캡처와 델리게이트 바인딩을 `PostInitializeComponents` 로 옮겨 1회로 못박고, 이 함수에는 액터 인포 갱신·권위 부여만 남긴다. `BaseWalkSpeed` 에는 헤더 기본값을 준다.
- **확신도**: 중간

### 3. 🟡 `AWxNpc` 의 `MetaHumanComponent` 는 조립이 불가능한 데드 합성이다
- **위치**: `Source/WxGame/Character/WxNpc.cpp:37`, `Source/WxGame/Character/WxMetaHumanComponent.cpp:36-41`
- **범주**: 중복/복잡도
- **문제**: `UWxMetaHumanComponent::OnRegister` 는 오너를 `Cast<ACharacter>` 해 `GetMesh()` 를 리더 메시로 잡고, 실패하면 조용히 반환한다(`WxMetaHumanComponent.cpp:36-41`). `AWxNpc` 의 베이스 `AWxDialogueActor` 는 `AActor` 직상속이라(`Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h:20`) 이 캐스트가 항상 실패하고, 생성자에서 붙이는 `MetaHumanComponent`(`WxNpc.cpp:37`)는 어떤 슬롯을 채워도 바디·페이스·그룸을 만들지 않는다. 헤더 주석(`WxNpc.h:17`)은 "외형 컴포넌트를 얹는 합성"이 이 클래스의 역할이라고 적어 기획자가 BP_Npc 슬롯을 채우면 동작할 것처럼 읽히는데, 코드에 경고조차 없어 채우는 순간 무증상 실패가 된다. `AWxNpc` 는 자기 `MeshComponent`(`WxNpc.cpp:26`)를 갖고 있으므로 리더로 쓸 메시 자체는 존재한다.
- **제안**: 둘 중 하나를 고른다 — (a) `AWxNpc` 에서 `MetaHumanComponent` 와 관련 주석을 걷어낸다, (b) 오너가 `ACharacter` 가 아니면 지정된 `USkeletalMeshComponent` 를 리더로 쓰는 경로를 열어 조립이 성립하게 한다. 어느 쪽이든 슬롯이 채워졌는데 리더 메시를 못 찾으면 `UE_LOG(Error)` 로 드러낸다.
- **확신도**: 높음

### 4. 🟡 적 처치 보상이 항상 `PlayerController 0` 에게 간다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:77-81`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 권위에서 `UGameplayStatics::GetPlayerController(this, 0)` 를 보상 수령자로 쓴다. 리슨/데디케이티드 서버에서 0번은 "호스트 또는 첫 접속자"이지 처치자가 아니므로, 다른 플레이어가 잡은 적의 재화·픽업이 엉뚱한 인벤토리로 들어간다. 같은 클래스의 백스탭 판정은 이번 사이클에 `IsInRearCone(const AActor* Interactor)`(`:43-60`)로 바뀌어 로컬 플레이어 가정을 걷어냈는데, 보상 경로만 그 가정이 남았다. 게다가 이 클래스에는 처치자를 되짚을 재료가 없다 — 사망은 `Ability.Death` 태그 변화로만 전달된다(`WxCharacterBase.cpp:73-74`).
- **제안**: 마지막 가해자를 남기려면 대미지 GE 컨텍스트의 Instigator 를 사망 이벤트 페이로드까지 전달하도록 WxCombat 쪽을 손봐야 한다. 싱글 전용으로 확정한 설계라면 최소한 이 지점에도 "MP 비대응"을 주석으로 명시해 다음 사람이 함정에 빠지지 않게 한다.
- **확신도**: 중간

### 5. 🟡 퀘스트 뷰모델 리졸버가 컴포넌트 늦은 도착을 처리하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp:68-82`
- **범주**: 설계/구조
- **문제**: `UWxViewModelResolver_Quest::CreateInstance` 는 생성 시점에 `GameState` 에서 `UWxQuestComponent` 를 못 찾으면 `nullptr` 를 돌려주고 끝이다(`:72-76`). 이 컴포넌트는 Experience 액션으로 주입되며 클라는 자기 로드 파이프라인이 끝나야 생기는데, HUD 는 빙의 시점에 푸시되고 서버는 서버 로드 완료만 기다려 폰을 스폰하므로, 클라 로드가 빙의보다 늦으면 그 클라의 퀘스트 HUD 는 세션 내내 비어 있다. 같은 모듈의 `UWxViewModel_Inventory`·`UWxViewModel_InteractionList` 는 정확히 이 이유로 `OnAny*Ready` 관찰 구조를 두고 있다(`WxViewModel_Inventory.cpp:12-29`, `:191-201`). 헤더 주석(`WxViewModel_Quest.h:51`)은 "미등록 게임모드에선 null" 만 언급해 타이밍 문제를 가린다.
- **제안**: `UWxViewModel_Quest` 를 `StartObserving(UWorld*)` 형태로 바꿔 인스턴스는 고정한 채 컴포넌트 도착 신호로 `Initialize` 를 늦게 수행한다(인벤토리 VM 과 동일 패턴). 스탠드얼론 전용으로 둘 거라면 주석에 그 전제를 적는다.
- **확신도**: 중간

### 6. 🟢 엔진 `Reset()` 경로에서 시작 인벤토리가 재지급된다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:28-30`, `Source/WxGame/Framework/WxGameMode.cpp:101-118`
- **범주**: 버그/정확성
- **문제**: `AGameModeBase::Reset()` 은 `InitGameState()` 를 다시 호출한다. `SetCurrentExperience` 는 그 경로를 멱등 처리하지만(`WxExperienceManagerComponent.cpp:110-113`), 바로 다음 줄의 `CallOrRegister_OnExperienceLoaded`(`WxGameMode.cpp:30`)는 이미 로드된 상태면 즉시 실행되어 `HandleExperienceLoaded` → 모든 접속자에게 `GrantDefaultInventory`(`:111`)를 다시 돌린다. 인벤토리는 컨트롤러 소유라 `Reset` 에도 비워지지 않으므로 시작 아이템이 중복 지급된다. 현재 프로젝트에 `ResetLevel`/`RestartGame` 호출이 없어 잠복 상태다.
- **제안**: `SetCurrentExperience` 가 실제로 새 Experience 를 설정했을 때만 델리게이트를 등록하도록 반환값을 두거나, `GrantDefaultInventory` 에 "이미 지급된 컨트롤러" 가드를 둔다.
- **확신도**: 중간

### 7. 🟢 처형 프롬프트만 하드코딩 비지역화 문자열이다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:105-108`
- **범주**: 버그/정확성
- **문제**: `GetInteractionPrompt` 가 `FText::FromString(TEXT("Finisher"))` 를 돌려준다. 다른 `IWxInteractable` 구현은 전부 데이터에서 오거나 지역화 대상이다 — `AWxDialogueActor` 는 대화 컴포넌트의 프롬프트, `AWxDevice` 는 StateTree 가 세팅한 프롬프트, `AWxItemPickup` 은 `NSLOCTEXT` 포맷이다. 적 처형만 코드에 문자열이 박혀 있다. 뒤잡/앞잡에 따라 문구를 달리할 여지도 막혀 있다(자격 판정은 이미 `CanInteract` 가 둘을 가른다, `:120-141`).
- **제안**: `EditDefaultsOnly FText FinisherPrompt`(필요하면 `BackstabPrompt` 분리)를 두고 기본값은 `NSLOCTEXT` 로 준다.
- **확신도**: 높음

### 8. 🟢 획득 토스트용 임시 아이템 뷰모델이 정리 없이 버려진다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-113`
- **범주**: 성능/안전
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 Def 모드 `UWxViewModel_Item` 을 새로 만들어 `LastAcquiredItem` 에 꽂는다. 그 `Initialize` 는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 두 델리게이트를 구독하는데(`WxViewModel_Item.cpp:45-46`), 다음 획득이 `LastAcquiredItem` 을 덮을 때도 `Deinitialize` 를 부르지 않고, `UWxViewModel_Inventory::Deinitialize` 도 이 필드를 `nullptr` 로 놓기만 한다(`:58`). `AddUObject` 는 약참조라 댕글링은 없고 GC 후 브로드캐스트에서 압축되지만, 그 전까지 획득 1회당 구독자 2개가 쌓여 모든 스택·충전 변경마다 죽은 VM 들이 함께 돈다. 같은 파일이 `AllItems` 자식 VM 에 대해서는 교체 시 `Deinitialize` 를 꼬박꼬박 부르고 있어(`:152-158`) 이 경로만 규율에서 빠져 있다.
- **제안**: `LastAcquiredItem` 을 교체하기 전에 이전 인스턴스의 `Deinitialize()` 를 부른다. 애초에 이 VM 은 표시 데이터만 필요하므로, Def 모드 `Initialize` 대신 `ApplyStaticDataFromDef` 상당의 정적 세팅만 하는 경량 경로를 두면 구독 자체가 불필요해진다.
- **확신도**: 중간

### 9. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.h:7`, `Source/WxGame/WxGame.Build.cs:11`, `Source/WxGame/WxGame.Build.cs:39-44`
- **범주**: 설계/구조
- **문제**: `MetaHumanSDKRuntime`·`HairStrandsCore` 는 `PrivateDependencyModuleNames`(`:39-44`)인데, 모듈 전체가 공개 인클루드 경로(`PublicIncludePaths = ModuleDirectory`, `:11`)라 `WxMetaHumanComponent.h` 가 `MetaHumanComponentUE.h` 를 공개적으로 끌어온다. WxGame 을 참조하는 모듈(현재 `Source/WxEditor/WxEditor.Build.cs:31`, 앞으로 GameFeature 플러그인)이 이 헤더를 포함하면 컴파일이 깨진다. 지금은 이 헤더를 포함하는 곳이 모듈 내부 3곳(`WxCharacterBase.cpp`·`WxMetaHumanComponent.cpp`·`WxNpc.cpp`)뿐이라 잠복 상태다.
- **제안**: 두 모듈을 `PublicDependencyModuleNames` 로 올리거나, `UWxMetaHumanComponent` 를 내부 전용으로 둘 거면 `WXGAME_API` 를 떼고 노출 범위를 명시한다.
- **확신도**: 중간

### 10. 🟢 `UWxMetaHumanComponent` 가 리더 메시 설정을 절반만 되돌린다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.cpp:52-54`, `Source/WxGame/Character/WxMetaHumanComponent.cpp:127-135`
- **범주**: 버그/정확성
- **문제**: 바디를 만들 때 오너 메시에 두 가지를 건다 — `SetVisibility(false)`(`:52`)와 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones`(`:54`). `OnUnregister` 는 앞의 것만 되돌리고(`:128-135`) 뒤의 것은 그대로 남긴다. BP 리컴파일·레벨 스트리밍으로 등록/해제가 반복되는 동안 오너 메시는 "보이는데 항상 본을 갱신하는" 상태로 남아, 화면 밖에서도 스켈레탈 평가가 계속 도는 비용을 문다(이 컴포넌트가 명시적으로 피하려던 비용이며, `CreateAttachedMesh` 는 부착물에 `OnlyTickPoseWhenRendered` 를 주고 있다, `:178`). 에디터에서는 그 상태로 맵을 저장하면 인스턴스 값으로 굳을 수도 있다.
- **제안**: 변경 전 값을 컴포넌트에 기억해 두고 `OnUnregister` 에서 `SetVisibility` 와 함께 복원한다. 기억할 값을 늘리기 싫다면 최소한 `EVisibilityBasedAnimTickOption::AlwaysTickPose` 기본값으로 되돌린다.
- **확신도**: 중간

### 11. 🟢 `BackstabRearHalfAngle` 의 주석 의미와 실제 수식이 어긋난다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.h:62-67`, `Source/WxGame/Character/WxEnemyCharacter.cpp:58-59`
- **범주**: 버그/정확성
- **문제**: 주석은 "정면 기준 이 각도(도) 바깥의 후방 원뿔"이라고 적었는데, 코드는 `RearThreshold = -cos(각도)` 로 임계를 잡아(`:58`) 사실상 **후방 벡터 기준 반각**을 구현한다. 기본값 90 에서만 두 해석이 "후방 반구"로 일치하고, 그 밖의 값에서는 정반대에 가깝게 벌어진다 — 기획자가 45 를 넣으면 "정면 45° 바깥"(뒤쪽 대부분)을 기대하지만 실제로는 "정확히 뒤에서 45° 이내"(뒤쪽 좁은 원뿔)만 성립한다. 프로퍼티 이름(`...RearHalfAngle`)은 코드 쪽 해석과 맞으므로 틀린 것은 주석이다.
- **제안**: 주석을 "후방(정반대) 방향 기준 반각 — 90 이면 후방 반구"로 정정한다. 정면 기준 해석을 쓰고 싶다면 수식을 `ForwardDot <= FMath::Cos(FMath::DegreesToRadians(각도))` 로 바꾸고 프로퍼티 이름도 함께 손본다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp` (각 헤더 포함)
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`
- **미검토 / 한계**:
  - 규칙 위반 스캔은 이번에도 깨끗하다: 모듈 전체에 `FORCEINLINE`·인라인 정의·람다가 없고, 66개 파일 전부 첫 줄 저작권 표기를 갖췄으며 `Wx` 접두사도 일관된다.
  - 뷰모델 명령 함수의 `BlueprintCallable`(`WxViewModel_Dialogue.h:32`, `WxViewModel_InteractionList.h:48`·`:51`, `WxViewModel_Inventory.h:83`, `WxViewModel_Item.h:47`)과 입력 콜백의 `Handle` 접두사 누락(`WxPlayerCharacter` 의 `Move`·`Look`·`ToggleCrouch`·`AbilityInput*`)은 직전 리뷰들에서 "프로젝트가 확정한 예외 / 엔진 관례를 따른 의도"로 판정된 사항이라 이번에도 발견으로 올리지 않았다. 판정을 뒤집을 근거는 새로 나오지 않았다.
  - `AbilitySystemComponent` 의 액터 인포는 엔진 `UAbilitySystemComponent::InitializeComponent` 가 `InitAbilityActorInfo(Owner, Owner)` 를 대신 수행하므로, 프로젝트에 `InitAbilityActorInfo` 호출이 한 곳도 없는 것은 문제가 아님을 확인했다(발견에서 제외).
  - `UWxCharacterMovementComponent::OnMovementModeChanged` 가 `MOVE_Falling` 에서 나오는 모든 전이(사망 시 `DisableMovement` 포함)에 `JumpToLandingSection` 을 돌리는 점은, 대상 몽타주에 `Grounded` 섹션이 있어야만 발화하는 구조라 실제 오작동 사례를 특정하지 못해 보류했다.
  - `UWxExperienceManagerComponent::EndPlay` 가 진행 중이던 비동기 로드(`Loading`·`LoadingGameFeatures`)를 취소하지 않는 점과 `UWxExperienceManager::RequestToDeactivatePlugin` 의 `FindChecked`(`:37`)는 레퍼런스(Lyra)와 동일한 형태이고 `Plugins/GameFeatures` 가 아직 비어 있어 판단을 보류했다.
  - `CollectActions`(`WxExperienceManagerComponent.cpp:315-339`)가 `CollectGameFeaturePluginURLs` 와 달리 중복 제거를 하지 않아 같은 `UWxExperienceActionSet` 을 두 번 등록하면 액션이 두 번 활성화되고 시작 아이템도 두 번 지급되지만, 데이터 오저작 전제라 발견으로 올리지 않았다.
  - 도메인 플러그인은 이 모듈이 부르는 지점만 교차 확인했다 — 그 내부 정확성은 보지 않았다.
  - BP/WBP 의 실제 슬롯·바인딩 구성은 범위 밖이다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 66파일 — `/module-review`로 갱신*
