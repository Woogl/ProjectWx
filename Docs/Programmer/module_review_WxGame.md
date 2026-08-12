# WxGame — 코드 리뷰

> 도메인 플러그인을 조립하는 게임 모듈로서 전반적으로 건강하다 — Lyra Experience 파이프라인 이식이 충실하고, 리플리케이션 권위·PIE 다중 세션·비동기 로드 순서 같은 함정이 주석과 가드로 대부분 눌려 있다. 이번 리뷰는 64개 소스 전부를 훑고, Experience 로드 파이프라인·캐릭터 계층·MVVM 뷰모델·어빌리티·치트를 cpp 수준까지 내려가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 `InitAbilitySystem()` 이 비멱등이라 재호출 시 이동 속도가 복리로 누적된다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:185-200` (호출처: 같은 파일 `:107` `PossessedBy`, `Source/WxGame/Character/WxPlayerCharacter.cpp:64-69` `OnRep_PlayerState`)
- **범주**: 버그/정확성
- **문제**: `InitAbilitySystem()` 은 진입 가드가 없다. 두 가지가 재호출에 취약하다.
  1. `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;` (`:187`) — 그런데 `MaxWalkSpeed` 는 `HandleSPDAttributeChanged` 가 `BaseWalkSpeed * SPD` 로 계속 덮어쓰는 **파생값**이다(`:204`). 프로젝트 전체가 이 소유권을 전제로 하고 있다(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:54` 주석이 "MaxWalkSpeed 를 직접 쓰면 SPD 어트리뷰트 콜백과 주인이 겹친다"고 명시). 두 번째 호출은 이미 스케일된 값을 새 기준값으로 잡으므로 SPD ≠ 1 인 상태에서 재호출되면 속도가 `Base * SPD²` 로 튄다.
  2. `GetGameplayAttributeValueChangeDelegate(...).AddUObject(...)` (`:192-193`) 은 중복 바인딩을 허용하므로 호출 횟수만큼 콜백이 쌓인다.

  재호출 경로는 실재한다 — `APawn::OnRep_PlayerState` 는 PlayerState 포인터가 바뀔 때마다(빙의 해제로 null 이 되는 전이 포함) 클라에서 다시 발화하고, 서버에서도 폰 재빙의는 `PossessedBy` 를 다시 태운다.
- **제안**: `BaseWalkSpeed` 를 `EditDefaultsOnly` 기본값이나 생성자 시점 캡처로 옮겨 런타임 파생값을 다시 읽지 않게 하고, 델리게이트 등록·`GiveAbilitySet` 은 `bAbilitySystemInitialized` 같은 1회 가드로 감싼다.
- **확신도**: 중간

### 2. 🟡 처치 보상 수령자가 0번 플레이어로 하드코딩되어 있다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:59-63`
- **범주**: 설계/구조
- **문제**: `HandleDeath()` 는 서버 권위 가드를 통과한 뒤 `UGameplayStatics::GetPlayerController(this, 0)` 를 보상 대상으로 넘긴다. 데디케이티드 서버에선 0번이 "처치한 플레이어" 가 아니라 그냥 먼저 접속한 플레이어라, 멀티 세션에서는 모든 처치 보상이 한 명에게 몰린다. 이 모듈의 나머지(사망 태그 복제, `HasAuthority` 분기, 처형 자격의 주체별 판정)는 전부 다중 플레이어를 전제로 쓰여 있어 이 지점만 단일 플레이어 가정이다. 단, 동일 관행이 `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxRewardStateTreeNodes.cpp:36` 에도 있고 `WxRewardStateTreeNodes.h:38` 에 "로컬 플레이어(0번 컨트롤러)" 로 명문화돼 있어, 프로젝트 차원의 의도적 스코프일 가능성이 있다.
- **제안**: 멀티를 지원할 시점이 오면 킬 크레딧(마지막 대미지 instigator)을 `HandleDeath` 로 전달해 그 컨트롤러에 지급한다. 당장 고치지 않는다면 "0번 플레이어 = 단일 플레이어 전제" 를 이 호출부 주석에도 남겨 두는 편이 낫다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `SetCurrentCategory` 의 `BlueprintCallable` 은 `BlueprintSetter` 로 대체 가능하다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.h:102-103` (프로퍼티 선언 `:81-82`)
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5는 `BlueprintCallable` 을 Blueprint Function Library / Blueprint Async Action 팩토리로 한정한다. 이 함수는 `CurrentCategory` 의 `BlueprintSetter` 로 지정돼 있고 주석도 "탭 위젯이 BlueprintSetter 를 통해 갱신한다" 고 적고 있는데, 엔진 관용구는 `UFUNCTION(BlueprintSetter)` 단독이다(예: `Engine/Source/Runtime/Engine/Classes/Components/ArrowComponent.h:68`). 즉 여기서는 규칙을 어기지 않고도 같은 바인딩이 성립한다.
  참고로 나머지 4곳(`WxViewModel_Dialogue.h:39`, `WxViewModel_InteractionList.h:52,56`, `WxViewModel_Item.h:59`)은 뷰(WBP)의 MVVM Event 바인딩이 VM 커맨드를 부르는 유일한 통로라 대체 수단이 없고, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:43` 에도 같은 패턴이 있어 사실상 프로젝트 관행이다. 그쪽은 CLAUDE.md 에 예외를 명문화하는 편이 실효적이다.
- **제안**: `SetCurrentCategory` 를 `UFUNCTION(BlueprintSetter, Category = ...)` 로 바꾼다. 기존 WBP 그래프가 이 함수 노드를 직접 호출하고 있다면 프로퍼티 Set 노드로 갈아끼워야 하므로 함께 확인한다.
- **확신도**: 중간

### 4. 🟢 EnhancedInput 델리게이트에 바인딩되는 콜백에 `Handle` prefix 가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:51-56` (바인딩: 같은 폴더 `WxPlayerCharacter.cpp:98,102,111,124,125`)
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 4는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `Move` `Look` `ToggleCrouch` `AbilityInputTriggered` `AbilityInputReleased` 는 모두 `EIC->BindAction(...)` 으로 델리게이트에 걸리는 콜백인데 prefix 가 없다. 프로젝트에서 `BindAction` 을 쓰는 곳은 이 파일이 유일해서 상반된 관행이 따로 있는 것도 아니다.
- **제안**: `HandleMoveInput` / `HandleLookInput` / `HandleCrouchInput` / `HandleAbilityInputTriggered` / `HandleAbilityInputReleased` 로 개명한다.
- **확신도**: 높음

### 5. 🟢 `IsAlive()` 가 널 체크보다 먼저 ASC 를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:176-183`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:82`
- **범주**: 성능/안전
- **문제**: `AWxCharacterBase::IsAlive()` 는 `AbilitySystemComponent->GetSet<...>()` 를 널 체크 없이 부른다. 반면 같은 클래스의 `GetOwnedGameplayTags`(`:137`)·`CanJumpInternal_Implementation`(`:112`) 은 널을 검사하고, 호출부인 `GetEligibleFinisherEventTag` 는 `if (!IsAlive() || !AbilitySystemComponent)` 로 **역참조 뒤에** 널을 검사한다. 가드의 순서가 뒤집혀 있어 의도가 성립하지 않는다. ASC 가 생성자 서브오브젝트라 실제로 널이 될 일은 없으므로 지금 크래시하지는 않지만, 어느 쪽이 진실인지 코드가 서로 모순된다.
- **제안**: ASC 가 항상 유효하다는 전제를 택해 `GetEligibleFinisherEventTag` 의 `!AbilitySystemComponent` 를 지우거나, 반대로 `IsAlive()` 안에서 먼저 널을 검사한다. 둘 중 하나로 통일한다.
- **확신도**: 중간

### 6. 🟢 Experience 미확정이 Warning 하나만 남기고 폰 없는 상태로 진행된다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:82-86`, 게이트 `Source/WxGame/Framework/WxGameMode.cpp:56-65`
- **범주**: 버그/정확성
- **문제**: `SetCurrentExperience` 에 무효 ID 가 들어오면 `Warning` 로그만 남기고 반환한다. 이 경우 `LoadState` 는 영영 `Unloaded` 이고 `IsExperienceLoaded()` 가 계속 false 라, `HandleStartingNewPlayer_Implementation` 이 매번 조기 반환해 **폰이 한 번도 스폰되지 않는다**(`OnExperienceLoaded` 도 브로드캐스트되지 않으므로 나중에 회복될 경로가 없다). 같은 파일 `:92` 의 "Experience 에셋으로 해석 실패" 와 `WxGameMode.cpp:49` 의 "폰을 스폰할 수 없다" 는 모두 `Error` 인데, 결과가 동일하게 치명적인 이 경로만 `Warning` 이라 로그 필터에 묻히기 쉽다.
- **제안**: `Error` 로 올리고, 메시지에 "폰이 스폰되지 않는다" 는 귀결까지 적어 원인 추적을 한 줄로 끝내게 한다.
- **확신도**: 중간

### 7. 🟢 스태미나 뷰모델이 원격 플레이어 캐릭터에도 생성된다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:136-157` (가시성 게이트는 `:57-62` `NotifyControllerChanged`)
- **범주**: 성능/안전
- **문제**: `BeginPlay` 는 `StaminaWidget->GetWidget()` 이 유효하기만 하면 로컬 조종 여부와 무관하게 `UWxViewModel_Attribute` 를 만들어 뷰에 꽂는다. `UWidgetComponent::InitWidget` 은 데디케이티드 서버만 걸러낼 뿐 가시성은 보지 않으므로, 클라이언트에서는 화면에 절대 뜨지 않는 원격 플레이어 캐릭터마다 위젯·뷰모델·어트리뷰트 변경 델리게이트 2건이 함께 생긴다. 가시성만 `IsLocallyControlled()` 로 꺼지고 있어(`:61`) 비용은 그대로 남는다.
- **제안**: 뷰모델 주입을 `BeginPlay` 가 아니라 `NotifyControllerChanged` 에서 `IsLocallyControlled()` 가 참일 때 1회만 수행한다(빙의가 `BeginPlay` 보다 늦게 도착하는 클라 경로도 이쪽이 자연스럽다).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Controller/WxEnemyController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Input/WxInputConfig.*`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.*`, `Source/WxGame/MVVM/WxViewModel_Dialogue.*`, `Source/WxGame/MVVM/WxViewModel_Quest.*`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.*`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.*`
- **확인했고 문제가 없던 지점**(재조사 방지용): `override` 의 `Super::` 호출 누락 없음(의도적 미호출 2건은 `WxGameMode.cpp:45,57` 에 근거 주석 있음), `FORCEINLINE`·인라인 정의 없음, 람다 없음, 전 파일 첫 줄 Copyright 존재, MVVM 뷰모델의 `Deinitialize` → `Super::Deinitialize()` 체인 정상, `RequestImageAsync` 가 같은 필드의 이전 요청을 취소하므로 `WxViewModel_Item` 의 def 아이콘/충전 아이콘 이중 요청에 경합 없음, 브로드캐스트 도중의 델리게이트 Add/Remove(`WxViewModel_Inventory.cpp:108,199`)는 엔진 멀티캐스트가 역순 순회 + 락 지연 컴팩션으로 보장하므로 안전, `UAbilitySystemComponent::InitializeComponent` 가 `InitAbilityActorInfo` 를 대신 수행하므로 클라 AI 폰의 ASC ActorInfo 미초기화 문제 없음.
- **미검토 / 한계**: BP/WBP 내부 구조(위젯 계층·MVVM 바인딩 그래프·`ChildActorClass`/`WidgetClass` 등 BP 디폴트 지정)는 범위 밖이라 보지 않았다. `UWxMetaHumanVisualComponent` 의 LOD 매핑 수치(`WxMetaHumanVisualComponent.cpp:99-127`)는 메타휴먼 에셋의 실제 LOD 구성 없이는 검증할 수 없어 로직 형태만 확인했다. Experience 로드 파이프라인의 teardown 경합(로드 중 EndPlay)은 Lyra 원본과 동일한 구조라 별도 발견으로 올리지 않았다.

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 64파일 — `/module-review`로 갱신*
