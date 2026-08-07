# WxGame — 코드 리뷰

> 조립 모듈답게 파일마다 책임이 얇고, Experience 로드 파이프라인·권위 게이트·MVVM 글루의 의도가 주석으로 성실히 문서화돼 있어 전반적으로 건강하다. 직전 리뷰(`95a57ef3`) 이후 4건이 실제로 고쳐졌고(더블 점프 주석 정리, `RefreshAllItems` 의 중복 `Retained` 배열 제거, `HandleDeath` 오버라이드 접근 지정자 정렬, `WxAbility_UseItem` 의 `CommitAbility` 순서 역전), 남은 6건은 코드 변경 없이 그대로다. 이번 리뷰는 `Source/WxGame` 62개 소스(.h/.cpp) 전부를 열고 프레임워크·캐릭터·어빌리티·MVVM 의 cpp 로직까지 내려가 확인했으며, 근거 확보를 위해 WxCombat·WxInventory·WxSave·WxUI 의 호출 대상도 교차 확인했다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 처치 보상이 항상 0번 플레이어에게 지급된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:63`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 `UGameplayStatics::GetPlayerController(this, 0)` 을 `UWxRewardLibrary::GrantReward` 의 `DirectGrantTarget` 으로 넘긴다(`:65`). 그 인자는 Pickup Fragment 가 없는 보상(골드 등 재화)을 곧바로 넣을 인벤토리 대상이라고 문서화돼 있으므로(`Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h:29`), 실제 처치자와 무관하게 서버의 0번 플레이어가 모든 재화를 가져간다. 모듈 전반이 서버 권위·소유 클라 복제를 세심히 지키는 것에 비해 여기만 싱글플레이 가정이 남아 있다. (5회 연속 지적, 미해결.)
- **제안**: 사망을 유발한 주체를 보관해 넘긴다. 대미지 경로의 마지막 instigator 를 `AWxCharacterBase` 에 기록하거나, 처형 경로가 이미 다루는 Interactor(`:123` `OnInteracted`)를 재사용해 그 컨트롤러를 대상으로 쓴다. 당분간 싱글플레이만 지원한다면 그 전제를 주석으로 못박아 두는 편이 낫다.
- **확신도**: 중간(의도된 단순화일 수 있음)

### 2. 🟡 `UpdateCharacterStateBeforeMovement` 가 `Spec.Ability` 를 널 검사 없이 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterMovementComponent.cpp:31`
- **범주**: 성능/안전
- **문제**: `if (Spec.IsActive() && !Spec.Ability->IsA<UWxAbility_LockOn>())` 가 `Spec.Ability` 를 무조건 역참조한다. `FGameplayAbilitySpec::Ability` 는 복제 대상이라 클라에서 스펙 배열이 먼저 도착하고 어빌리티 CDO 참조가 아직 매핑되지 않은 순간에는 널일 수 있으며(엔진 GAS 코드가 곳곳에서 `if (Spec.Ability)` 로 방어하는 이유), 이 코드는 웅크리기 의사가 있을 때 매 이동 틱 돌기 때문에(`:28` 루프) 그 창에 겹치면 이동 업데이트 중 크래시가 난다. 같은 파일의 다른 소비자(`WxViewModelResolver_PlayerCharacter.cpp:39`)는 `Cast<UWxAbilityBase>(Spec.Ability)` 로 널을 흡수하고 있어 모듈 안에서도 방어 수준이 갈린다. 함께 볼 점으로, 판정이 "락온 외 활성 어빌리티가 하나라도 있는가"뿐인데 매 틱 활성화 가능 어빌리티 전체를 선형 순회한다. (5회 연속 지적, 미해결.)
- **제안**: 루프 조건에 `Spec.Ability &&` 를 추가한다. 순회 비용이 신경 쓰이면 ASC 의 블로킹 태그(`AreAbilityTagsBlocked(Ability)` — `AWxCharacterBase::CanJumpInternal_Implementation` 이 이미 쓰는 방식, `Source/WxGame/Character/WxCharacterBase.cpp:138`)로 같은 판정을 상수 시간에 대체할 수 있는지 검토한다.
- **확신도**: 중간(널 창이 짧아 재현은 드묾)

### 3. 🟡 `InitAbilitySystem` 이 멱등하지 않아 재호출 시 이동속도·어트리뷰트·어빌리티가 어긋난다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:202`
- **범주**: 버그/정확성
- **문제**: 이 함수는 (a) `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed`(`:204`)로 기준 속도를 캡처하고 (b) SPD 어트리뷰트 변경에 `AddUObject` 로 구독하며(`:209`) (c) 권위에서 `GiveAbilitySet()`(`:215`)을 호출하는데, 셋 다 중복 방지가 없다. 두 번째 호출이 생기면 이미 SPD 배율이 곱해진 `MaxWalkSpeed` 를 새 기준값으로 캡처해 속도가 복리로 커지고(500 → SPD 1.2 → 600 을 기준으로 다시 720), SPD 콜백이 두 번 등록되며, `UWxAbilitySystemComponent::GiveAbilitySet` 은 기존 핸들을 비우지 않고 덧붙이므로(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:23`) 어빌리티 스펙이 중복 부여되고 `UWxAbilitySet::GiveToAbilitySystem`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:36`)이 `SetNumericAttributeBase` 로 HP·SP 등 어트리뷰트를 테이블 초기값으로 되돌리고 `GrantedEffects` 를 다시 적용한다.
  진입점이 서버 `PossessedBy`(`:123`)와 클라 `OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:51`) 둘인데, 후자는 **`PlayerState` 가 널로 복제되는 언포제스 시에도 조건 없이 호출된다** — 이 경로가 열리는 순간 클라에서 `BaseWalkSpeed` 가 곧바로 오염된다. 다만 현 저장소에는 폰을 인플레이스로 언포제스·재빙의하는 호출자가 없고(사망 부활은 맵 리로드 경유, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:95`) 실제 증상은 나지 않는다. 부수적으로 `BaseWalkSpeed` 는 헤더에서 초기화되지 않는다(`Source/WxGame/Character/WxCharacterBase.h:133`). (5회 연속 지적, 미해결.)
- **제안**: `BaseWalkSpeed` 를 헤더에서 0 초기화하고 캡처는 생성자/`PostInitializeComponents` 에서 1회만 한다. `InitAbilitySystem` 에는 재진입 가드를 두거나 재등록 전 `RemoveAll(this)` 로 대칭을 맞추고, `OnRep_PlayerState` 는 `GetPlayerState()` 유효 시에만 호출하도록 좁힌다.
- **확신도**: 중간(정상 흐름에선 1회만 호출되므로 잠재 결함)

### 4. 🟢 획득 알림 VM(`LastAcquiredItem`)이 교체·해제 시 `Deinitialize` 되지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:108`, 해제부 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:58`
- **범주**: 중복/복잡도
- **문제**: `HandleStackChanged` 는 `Delta > 0` 마다 Def 모드 `UWxViewModel_Item` 을 새로 만들어 `LastAcquiredItem` 에 대입하는데(`:108-111`), 그 Def 모드 `Initialize` 는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 에 바인딩한다(`Source/WxGame/MVVM/WxViewModel_Item.cpp:45-46`). 교체 시에도, `Deinitialize` 시에도(`:58` 단순 nullptr 대입) 이전 VM 에 `Deinitialize()` 를 부르지 않는다. 같은 함수의 `AllItems` 경로는 미보존 VM 을 일일이 `Deinitialize` 하며 대칭을 지키는데(`:152-158`) 획득 VM 만 예외다. `UWxViewModel::BeginDestroy` 가 `Deinitialize` 를 부르고(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:8-12`) 델리게이트도 약참조라 영구 누수는 아니지만, GC 전까지 획득 횟수만큼 죽은 구독자가 쌓여 매 스택 변경 브로드캐스트가 그만큼 헛돈다. (5회 연속 지적, 미해결.)
- **제안**: 교체 직전과 `Deinitialize` 시 이전 `LastAcquiredItem` 에 `Deinitialize()` 를 호출해 `AllItems` 경로와 대칭을 맞춘다.
- **확신도**: 중간

### 5. 🟢 `IsAlive()` 가 널 검사보다 먼저 `AbilitySystemComponent` 를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:195`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:85`
- **범주**: 성능/안전
- **문제**: `IsAlive()` 는 `AbilitySystemComponent->GetSet<...>()` 를 널 검사 없이 역참조하는데, 같은 클래스의 `GetOwnedGameplayTags`(`:154`)·`CanJumpInternal_Implementation`(`:128`)은 `if (AbilitySystemComponent)` 로 방어한다. 게다가 `AWxEnemyCharacter::GetEligibleFinisherEventTag` 의 가드가 `if (!IsAlive() || !AbilitySystemComponent)` 순서라, ASC 가 널이면 `!AbilitySystemComponent` 검사에 도달하기 전에 `IsAlive()` 안에서 이미 역참조한다. 실제로 ASC 는 생성자에서 항상 만들어져(`:33`) 널이 되지 않으므로 크래시로 이어지진 않지만, 방어 의도와 실제 동작이 어긋나 읽는 사람을 오도한다. (5회 연속 지적, 미해결.)
- **제안**: `IsAlive()` 에 널 가드를 추가하거나, 반대로 ASC 가 항상 유효하다는 전제로 통일해 불필요한 널 검사들을 정리한다.
- **확신도**: 낮음(현재 경로에서 ASC 는 항상 유효 — 의도된 단순화일 수 있음)

### 6. 🟢 규칙 4 위반 가능 — 입력 바인딩 콜백에 `Handle` prefix 없음
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:43-49`, 바인딩부 `Source/WxGame/Character/WxPlayerCharacter.cpp:81`·`:85`·`:94`·`:109-111`
- **범주**: 규칙 위반
- **문제**: `Move`/`Look`/`ToggleCrouch`/`AbilityInputStarted`/`AbilityInputTriggered`/`AbilityInputReleased` 는 모두 `UEnhancedInputComponent::BindAction` 으로 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 가 없다. 같은 클래스 계층의 다른 델리게이트 콜백(`HandleSPDAttributeChanged`, `HandleEquipVisualChanged`, `HandleMontageCompleted` 등)은 규칙을 지키고 있어 모듈 안에서도 일관되지 않다. 저장소에서 `BindAction` 을 쓰는 곳은 이 파일뿐이라 비교할 선례가 없다. (5회 연속 지적, 미해결.)
- **제안**: 규칙을 문자 그대로 적용하면 `HandleMoveInput` 등으로 개명한다. 입력 핸들러를 규칙 4 대상에서 뺄 생각이면 `CLAUDE.md` 에 예외를 적어 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`·`.h`, `Source/WxGame/Framework/WxGameMode.cpp`·`.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceManager.cpp`·`.h`, `Source/WxGame/Character/WxCharacterBase.cpp`·`.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`·`.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`·`.h`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`·`.h`, `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp`·`.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`·`.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`·`.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`·`.h`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`·`.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`·`.h`, `Source/WxGame/Framework/WxGameState.cpp`·`.h`, `Source/WxGame/Controller/WxPlayerController.cpp`·`.h`, `Source/WxGame/Player/WxPlayerState.cpp`·`.h`, `Source/WxGame/Character/WxBossCharacter.cpp`·`.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`·`.h`, `Source/WxGame/Input/WxInputConfig.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h`
- **교차 확인(모듈 밖, 근거 확보용)**: `Plugins/WxCombat` 의 `UWxAbilitySystemComponent::GiveAbilitySet`·`UWxAbilitySet::GiveToAbilitySystem`·`GetInputActions`(3번 근거), `Plugins/WxInventory` 의 `UWxRewardLibrary::GrantReward` 시그니처와 `DirectGrantTarget` 의미(1번 근거), `Plugins/WxUI` 의 `UWxViewModel::BeginDestroy`/`Deinitialize` 와 `UWxViewModel_Character::Initialize`(4번 근거), `Plugins/WxSave/WxPlayerSpawnComponent`(3번의 빙의 순서 확인 — `PossessedBy` → `InitAbilitySystem` 이 `SetPawn` 브로드캐스트의 `ApplySavedPlayerStats` 보다 먼저라 저장 스탯이 어트리뷰트 초기화에 덮이지 않음을 확인), `WxAIPerceptionComponent`·`WxAbility_Finisher`·`WxCombatAttributeSet`(처형 자격 판정이 읽는 `State.InCombat`·`State.Finisher`·`State.Groggy` 가 모두 복제 대상 API 로 부여됨을 확인 — 클라 표시 게이트가 성립), 저장소 전역 규칙 스캔(첫 줄 저작권 표기 62/62 통과, `FORCEINLINE`·인라인 정의 0건, 람다 0건, `BlueprintCallable` 5건은 전부 뷰모델 커맨드)
- **미검토 / 한계**: (1) 정적 분석만 수행했다 — 2·3번은 코드 경로로만 확인했고 원격 클라 세션 재현은 하지 않았다. (2) 규칙 5(`BlueprintCallable`)는 뷰모델 커맨드(`RequestAdvance`/`RequestInteract`/`RequestCycle`/`RequestUseConsumable`/`SetCurrentCategory`, 5건)를 예외로 확정한 이전 결정이 있어 제외했다. (3) `UWxGameFeatureAction_AddComponents::FContextHandles`(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h:65`)는 `Wx` prefix 없는 `F` 타입이지만, USTRUCT 이 아닌 클래스 내부 private 구현 구조체라 규칙 1 위반으로 올리지 않았다 — 판단이 다르면 개명 대상이다. (4) `WxResolveReceiverClass`(`.cpp:29`)·`WxHandleDeactivationPauserCompleted`(`WxExperienceManagerComponent.cpp:18`)의 파일 static 자유 함수는 `CLAUDE.md` 명시 규칙 대상이 아니라 제외했다. (5) `WxResolveReceiverClass` 가 `UPawnComponent` 파생을 `APawn::StaticClass()` 로 매핑하는 탓에, 폰 대상 주입 컴포넌트는 receiver 로 opt-in 한 모든 캐릭터(에너미 포함)에 붙는다 — README 가 명시한 의도된 설계이고 현재 저장소에 `UPawnComponent` 파생이 하나도 없어 발현하지 않으므로 발견으로 올리지 않았다. (6) `UWxExperienceManagerComponent::EndPlay` 가 `Loaded` 이전 상태(로딩 중 월드 종료)에서 액션 비활성을 건너뛰는 비대칭과, `UWxMetaHumanVisualComponent` 가 넷모드 가드 없이(커맨드릿 가드만 존재, `:24`) 부착물을 조립하는 점은 각각 Lyra 원본 동일·서버 타겟 부재로 실효가 없어 제외했다. (7) BP/WBP 디폴트값(`InputConfig`·`RewardRow`·`BehaviorTreeAsset`·메타휴먼 슬롯·Experience 에셋 실제 내용)은 범위 밖이라, 에셋 미지정으로만 가려져 있는 경로가 남아 있을 수 있다.

---
*문서 기준 커밋 `18f580a2` · 리뷰일 2026-08-07 · 소스 62파일 — `/module-review`로 갱신*
