# WxGame — 코드 리뷰

> 도메인 플러그인을 조립하는 게임 모듈로서 전반적으로 건강하다 — Lyra Experience 파이프라인 이식이 충실하고, 리플리케이션 권위·PIE 다중 세션·비동기 로드 순서 같은 함정이 주석과 가드로 대부분 눌려 있다. 이번 리뷰는 64개 소스 전부를 훑고 Experience 로드 파이프라인·캐릭터 계층·MVVM 뷰모델·GAS 응용 어빌리티·메타휴먼 조립·치트를 cpp 수준까지 내려가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 `InitAbilitySystem()` 이 비멱등이라 재호출 시 이동 속도가 복리로 누적된다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:192-207` (호출처: 같은 파일 `:107` `PossessedBy`, `Source/WxGame/Character/WxPlayerCharacter.cpp:64-69` `OnRep_PlayerState`)
- **범주**: 버그/정확성
- **문제**: `InitAbilitySystem()` 에 진입 가드가 없다. 두 지점이 재호출에 취약하다.
  1. `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;` (`:194`) — 그런데 `MaxWalkSpeed` 는 `HandleSPDAttributeChanged` 가 `BaseWalkSpeed * SPD` 로 계속 덮어쓰는 **파생값**이다(`:209-212`). 프로젝트 전체가 이 소유권을 전제로 한다(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52` 주석이 "MaxWalkSpeed 를 직접 쓰면 SPD 어트리뷰트 콜백과 주인이 겹친다"고 명시). 두 번째 호출은 이미 스케일된 값을 새 기준값으로 잡으므로, SPD ≠ 1 인 상태에서 재호출되면 속도가 `Base * SPD²` 로 튄다.
  2. `GetGameplayAttributeValueChangeDelegate(...).AddUObject(...)` (`:199-200`) 은 중복 바인딩을 허용하므로 호출 횟수만큼 콜백이 쌓인다.

  재호출 경로는 실재한다 — `APawn::OnRep_PlayerState` 는 PlayerState 포인터가 바뀔 때마다(빙의 해제로 null 이 되는 전이 포함) 클라에서 다시 발화하고, 서버에서도 폰 재빙의는 `PossessedBy` 를 다시 태운다. 덧붙여 `BaseWalkSpeed` 는 `WxCharacterBase.h:119` 에서 초기화 없이 선언돼 있어, 이 경로가 한 번도 돌지 않는 시뮬 프록시에서는 미초기화 값으로 남는다(현재는 콜백도 함께 미바인딩이라 읽히지 않는다).
- **제안**: `BaseWalkSpeed` 를 `EditDefaultsOnly` 기본값이나 생성자 시점 캡처로 옮겨 런타임 파생값을 다시 읽지 않게 하고, 델리게이트 등록·`GiveAbilitySet` 은 `bAbilitySystemInitialized` 같은 1회 가드로 감싼다.
- **확신도**: 중간

### 2. 🟡 처치 보상 수령자가 0번 플레이어로 하드코딩되어 있다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:57-61`
- **범주**: 설계/구조
- **문제**: `HandleDeath()` 는 서버 권위 가드를 통과한 뒤 `UGameplayStatics::GetPlayerController(this, 0)` 를 보상 대상으로 넘긴다. 데디케이티드 서버에서 0번은 "처치한 플레이어" 가 아니라 그냥 먼저 접속한 플레이어라, 멀티 세션에서는 모든 처치 보상이 한 명에게 몰린다. 이 모듈의 나머지(사망 태그 복제, `HasAuthority` 분기, 처형 자격의 주체별 판정)는 전부 다중 플레이어를 전제로 쓰여 있어 이 지점만 단일 플레이어 가정이다. 단, 동일 관행이 도메인 플러그인 전반에 퍼져 있어(`Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxStateTreeTask_GiveRewards.cpp:36`, `.../WxStateTreeTask_RefillItemCharges.cpp:32`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp:26`) 프로젝트 차원의 의도적 스코프일 가능성이 크다.
- **제안**: 멀티를 지원할 시점이 오면 킬 크레딧(마지막 대미지 instigator)을 `HandleDeath` 로 전달해 그 컨트롤러에 지급한다. 당장 고치지 않는다면 "0번 플레이어 = 단일 플레이어 전제" 를 이 호출부 주석에도 남겨 두는 편이 낫다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟡 메타휴먼 코스메틱 부착물이 데디케이티드 서버에서도 조립·등록된다
- **위치**: `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:19-33` (생성 본문 `:43-62`, `:92-146`)
- **범주**: 성능/안전
- **문제**: `OnRegister()` 의 유일한 넷/실행 모드 게이트는 `IsRunningCommandlet()` 뿐이다. 그 뒤로 만들어지는 것은 전부 순수 시각 요소다 — 페이스 `USkeletalMeshComponent`(+`FaceAnimClass` ABP), 최대 6개의 `UGroomComponent`, `ULODSyncComponent`, `UWxMetaHumanComponent`. 전부 `NoCollision` 이라 게임플레이 판정에 기여하지 않는데, 데디케이티드 서버의 캐릭터·NPC 마다 생성·등록된다. 엔진 `UGroomComponent` 에는 자체 데디케이티드 서버 가드가 없어(UE 5.8 `HairStrandsCore/Private/GroomComponent.cpp`) 여기서 걸러 주지 않으면 그대로 등록된다. 아웃핏은 `VisibilityBasedAnimTickOption = AlwaysTickPoseAndRefreshBones` (`:70`) 라 렌더되지 않아도 포즈 틱 대상이 된다(대부분 `SetLeaderPoseComponent` 로 비용이 눌리지만 PP-ABP 가 있는 메시는 그렇지 않다). 같은 모듈의 `StartExperienceLoad`(`Framework/WxExperienceManagerComponent.cpp:160-170`)는 `NM_DedicatedServer` 를 명시적으로 가르고 있어 서버 구성을 실제로 상정한 코드베이스다.
- **제안**: `OnRegister()` 초반의 커맨드릿 가드 옆에 `IsRunningDedicatedServer()`(또는 `GetNetMode() == NM_DedicatedServer`) 조기 반환을 추가한다. 서버가 특정 부착물의 본·소켓을 필요로 한다면 그 컴포넌트만 예외로 남긴다.
- **확신도**: 중간

### 4. 🟢 `SetCurrentCategory` 의 `BlueprintCallable` 은 `BlueprintSetter` 로 대체 가능하다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.h:93-94` (프로퍼티 선언 `:73-74`)
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5는 `BlueprintCallable` 을 Blueprint Function Library / Blueprint Async Action 팩토리로 한정한다. 이 함수는 `CurrentCategory` 의 `BlueprintSetter` 로 지정돼 있고 주석도 "탭 위젯이 BlueprintSetter 를 통해 갱신한다" 고 적고 있는데, 엔진 관용구는 `UFUNCTION(BlueprintSetter)` 단독이다(예: `Engine/Source/Runtime/Engine/Classes/Components/ArrowComponent.h:68`). 즉 여기서는 규칙을 어기지 않고도 같은 바인딩이 성립한다.
  참고로 나머지 4곳(`WxViewModel_Dialogue.h:39`, `WxViewModel_InteractionList.h:51,55`, `WxViewModel_Item.h:55`)은 뷰(WBP)의 MVVM Event 바인딩이 VM 커맨드를 부르는 유일한 통로라 대체 수단이 없고, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:40` 에도 같은 패턴이 있어 사실상 프로젝트 관행이다. 그쪽은 CLAUDE.md 에 예외를 명문화하는 편이 실효적이다.
- **제안**: `SetCurrentCategory` 를 `UFUNCTION(BlueprintSetter, Category = ...)` 로 바꾼다. 기존 WBP 그래프가 이 함수 노드를 직접 호출하고 있다면 프로퍼티 Set 노드로 갈아끼워야 하므로 함께 확인한다.
- **확신도**: 중간

### 5. 🟢 EnhancedInput 델리게이트에 바인딩되는 콜백에 `Handle` prefix 가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:52-57` (바인딩: `Source/WxGame/Character/WxPlayerCharacter.cpp:98,102,111,124,125`)
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 4는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `Move` `Look` `ToggleCrouch` `AbilityInputTriggered` `AbilityInputReleased` 는 모두 `EIC->BindAction(...)` 으로 델리게이트에 걸리는 콜백인데 prefix 가 없다. 같은 클래스의 다른 콜백(`HandleSPDAttributeChanged` 등)은 규칙을 지키고 있고, 프로젝트에서 `BindAction` 을 쓰는 곳은 이 파일이 유일해서 상반된 관행이 따로 있는 것도 아니다.
- **제안**: `HandleMoveInput` / `HandleLookInput` / `HandleCrouchInput` / `HandleAbilityInputTriggered` / `HandleAbilityInputReleased` 로 개명한다.
- **확신도**: 높음

### 6. 🟢 `IsAlive()` 가 널 체크보다 먼저 ASC 를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:183-190`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:80`
- **범주**: 성능/안전
- **문제**: `AWxCharacterBase::IsAlive()` 는 `AbilitySystemComponent->GetSet<...>()` 를 널 체크 없이 부른다. 반면 같은 클래스의 `GetOwnedGameplayTags`(`:142-148`)·`CanJumpInternal_Implementation`(`:112`) 은 널을 검사하고, 호출부인 `GetEligibleFinisherEventTag` 는 `if (!IsAlive() || !AbilitySystemComponent)` 로 **역참조 뒤에** 널을 검사한다. 가드의 순서가 뒤집혀 있어 의도가 성립하지 않는다. ASC 가 생성자 서브오브젝트라 실제로 널이 될 일은 없으므로 지금 크래시하지는 않지만, 어느 쪽이 진실인지 코드가 서로 모순된다.
- **제안**: ASC 가 항상 유효하다는 전제를 택해 `GetEligibleFinisherEventTag` 의 `!AbilitySystemComponent` 를 지우거나, 반대로 `IsAlive()` 안에서 먼저 널을 검사한다. 둘 중 하나로 통일한다.
- **확신도**: 중간

### 7. 🟢 Experience 미확정이 Warning 하나만 남기고 폰 없는 상태로 진행된다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:82-86`, 게이트 `Source/WxGame/Framework/WxGameMode.cpp:55-65`
- **범주**: 버그/정확성
- **문제**: `SetCurrentExperience` 에 무효 ID 가 들어오면 `Warning` 로그만 남기고 반환한다. 이 경우 `LoadState` 는 영영 `Unloaded` 이고 `IsExperienceLoaded()` 가 계속 false 라, `HandleStartingNewPlayer_Implementation` 이 매번 조기 반환해 **폰이 한 번도 스폰되지 않는다**(`OnExperienceLoaded` 도 브로드캐스트되지 않으므로 나중에 회복될 경로가 없다). 같은 파일 `:92` 의 "Experience 에셋으로 해석 실패" 와 `WxGameMode.cpp:49` 의 "폰을 스폰할 수 없다" 는 모두 `Error` 인데, 결과가 동일하게 치명적인 이 경로만 `Warning` 이라 로그 필터에 묻히기 쉽다.
- **제안**: `Error` 로 올리고, 메시지에 "폰이 스폰되지 않는다" 는 귀결까지 적어 원인 추적을 한 줄로 끝내게 한다.
- **확신도**: 중간

### 8. 🟢 스태미나 뷰모델이 원격 플레이어 캐릭터에도 생성된다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:144-165` (가시성 게이트는 `:57-62` `NotifyControllerChanged`)
- **범주**: 성능/안전
- **문제**: `BeginPlay` 는 `StaminaWidget->GetWidget()` 이 유효하기만 하면 로컬 조종 여부와 무관하게 `UWxViewModel_Attribute` 를 만들어 뷰에 꽂는다. `UWidgetComponent::InitWidget` 은 데디케이티드 서버만 걸러낼 뿐 가시성은 보지 않으므로, 클라이언트에서는 화면에 절대 뜨지 않는 원격 플레이어 캐릭터마다 위젯·뷰모델·어트리뷰트 변경 델리게이트 2건이 함께 생긴다. 가시성만 `IsLocallyControlled()` 로 꺼지고 있어(`:61`) 비용은 그대로 남는다.
- **제안**: 뷰모델 주입을 `BeginPlay` 가 아니라 `NotifyControllerChanged` 에서 `IsLocallyControlled()` 가 참일 때 1회만 수행한다(빙의가 `BeginPlay` 보다 늦게 도착하는 클라 경로도 이쪽이 자연스럽다).
- **확신도**: 중간

### 9. 🟢 획득 토스트 VM 이 교체될 때 이전 VM 을 `Deinitialize` 하지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-113`
- **범주**: 중복/복잡도
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 새 `UWxViewModel_Item` 을 만들어 `Initialize` 하고 `LastAcquiredItem` 을 교체한다. `UWxViewModel_Item::Initialize` 는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 에 구독하는데(`WxViewModel_Item.cpp:45-46`), 밀려난 이전 VM 은 `Deinitialize` 되지 않아 GC 가 돌기 전까지 구독이 남는다. 그 사이 획득 1회당 델리게이트가 2개씩 늘고, 충전 변경마다 죽은 VM 들이 `RefreshChargeIcon()` → `RequestImageAsync` 로 아이콘 비동기 로드를 각자 요청한다. 같은 파일의 `RefreshAllItems`(`:152-158`)는 목록에서 빠진 VM 을 명시적으로 `Deinitialize` 하고 있어, 이 경로만 클래스 자신의 관행을 벗어난다. (베이스 `UWxViewModel::BeginDestroy` 가 `Deinitialize` 를 부르므로 GC 시점에 자동 회수되긴 한다.)
- **제안**: 새 VM 을 만들기 전에 기존 `LastAcquiredItem` 이 있으면 `Deinitialize()` 를 부른다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h/.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Input/WxInputConfig.*`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.*`, `Source/WxGame/MVVM/WxViewModel_Dialogue.*`, `Source/WxGame/MVVM/WxViewModel_Quest.*`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.*`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.*`
- **확인했고 문제가 없던 지점**(재조사 방지용): `override` 의 `Super::` 호출 누락 없음(의도적 미호출 2건은 `WxGameMode.cpp:45,57` 에 근거 주석 있음), `FORCEINLINE`·인라인 정의 없음, 람다 없음, 전 파일 첫 줄 Copyright 존재, MVVM 뷰모델의 `Deinitialize` → `Super::Deinitialize()` 체인 정상, `RequestImageAsync` 가 같은 필드의 이전 요청을 취소하므로 `WxViewModel_Item` 의 def 아이콘/충전 아이콘 이중 요청에 경합 없음, 브로드캐스트 도중의 델리게이트 Add/Remove(`WxViewModel_Inventory.cpp:108,199`)는 엔진 멀티캐스트가 역순 순회 + 락 지연 컴팩션으로 보장하므로 안전, `UAbilitySystemComponent::InitializeComponent` 가 `InitAbilityActorInfo(Owner, Owner)` 를 대신 수행하므로(UE 5.8 `AbilitySystemComponent_Abilities.cpp:84`) 클라 AI 폰의 ASC ActorInfo 미초기화 문제 없음, `GetAbilityInputActions()` 는 granted spec 이 아니라 `AbilitySet` 에셋에서 읽으므로 클라의 어빌리티 복제 타이밍과 무관하게 입력이 바인딩됨, `UWxExperienceManager::RequestToDeactivatePlugin` 의 `FindChecked` 는 URL 수집과 `NotifyOfPluginActivation` 이 같은 루프에 있어 키 부재로 터지지 않음.
- **미검토 / 한계**: BP/WBP 내부 구조(위젯 계층·MVVM 바인딩 그래프·`ChildActorClass`/`WidgetClass` 등 BP 디폴트 지정)는 범위 밖이라 보지 않았다. `UWxMetaHumanVisualComponent` 의 LOD 매핑 수치(`WxMetaHumanVisualComponent.cpp:99-127`)는 메타휴먼 에셋의 실제 LOD 구성 없이는 검증할 수 없어 로직 형태만 확인했다. Experience 로드 파이프라인의 teardown 경합(로드 중 EndPlay 로 `FinishExperienceLoad` 가 월드 컨텍스트 없이 액션을 활성화하는 경로)은 Lyra 원본과 동일한 구조이고 재현 조건을 특정하지 못해 별도 발견으로 올리지 않았다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 64파일 — `/module-review`로 갱신*
