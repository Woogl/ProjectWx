# WxGame — 코드 리뷰

> 부트스트랩(Experience 파이프라인)·캐릭터 계층·MVVM 접착 코드 모두 책임이 또렷하고 주석이 설계 의도를 잘 남겨 전반적으로 건강하다. 🔴 는 없고, 발견은 대부분 "알려진 공백이 코드에 함정으로 남은 것"과 MP·재진입 경로의 멱등성 부족이다. 프레임워크(GameMode·ExperienceManager·GameFeatureAction)·캐릭터 4종·컨트롤러·어빌리티·MetaHuman 컴포넌트·MVVM 8종의 cpp 까지 통독했고, 도메인 플러그인 쪽은 호출 대상 함수만 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 `AWxNpc` 의 `MetaHumanComponent` 는 조립이 불가능한 데드 합성이다
- **위치**: `Source/WxGame/Character/WxNpc.cpp:37`, `Source/WxGame/Character/WxNpc.h:17`, `Source/WxGame/Character/WxMetaHumanComponent.cpp:36-41`
- **범주**: 중복/복잡도
- **문제**: `UWxMetaHumanComponent::OnRegister` 는 오너를 `Cast<ACharacter>` 해 `GetMesh()` 를 리더로 잡고, 실패하면 조용히 반환한다(`WxMetaHumanComponent.cpp:36-41`). `AWxNpc` 는 `AWxDialogueActor`(`AActor` 직상속)라 이 캐스트가 항상 실패하므로, 생성자에서 붙이는 `MetaHumanComponent`(`WxNpc.cpp:37`)는 어떤 슬롯을 채워도 바디·페이스·그룸을 만들지 않는다. 헤더 주석(`WxNpc.h:17`)은 "외형 컴포넌트를 얹는 합성"이 이 클래스의 역할이라고 적어 기획자가 BP_Npc 슬롯을 채우면 동작할 것처럼 읽힌다. 워크로그(2026-08-18)에 "현재 슬롯이 비어 무영향"으로 기록된 알려진 공백이지만, 코드에는 경고조차 없어 채우는 순간 무증상 실패가 된다.
- **제안**: 둘 중 하나를 고른다 — (a) `AWxNpc` 에서 `MetaHumanComponent` 와 관련 주석을 걷어낸다, (b) NPC 용 리더 메시 경로(예: 오너가 `ACharacter` 가 아니면 지정된 `USkeletalMeshComponent` 를 쓰는 슬롯)를 정해 조립이 성립하게 한다. 어느 쪽이든 `OnRegister` 에서 슬롯이 하나라도 채워졌는데 리더 메시를 못 찾으면 `UE_LOG(Error)` 로 드러낸다.
- **확신도**: 높음

### 2. 🟡 적 처치 보상이 항상 `PlayerController 0` 에게 간다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:91-93`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 권위에서 `UGameplayStatics::GetPlayerController(this, 0)` 를 보상 수령자로 쓴다. 리슨/데디케이티드 서버에서는 0번이 "호스트 또는 첫 접속자"이지 처치자가 아니므로, 다른 플레이어가 잡은 적의 재화·픽업이 엉뚱한 인벤토리로 들어간다. 주석("로컬 플레이어 인벤토리에 즉시 지급")은 싱글 가정을 명시하지만, 같은 클래스의 `HandleIncomingDamageChanged` 가 이미 GE 컨텍스트에서 가해자를 얻고 있어 처치자 식별 재료는 갖춰져 있다.
- **제안**: 사망을 일으킨 GE 의 `Instigator`(`HandleIncomingDamageChanged` 와 같은 경로로 마지막 가해자를 기억하거나, 사망 이벤트 페이로드에서 꺼냄)에서 컨트롤러를 역추적해 `GrantReward` 의 수령자로 넘긴다. 싱글 전용으로 확정한 설계라면 주석에 "MP 비대응" 을 명시하고 `GetNetMode() != NM_Standalone` 일 때 경고를 남긴다.
- **확신도**: 중간

### 3. 🟡 `InitAbilitySystem` 이 재진입에 멱등하지 않다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:192-207`, `Source/WxGame/Character/WxPlayerCharacter.cpp:64`, `Source/WxGame/Character/WxEnemyCharacter.cpp:46-53`
- **범주**: 버그/정확성
- **문제**: 호출될 때마다 (1) `BaseWalkSpeed = MaxWalkSpeed` 를 다시 잡고(`:194`), (2) SPD 변경 델리게이트를 `AddUObject` 로 또 붙이며(`:200`), (3) 권위면 `GiveAbilitySet()` 을 또 호출한다(`:205`, `UWxAbilitySystemComponent::GiveAbilitySet` 은 가드 없이 `GiveToAbilitySystem` 을 재실행하고 핸들을 덮어쓴다). 파생 `AWxEnemyCharacter::InitAbilitySystem` 도 대미지 구독을 중복 추가한다. 호출 진입점이 `PossessedBy`(서버)와 `OnRep_PlayerState`(클라) 두 곳이고, 클라 쪽은 언빙의로 `PlayerState` 가 널로 복제될 때도 불린다. 두 번 불리면 SPD 배율이 걸린 상태의 속도가 기준값이 되어 이후 배율이 복리로 곱해지고, 서버에선 어빌리티가 이중 부여된다. 현재 코드베이스에 같은 폰을 재빙의하는 호출은 없어 잠복 상태이지만, 리스폰·컨트롤러 스왑을 도입하는 순간 터진다. 부수적으로 바인딩 직후 현재 SPD 값을 1회 적용하지 않아, 바인딩 전에 이미 배율이 복제돼 있던 클라 폰은 다음 변경까지 기준 속도로 남는다.
- **제안**: `InitAbilitySystem` 을 1회 가드(이미 초기화된 ASC 면 `RefreshAbilityActorInfo` 만)하거나, `BaseWalkSpeed` 캡처·델리게이트 바인딩을 `PostInitializeComponents` 로 옮기고 이 함수에는 액터 인포 갱신·권위 부여만 남긴다. `GiveAbilitySet` 은 `AbilitySetGrantedHandles` 가 비어 있을 때만 부여하도록 WxCombat 쪽 가드를 함께 둔다. 바인딩 직후 현재 SPD 로 `MaxWalkSpeed` 를 한 번 동기화한다.
- **확신도**: 중간

### 4. 🟡 퀘스트 뷰모델 리졸버는 컴포넌트 늦은 도착을 처리하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp:68-82`
- **범주**: 설계/구조
- **문제**: `UWxViewModelResolver_Quest::CreateInstance` 는 생성 시점에 `GameState` 에서 `UWxQuestComponent` 를 못 찾으면 `nullptr` 를 돌려주고 끝이다(`:75`). 이 컴포넌트는 Experience 액션으로 주입되며 복제되지 않아 클라는 자기 로드 파이프라인이 끝나야 생기는데, HUD 는 빙의 시점에 푸시되고 서버는 서버 로드 완료만 기다려 폰을 스폰하므로, 클라 로드가 빙의보다 늦으면 그 클라의 퀘스트 HUD 는 세션 내내 비어 있다. 같은 모듈의 `UWxViewModel_Inventory`·`UWxViewModel_InteractionList` 는 정확히 이 이유로 `OnAny*Ready` 관찰 구조를 두고 있다(`WxViewModel_Inventory.cpp:12-29`). 헤더 주석은 "미등록 게임모드에선 null" 만 언급해 타이밍 문제를 가리고 있다.
- **제안**: `UWxViewModel_Quest` 를 `StartObserving(UWorld*)` 형태로 바꿔 인스턴스는 고정한 채 `UWxQuestComponent` 도착 신호(인벤토리처럼 클래스 델리게이트, 또는 `GameState` 의 컴포넌트 추가 관찰)로 `Initialize` 를 늦게 수행한다. 스탠드얼론 전용으로 둘 거라면 주석에 그 전제를 적는다.
- **확신도**: 중간

### 5. 🟢 엔진 `Reset()` 경로에서 시작 인벤토리가 재지급된다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:28-29`, `Source/WxGame/Framework/WxGameMode.cpp:105-115`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:77-80`
- **범주**: 버그/정확성
- **문제**: `SetCurrentExperience` 는 `AGameModeBase::Reset()` 이 `InitGameState` 를 재호출하는 경우를 멱등 처리한다고 명시하지만(`WxExperienceManagerComponent.h:47`, `.cpp:77-80`), 바로 다음 줄의 `CallOrRegister_OnExperienceLoaded`(`WxGameMode.cpp:29`) 는 이미 로드된 상태면 즉시 실행되어 `HandleExperienceLoaded` → 모든 접속자에게 `GrantDefaultInventory`(`:115`) 를 다시 돌린다. 인벤토리는 컨트롤러 소유라 `Reset` 에도 비워지지 않으므로 시작 아이템이 중복 지급된다. 현재 프로젝트에 `ResetLevel`/`RestartGame` 호출은 없어 잠복 상태다.
- **제안**: `InitGameState` 에서 `SetCurrentExperience` 가 실제로 새 Experience 를 설정했을 때만 델리게이트를 등록하거나(반환값 추가), `HandleExperienceLoaded` 의 지급을 "폰이 없던 접속자" 로 한정하지 말고 컨트롤러별 1회 플래그 대신 인벤토리가 비어 있을 때만 지급하도록 `GrantDefaultInventory` 에 가드를 둔다.
- **확신도**: 중간

### 6. 🟢 뷰모델의 `BlueprintCallable` 은 규칙의 허용 범위 밖이다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:39`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:51`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:54`, `Source/WxGame/MVVM/WxViewModel_Item.h:47`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:83`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 5항은 `BlueprintCallable` 을 Blueprint Function Library·Async Action 팩토리로 한정하고, 합의된 예외는 "위젯 서브클래스의 MVVM 바인딩 수신용 1-arg setter" 뿐이다. `RequestAdvance`·`RequestInteract`·`RequestCycle`·`RequestUseConsumable` 은 뷰모델의 명령 진입점이고, `SetCurrentCategory` 는 `BlueprintSetter` 대상이라 성격이 다르다. WxUI 의 `UWxViewModel_Ability` 에도 같은 용법이 있어 사실상 프로젝트 관례로 보이지만, 규칙 문서엔 반영돼 있지 않다.
- **제안**: 뷰모델의 "뷰 → 도메인 명령 전달용 `BlueprintCallable`" 을 CLAUDE.md 예외 목록에 명시하거나, 규칙을 지킬 거라면 이 명령들을 `UWxUILibrary` 류 파사드로 옮긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 7. 🟢 `UWxGameFeatureAction_AddComponents` 의 활성/비활성 override 가 `Super` 를 부르지 않는다
- **위치**: `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:50`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:67`
- **범주**: 규칙 위반
- **문제**: `OnGameFeatureActivating(FGameFeatureActivatingContext&)`·`OnGameFeatureDeactivating(FGameFeatureDeactivatingContext&)` 의 엔진 기본 구현은 인자 없는 `OnGameFeatureActivating()`/`OnGameFeatureDeactivating()` 훅으로 위임한다. `Super` 를 생략하면 그 훅이 영영 불리지 않는다. 현재 이 클래스의 파생도, 인자 없는 훅을 쓰는 코드도 없어 실동작 차이는 없다(Lyra 의 `WorldActionBase` 도 같은 형태).
- **제안**: 두 함수 첫 줄에 `Super::` 호출을 추가한다. 생략을 유지할 거라면 이유를 한 줄 주석으로 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 8. 🟢 공개 헤더가 Private 의존 모듈의 헤더를 포함한다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.h:7`, `Source/WxGame/WxGame.Build.cs:42-43`
- **범주**: 설계/구조
- **문제**: `MetaHumanSDKRuntime`·`HairStrandsCore` 는 `PrivateDependencyModuleNames` 인데, 모듈 전체가 공개 인클루드 경로(`PublicIncludePaths = ModuleDirectory`)라 `WxMetaHumanComponent.h` 가 `MetaHumanComponentUE.h` 를 공개적으로 끌어온다. WxGame 을 참조할 수 있는 GameFeature 플러그인이 이 헤더(또는 `AWxNpc`/`AWxCharacterBase` 의 구체 구현을 위해 이 헤더)를 포함하면 컴파일이 깨진다. 지금은 외부 포함처가 없어 잠복 상태다.
- **제안**: 두 모듈을 `PublicDependencyModuleNames` 로 올리거나, `UWxMetaHumanComponent` 를 내부 전용으로 두려면 `WXGAME_API` 를 떼고 Private 폴더 구조(`Public/`·`Private/`)를 도입해 노출 범위를 명시한다.
- **확신도**: 중간

### 9. 🟢 처형 프롬프트만 하드코딩 비지역화 문자열이다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:172`
- **범주**: 버그/정확성
- **문제**: `GetInteractionPrompt` 가 `FText::FromString(TEXT("Finisher"))` 를 돌려준다. 다른 `IWxInteractable` 구현(`AWxDevice`·`AWxTriggerDevice`·`AWxDialogueActor`)은 전부 에셋 편집 가능한 `FText` 필드를 반환해 기획자가 문구를 바꾸고 지역화할 수 있는데, 적 처형만 코드에 박혀 있다. 뒤잡/앞잡에 따라 문구를 달리할 여지도 막혀 있다.
- **제안**: `EditDefaultsOnly FText FinisherPrompt`(필요하면 `BackstabPrompt` 분리)를 두고, 기본값은 `NSLOCTEXT` 로 준다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp` (각 헤더 포함)
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`
- **미검토 / 한계**: 도메인 플러그인 쪽은 호출 대상(`UWxAbilitySystemComponent::GiveAbilitySet`·`GetAbilityInputActions`, `UWxViewModel_Character::Initialize`, `UWxQuestComponent` 복제 여부, `UWxSaveWorldSubsystem` 의 `OnWorldInitializedActors` 등록)만 교차 확인했고 그 내부 정확성은 보지 않았다. 심리스 트래블(`HandleSeamlessTravelPlayer`)이 Experience 로드 게이트를 우회하는지는 엔진 경로 추적이 필요해 판단을 보류했다(프로젝트에 심리스 트래블 사용 흔적 없음). BP/WBP 의 실제 슬롯·바인딩 구성은 범위 밖이다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 66파일 — `/module-review`로 갱신*
