# WxGame — 코드 리뷰

> 부트스트랩(Experience 파이프라인)·캐릭터 계층·MVVM 접착 코드 모두 책임 분리와 주석의 설계 의도가 또렷해 전반적으로 건강하다. 🔴 는 없고, 남은 발견은 대부분 "싱글플레이 전제가 코드에 함정으로 남은 것"과 재진입·타이밍 멱등성 부족이다. 이번 리뷰는 프레임워크 6종·캐릭터 5종·컨트롤러·어빌리티 2종·MetaHuman 컴포넌트·MVVM 8종의 cpp 까지 통독했고, 직전 리뷰(`bd689a19`) 이후 바뀐 HUD 발행 경로와 백스탭 자격 판정을 새로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 HUD 클래스 발행이 그 값을 읽는 액션 활성보다 늦다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:281-290`
- **범주**: 버그/정확성
- **문제**: `FinishExperienceLoad` 는 액션을 먼저 활성화하고(`:281-287`) 그다음에 `WxPublishGameHUDClass` 로 HUD 지정을 UI 매니저에 싣는다(`:290`). 그런데 그 액션 중 하나가 `UWxHUDComponent`(WxUI) 를 컨트롤러에 주입하고, 이 컴포넌트는 `BeginPlay` 에서 "주입이 빙의보다 늦은 경우"를 위해 현재 폰으로 즉시 따라잡기를 돈다(`Plugins/WxUI/Source/WxUI/Private/Component/WxHUDComponent.cpp:26`) — 그 경로가 곧바로 `GetGameHUDClass()` 를 읽는다(`:66`). 서버는 로드 완료 전 폰 스폰을 막으므로 따라잡기 시점에 폰이 없어 무해하지만, 클라는 폰·빙의가 자기 로드 파이프라인과 무관하게 복제로 먼저 도착할 수 있어 폰이 이미 있는 상태에서 컴포넌트가 붙는다. 그러면 아직 비어 있는(직전 세계에서 `EndPlay` 가 지운, `:70`) 지정을 읽어 push 가 무동작이 되고, 재시도 후크가 없어 그 클라는 세션 내내 HUD 없이 남는다. 워크로그(2026-08-22 HUD Experience 컴포넌트화)는 "빙의는 항상 로드 완료 뒤"를 전제로 이 순서를 정당화했는데, 그 전제는 서버에서만 성립한다.
- **제안**: `WxPublishGameHUDClass(this, CurrentExperience)` 를 액션 활성 루프 앞으로 옮긴다. HUD 지정은 순수 데이터라 액션 실행에 의존하지 않으므로 순서만 바꾸면 전제가 무조건 참이 된다.
- **확신도**: 중간

### 2. 🟡 `InitAbilitySystem` 이 재진입에 멱등하지 않다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:200-214`, `Source/WxGame/Character/WxPlayerCharacter.cpp:60-65`, `Source/WxGame/Character/WxEnemyCharacter.cpp:46-52`
- **범주**: 버그/정확성
- **문제**: 호출될 때마다 (1) `BaseWalkSpeed = MaxWalkSpeed` 를 다시 잡고(`:202`), (2) SPD 변경 델리게이트를 `AddUObject` 로 또 붙이며(`:207-208`), (3) 권위면 `GiveAbilitySet()` 을 또 호출한다(`:213`). `UWxAbilitySystemComponent::GiveAbilitySet` 은 가드 없이 부여를 재실행하고 핸들 컨테이너에 덧쌓는다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:17-24`). 파생 `AWxEnemyCharacter::InitAbilitySystem` 도 대미지 구독을 중복 추가한다. 진입점이 `PossessedBy`(서버)와 `OnRep_PlayerState`(클라) 두 곳이라, 두 번 불리면 SPD 배율이 걸린 상태의 속도가 새 기준값이 되어 배율이 복리로 곱해지고 서버에선 어빌리티가 이중 부여된다. 현재 같은 폰을 재빙의하는 호출이 없어 잠복 상태이지만, 리스폰·컨트롤러 스왑을 도입하는 순간 터진다. 부수적으로 `BaseWalkSpeed` 는 헤더에서 초기화되지 않고(`WxCharacterBase.h:124`), 바인딩 직후 현재 SPD 를 1회 적용하지 않아 바인딩 전에 이미 배율이 복제돼 있던 클라 폰은 다음 변경까지 기준 속도로 남는다.
- **제안**: `InitAbilitySystem` 을 1회 가드(이미 초기화된 ASC 면 `RefreshAbilityActorInfo` 만)하거나, `BaseWalkSpeed` 캡처·델리게이트 바인딩을 `PostInitializeComponents` 로 옮기고 이 함수에는 액터 인포 갱신·권위 부여만 남긴다. `GiveAbilitySet` 은 `AbilitySetGrantedHandles` 가 비어 있을 때만 부여하도록 WxCombat 쪽 가드를 함께 둔다.
- **확신도**: 중간

### 3. 🟡 적 처치 보상이 항상 `PlayerController 0` 에게 간다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:109-113`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 권위에서 `UGameplayStatics::GetPlayerController(this, 0)` 를 보상 수령자로 쓴다. 리슨/데디케이티드 서버에서 0번은 "호스트 또는 첫 접속자"이지 처치자가 아니므로, 다른 플레이어가 잡은 적의 재화·픽업이 엉뚱한 인벤토리로 들어간다. 같은 클래스의 `HandleIncomingDamageChanged` 가 이미 GE 컨텍스트에서 가해자를 얻고 있어(`:62-63`) 처치자 식별 재료는 갖춰져 있다. 같은 0번 플레이어 가정이 백스탭 판정(`IsLocalPlayerInRearCone`, `:76`)에도 있으나 그쪽은 헤더 주석(`WxEnemyCharacter.h:54-59`)이 한계를 명시해 둔 상태다.
- **제안**: 마지막 가해자(사망을 일으킨 GE 의 Instigator)에서 컨트롤러를 역추적해 `GrantReward` 의 수령자로 넘긴다. 싱글 전용으로 확정한 설계라면 보상 쪽에도 백스탭 판정처럼 "MP 비대응" 을 주석에 명시한다.
- **확신도**: 중간

### 4. 🟡 퀘스트 뷰모델 리졸버는 컴포넌트 늦은 도착을 처리하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp:128-141`
- **범주**: 설계/구조
- **문제**: `UWxViewModelResolver_Quest::CreateInstance` 는 생성 시점에 `GameState` 에서 `UWxQuestComponent` 를 못 찾으면 `nullptr` 를 돌려주고 끝이다(`:132-136`). 이 컴포넌트는 Experience 액션으로 주입되며 복제되지 않아 클라는 자기 로드 파이프라인이 끝나야 생기는데, HUD 는 빙의 시점에 푸시되고 서버는 서버 로드 완료만 기다려 폰을 스폰하므로, 클라 로드가 빙의보다 늦으면 그 클라의 퀘스트 HUD 는 세션 내내 비어 있다. 같은 모듈의 `UWxViewModel_Inventory`·`UWxViewModel_InteractionList` 는 정확히 이 이유로 `OnAny*Ready` 관찰 구조를 두고 있다(`WxViewModel_Inventory.cpp:132-149`). 헤더 주석(`WxViewModel_Quest.h:51`)은 "미등록 게임모드에선 null" 만 언급해 타이밍 문제를 가린다.
- **제안**: `UWxViewModel_Quest` 를 `StartObserving(UWorld*)` 형태로 바꿔 인스턴스는 고정한 채 컴포넌트 도착 신호로 `Initialize` 를 늦게 수행한다. 스탠드얼론 전용으로 둘 거라면 주석에 그 전제를 적는다.
- **확신도**: 중간

### 5. 🟡 `AWxNpc` 의 `MetaHumanComponent` 는 조립이 불가능한 데드 합성이다
- **위치**: `Source/WxGame/Character/WxNpc.cpp:75`, `Source/WxGame/Character/WxNpc.h:17`, `Source/WxGame/Character/WxMetaHumanComponent.cpp:216-221`
- **범주**: 중복/복잡도
- **문제**: `UWxMetaHumanComponent::OnRegister` 는 오너를 `Cast<ACharacter>` 해 `GetMesh()` 를 리더로 잡고, 실패하면 조용히 반환한다(`WxMetaHumanComponent.cpp:216-221`). `AWxNpc` 는 `AWxDialogueActor`(`AActor` 직상속)라 이 캐스트가 항상 실패하므로, 생성자에서 붙이는 `MetaHumanComponent`(`WxNpc.cpp:75`)는 어떤 슬롯을 채워도 바디·페이스·그룸을 만들지 않는다. 헤더 주석(`WxNpc.h:17`)은 "외형 컴포넌트를 얹는 합성"이 이 클래스의 역할이라고 적어 기획자가 BP_Npc 슬롯을 채우면 동작할 것처럼 읽힌다. 알려진 공백이지만 코드에 경고조차 없어 채우는 순간 무증상 실패가 된다.
- **제안**: 둘 중 하나를 고른다 — (a) `AWxNpc` 에서 `MetaHumanComponent` 와 관련 주석을 걷어낸다, (b) 오너가 `ACharacter` 가 아니면 지정된 `USkeletalMeshComponent` 를 리더로 쓰는 경로를 열어 조립이 성립하게 한다. 어느 쪽이든 슬롯이 채워졌는데 리더 메시를 못 찾으면 `UE_LOG(Error)` 로 드러낸다.
- **확신도**: 높음

### 6. 🟢 엔진 `Reset()` 경로에서 시작 인벤토리가 재지급된다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:28-29`, `Source/WxGame/Framework/WxGameMode.cpp:100-117`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:110-113`
- **범주**: 버그/정확성
- **문제**: `SetCurrentExperience` 는 `AGameModeBase::Reset()` 이 `InitGameState` 를 재호출하는 경우를 멱등 처리한다고 명시하지만(`WxExperienceManagerComponent.h:45-48`, `.cpp:110-113`), 바로 다음 줄의 `CallOrRegister_OnExperienceLoaded`(`WxGameMode.cpp:29`) 는 이미 로드된 상태면 즉시 실행되어 `HandleExperienceLoaded` → 모든 접속자에게 `GrantDefaultInventory`(`:110`) 를 다시 돌린다. 인벤토리는 컨트롤러 소유라 `Reset` 에도 비워지지 않으므로 시작 아이템이 중복 지급된다. 현재 프로젝트에 `ResetLevel`/`RestartGame` 호출은 없어 잠복 상태다.
- **제안**: `SetCurrentExperience` 가 실제로 새 Experience 를 설정했을 때만 델리게이트를 등록하도록 반환값을 두거나, `GrantDefaultInventory` 에 "이미 지급된 컨트롤러" 가드를 둔다.
- **확신도**: 중간

### 7. 🟢 처형 프롬프트만 하드코딩 비지역화 문자열이다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:137-140`
- **범주**: 버그/정확성
- **문제**: `GetInteractionPrompt` 가 `FText::FromString(TEXT("Finisher"))` 를 돌려준다. 다른 `IWxInteractable` 구현(`AWxDevice`·`AWxTriggerDevice`·`AWxDialogueActor`)은 전부 에셋 편집 가능한 `FText` 필드를 반환해 기획자가 문구를 바꾸고 지역화할 수 있는데, 적 처형만 코드에 박혀 있다. 뒤잡/앞잡에 따라 문구를 달리할 여지도 막혀 있다(자격 판정은 이미 `CanInteract` 가 둘을 가른다, `:147-168`).
- **제안**: `EditDefaultsOnly FText FinisherPrompt`(필요하면 `BackstabPrompt` 분리)를 두고 기본값은 `NSLOCTEXT` 로 준다.
- **확신도**: 높음

### 8. 🟢 `UWxGameFeatureAction_AddComponents` 의 활성/비활성 override 가 `Super` 를 부르지 않는다
- **위치**: `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:120`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:137`
- **범주**: 규칙 위반
- **문제**: `OnGameFeatureActivating(FGameFeatureActivatingContext&)`·`OnGameFeatureDeactivating(FGameFeatureDeactivatingContext&)` 의 엔진 기본 구현은 인자 없는 `OnGameFeatureActivating()`/`OnGameFeatureDeactivating()` 훅으로 위임한다. `Super` 를 생략하면 그 훅이 영영 불리지 않는다. 현재 이 클래스의 파생도, 인자 없는 훅을 쓰는 코드도 없어 실동작 차이는 없다(Lyra 의 `WorldActionBase` 도 같은 형태).
- **제안**: 두 함수 첫 줄에 `Super::` 호출을 추가한다. 생략을 유지할 거라면 이유를 한 줄 주석으로 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 9. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.h:7`, `Source/WxGame/WxGame.Build.cs:11`, `Source/WxGame/WxGame.Build.cs:42-43`
- **범주**: 설계/구조
- **문제**: `MetaHumanSDKRuntime`·`HairStrandsCore` 는 `PrivateDependencyModuleNames` 인데, 모듈 전체가 공개 인클루드 경로(`PublicIncludePaths = ModuleDirectory`)라 `WxMetaHumanComponent.h` 가 `MetaHumanComponentUE.h` 를 공개적으로 끌어온다. WxGame 을 참조할 수 있는 GameFeature 플러그인이 이 헤더를 (직접이든 `AWxNpc`·`AWxCharacterBase` 경유든) 포함하면 컴파일이 깨진다. 지금은 외부 포함처가 없어 잠복 상태다.
- **제안**: 두 모듈을 `PublicDependencyModuleNames` 로 올리거나, `UWxMetaHumanComponent` 를 내부 전용으로 둘 거면 `WXGAME_API` 를 떼고 노출 범위를 명시한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp` (각 헤더 포함)
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`
- **미검토 / 한계**: 도메인 플러그인은 이 모듈이 부르는 지점만 교차 확인했다(`UWxAbilitySystemComponent::GiveAbilitySet`·`GetAbilityInputActions`, `UWxHUDComponent`, `UWxViewModel` 베이스, `UWxInteractionScannerComponent` 의 이벤트 페이로드) — 그 내부 정확성은 보지 않았다. `EndPlay` 가 진행 중이던 비동기 로드(`Loading`·`LoadingGameFeatures` 상태)를 취소하지 않아 월드 종료 뒤 `FinishExperienceLoad` 가 도는 경로는, GameFeature 플러그인이 아직 하나도 없고(`Plugins/GameFeatures` 부재) 레퍼런스(Lyra)와 같은 형태라 판단을 보류했다. 뷰모델의 `BlueprintCallable` 명령 함수는 프로젝트가 규칙 예외로 확정한 사항이라 이번 리뷰에서 제외했다. BP/WBP 의 실제 슬롯·바인딩 구성은 범위 밖이다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 66파일 — `/module-review`로 갱신*
