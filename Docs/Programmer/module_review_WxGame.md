# WxGame — 코드 리뷰

> 조립 모듈답게 파일마다 책임이 얇고, Experience 파이프라인·권위 게이트·MVVM 글루가 주석으로 성실히 문서화돼 있어 전반적으로 건강하다. 직전 리뷰의 🔴(UseItem 권위 게이트 누락)와 컨트롤러 HUD 푸시 문제는 해소됐고, 남은 것은 멀티플레이 전파 가정과 문서-구현 불일치다. 이번 리뷰는 `Source/WxGame` 60개 소스(.h/.cpp)를 훑고 프레임워크·캐릭터·어빌리티·MVVM 의 cpp 로직까지 내려가 확인했다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 사망 태그 구독이 서버·소유 클라에만 걸려 "전 머신 사망 처리"가 시뮬 프록시에서 성립하지 않는다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:214`, `Source/WxGame/Character/WxCharacterBase.cpp:279`
- **범주**: 설계/구조
- **문제**: `HandleDeathTagChanged` 등록이 `InitAbilitySystem` 안에 있는데, 이 함수의 진입점은 서버 `PossessedBy`(`:125`)와 소유 클라 `AWxPlayerCharacter::OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:75`) 둘뿐이다. 같은 파일이 래그돌 구독을 `PostInitializeComponents` 로 뺀 이유를 "서버·오너 클라에서만 도는 InitAbilitySystem이 아니라 여기서 구독한다"(`:70`)로 명시하고 있어, 저자도 이 제약을 알고 있다. 그런데 `HandleDeath` 의 주석(`:281-282`)은 "사망 태그는 복제되어 모든 머신에서 이 경로를 타므로, 각 머신의 로컬 판정이 함께 해제된다"고 반대로 적혀 있다. 실제로는 원격 클라(적 캐릭터는 AI 빙의가 서버에서만 일어나므로 **모든** 클라, 다른 플레이어 캐릭터는 시뮬 프록시 머신)에서 `HandleDeath` 가 아예 호출되지 않는다. 결과로 (a) `Weapon->CancelAttack()`(`:285`)이 그 머신에서 안 돌아 사망한 캐릭터의 히트 콜리전·틱이 스윙 구간에 남고 — `AWxWeaponBase::ProcessHit` 는 권위 게이트가 없어 로컬로 계속 히트를 처리한다(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:253`) — (b) `OnDeath` 방송(`:288`)도 그 머신에선 발생하지 않는다.
- **제안**: 래그돌과 같은 처리를 한다 — `HandleDeathTagChanged` 구독을 `PostInitializeComponents` 로 옮기고(초기 복제로 이미 태그가 실려온 경우의 1회 즉시 확인도 함께), 보상 지급 같은 권위 전용 부분은 지금처럼 `HandleDeath` 내부의 `HasAuthority` 가드로 남긴다(`Source/WxGame/Character/WxEnemyCharacter.cpp:51`). 옮기지 않을 거면 `:281-282` 주석을 실제 동작에 맞게 정정한다.
- **확신도**: 높음(등록 지점은 확정적). 싱글플레이·스탠드얼론에서는 증상이 드러나지 않음

### 2. 🟡 처치 보상이 항상 0번 플레이어에게 지급된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:63`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 `UGameplayStatics::GetPlayerController(this, 0)` 을 `UWxRewardLibrary::GrantReward` 의 직접 지급 대상으로 넘긴다(`:65`). 이 인자는 픽업 형태가 없는 보상(골드 등 재화)을 곧바로 넣을 인벤토리 대상이므로, 실제 처치자와 무관하게 서버의 0번 플레이어가 모든 재화를 가져간다. 모듈 전반이 서버 권위·소유 클라 복제를 세심히 지키는 것에 비해 여기만 싱글플레이 가정이 남아 있다. (직전 리뷰에서도 지적된 항목이며 미해결.)
- **제안**: 사망을 유발한 주체를 보관해 넘긴다. 데미지 경로의 마지막 instigator 를 `AWxCharacterBase` 에 기록하거나, 처형 경로가 이미 다루는 Interactor(`:133`)를 재사용해 그 컨트롤러를 대상으로 쓴다. 당분간 싱글플레이만 지원한다면 그 전제를 주석으로 못박아 두는 편이 낫다.
- **확신도**: 중간(의도된 단순화일 수 있음)

### 3. 🟡 `InitAbilitySystem` 이 멱등하지 않아 재호출 시 이동속도가 복리로 부푼다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:204`
- **범주**: 버그/정확성
- **문제**: 이 함수는 (a) `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed`(`:206`)로 기준 속도를 캡처하고 (b) SPD 어트리뷰트 변경·`State.Dead` 태그 이벤트에 `AddUObject` 로 구독하는데, 둘 다 중복 방지가 없다. 진입점이 서버 `PossessedBy`(`:125`)와 클라 `OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:75`) 둘이라, 언포제스 후 재빙의(PlayerState 가 PS→null→PS 로 바뀌며 OnRep 재발화)처럼 두 번째 호출이 생기면 이미 SPD 배율이 곱해진 `MaxWalkSpeed` 를 새 기준값으로 캡처해 속도가 복리로 커진다(500 → SPD 1.2 → 600 을 기준으로 다시 720). 부수적으로 `BaseWalkSpeed` 는 헤더에서 초기화되지 않아(`Source/WxGame/Character/WxCharacterBase.h:133`) `InitAbilitySystem` 이 한 번도 안 도는 경로(원격 클라의 적 캐릭터)에선 미초기화 값이 그대로 남는다.
- **제안**: `BaseWalkSpeed` 를 헤더에서 0 초기화하고 캡처는 생성자/`PostInitializeComponents` 에서 1회만 한다. `InitAbilitySystem` 에는 재진입 가드(초기화 플래그 또는 재등록 전 `RemoveAll(this)`)를 둔다.
- **확신도**: 중간(정상 흐름에선 1회만 호출되므로 잠재 결함)

### 4. 🟡 `UpdateCharacterStateBeforeMovement` 가 `Spec.Ability` 를 널 검사 없이 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterMovementComponent.cpp:32`
- **범주**: 성능/안전
- **문제**: `if (Spec.IsActive() && !Spec.Ability->IsA<UWxAbility_LockOn>())` 가 `Spec.Ability` 를 무조건 역참조한다. `FGameplayAbilitySpec::Ability` 는 복제 대상이라 클라에서 스펙 배열이 먼저 도착하고 어빌리티 CDO 참조가 아직 매핑되지 않은 순간에는 널일 수 있으며(엔진 GAS 코드가 곳곳에서 `if (Spec.Ability)` 로 방어하는 이유), 이 코드는 매 이동 틱(웅크리기 의사가 있을 때) 돌기 때문에 그 창에 겹치면 이동 업데이트 중 크래시가 난다. 함께 볼 점으로, 판정이 "락온 외 활성 어빌리티가 하나라도 있는가"뿐인데 매 틱 활성화 가능 어빌리티 전체를 선형 순회한다.
- **제안**: 루프 조건에 `Spec.Ability &&` 를 추가한다. 순회 비용이 신경 쓰이면 ASC 의 블로킹 태그(`AreAbilityTagsBlocked(Ability)` — `AWxCharacterBase::CanJumpInternal_Implementation` 이 이미 쓰는 방식, `Source/WxGame/Character/WxCharacterBase.cpp:140`)로 같은 판정을 상수 시간에 대체할 수 있는지 검토한다.
- **확신도**: 중간(널 창이 짧아 재현은 드묾)

### 5. 🟢 더블 점프 2단 감쇠가 구현되어 있지 않다 (주석만 존재)
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:37`
- **범주**: 버그/정확성
- **문제**: `JumpMaxCount = 2` 위 주석이 "2단 Z속도 절반 적용은 `UWxCharacterMovementComponent::DoJump` 에서 처리한다"고 선언하지만, `UWxCharacterMovementComponent` 에는 `DoJump` 오버라이드가 없다(헤더는 `GetGravityZ`/`UpdateCharacterStateBeforeMovement` 둘뿐). 저장소 전체(`Source`+`Plugins`)를 검색해도 `DoJump` 정의가 없고, 이 주석 한 줄이 유일한 등장이다. 따라서 2단 점프는 1단과 같은 `JumpZVelocity`(640)로 나간다. (직전 리뷰에서도 지적된 항목이며 미해결.)
- **제안**: 의도가 유효하면 `DoJump(bool bReplayingMoves, float DeltaTime)` 를 추가해 `CharacterOwner->JumpCurrentCount >= 1` 일 때 Z 속도를 감쇠시킨다. 폐기된 계획이면 주석을 지운다.
- **확신도**: 높음

### 6. 🟢 획득 알림 VM(`LastAcquiredItem`)이 교체·해제 시 `Deinitialize` 되지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104`, 해제부 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:58`
- **범주**: 중복/복잡도
- **문제**: `HandleStackChanged` 는 `Delta > 0` 마다 Def 모드 `UWxViewModel_Item` 을 새로 만들어 `LastAcquiredItem` 에 대입하는데, 그 Def 모드 `Initialize` 는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 에 바인딩한다(`Source/WxGame/MVVM/WxViewModel_Item.cpp:45-46`). 교체 시에도, `Deinitialize` 시에도(단순 nullptr 대입) 이전 VM 에 `Deinitialize()` 를 부르지 않는다. 같은 파일의 `AllItems` 는 미보존 VM 을 일일이 `Deinitialize` 하며 대칭을 지키는데(`:154-160`) 획득 VM 만 예외다. `UWxViewModel::BeginDestroy` 가 `Deinitialize` 를 부르므로 영구 누수는 아니지만, GC 전까지 무의미한 콜백을 계속 받고 수명 규약이 한 파일 안에서 어긋난다.
- **제안**: 교체 직전과 `Deinitialize` 시 이전 `LastAcquiredItem` 에 `Deinitialize()` 를 호출해 `AllItems` 경로와 대칭을 맞춘다.
- **확신도**: 중간

### 7. 🟢 `RefreshAllItems` 의 `NewItems`/`Retained` 는 내용이 완전히 동일한 중복 배열
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:121`
- **범주**: 중복/복잡도
- **문제**: 같은 루프에서 `NewItems.Add(ChildVM); Retained.Add(ChildVM);`(`:149-150`)로 동일한 원소만 넣고, `Retained` 는 이후 `Retained.Contains(OldVM)`(`:156`) 판정에만 쓰인 뒤 버려진다. 두 배열은 항상 같으므로 `Retained` 는 순수 중복이다. 부수적으로 기존 VM 매칭 루프(`:134`)와 `Contains` 가 겹쳐 슬롯 수 N 에 대해 O(N²)이지만 인벤토리 규모상 실질 비용 문제는 아니다. (직전 리뷰에서도 지적된 항목이며 미해결.)
- **제안**: `Retained` 를 제거하고 `NewItems.Contains(OldVM)` 으로 판정한다.
- **확신도**: 높음

### 8. 🟢 규칙 5 위반 — 뷰모델에 `BlueprintCallable` 5건
- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:41`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:53`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:57`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:102`, `Source/WxGame/MVVM/WxViewModel_Item.h:64`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리로 한정한다. `RequestAdvance`/`RequestInteract`/`RequestCycle`/`RequestUseConsumable` 은 WBP 가 뷰모델에 커맨드를 거는 용도이고, `SetCurrentCategory` 는 `BlueprintSetter` 겸용이라 엔진 요구에 가깝다. 어느 쪽도 규칙이 허용한 두 자리는 아니다. (직전 리뷰의 동일 지적 후 1건이 더 늘었다.)
- **제안**: 규칙을 지킬 거면 WBP 가 스캐너/세션/인벤토리 컴포넌트를 얇은 BP Function Library 를 경유해 부르게 하고, 유지할 거면 `CLAUDE.md` 규칙 5 에 "뷰모델의 뷰→모델 커맨드 함수" 예외를 명시해 다음 세션이 다시 헷갈리지 않게 한다.
- **확신도**: 높음(위반 사실). 단 의도된 예외일 수 있음

### 9. 🟢 규칙 6 위반 — 헤더에 인라인 위임 생성자 정의
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:38`
- **범주**: 규칙 위반
- **문제**: `AWxCharacterBase() : AWxCharacterBase(FObjectInitializer::Get()) {}` 는 헤더 내 인라인 함수(생성자) 정의로, "인라인 함수 정의를 금지한다"는 코딩 규칙 6 에 걸린다. 모듈 전체에서 인라인 정의는 이 한 줄뿐이라 예외로 남아 있다. 파생 클래스(`AWxEnemyCharacter`)가 인자 없는 생성자를 쓰므로 기본 생성자 자체는 필요하다.
- **제안**: 헤더에는 선언만 두고 `WxCharacterBase.cpp` 에 `AWxCharacterBase::AWxCharacterBase() : AWxCharacterBase(FObjectInitializer::Get()) {}` 로 정의를 옮긴다.
- **확신도**: 높음(위반 사실). 단 의도된 예외일 수 있음

### 10. 🟢 규칙 4 위반 가능 — 입력 바인딩 콜백에 `Handle` prefix 없음
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:52-58`, 바인딩부 `Source/WxGame/Character/WxPlayerCharacter.cpp:105-135`
- **범주**: 규칙 위반
- **문제**: `Move`/`Look`/`ToggleCrouch`/`AbilityInputStarted`/`AbilityInputTriggered`/`AbilityInputReleased` 는 모두 `UEnhancedInputComponent::BindAction` 으로 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 가 없다. 같은 클래스 계층의 다른 델리게이트 콜백(`HandleSPDAttributeChanged`, `HandleEquipVisualChanged`, `HandleMontageCompleted` 등)은 규칙을 지키고 있어 모듈 안에서도 일관되지 않다. 저장소에서 `BindAction` 을 쓰는 곳은 이 파일뿐이라 비교할 선례가 없다.
- **제안**: 규칙을 문자 그대로 적용하면 `HandleMoveInput` 등으로 개명한다. 입력 핸들러를 규칙 4 대상에서 뺄 생각이면 `CLAUDE.md` 에 예외를 적어 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 11. 🟢 `IsAlive()` 가 널 검사보다 먼저 `AbilitySystemComponent` 를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:195`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:85`
- **범주**: 성능/안전
- **문제**: `IsAlive()` 는 `AbilitySystemComponent->GetSet<...>()` 를 널 검사 없이 역참조하는데, 같은 클래스의 `GetOwnedGameplayTags`(`:154`)·`CanJumpInternal_Implementation`(`:130`)은 `if (AbilitySystemComponent)` 로 방어한다. 게다가 `AWxEnemyCharacter::GetEligibleFinisherEventTag` 의 가드가 `if (!IsAlive() || !AbilitySystemComponent)` 순서라, ASC 가 널이면 `!AbilitySystemComponent` 검사에 도달하기 전에 `IsAlive()` 안에서 이미 역참조한다. 실제로 ASC 는 생성자에서 항상 만들어져 널이 되지 않으므로 크래시로 이어지진 않지만, 방어 의도와 실제 동작이 어긋나 읽는 사람을 오도한다.
- **제안**: `IsAlive()` 에 널 가드를 추가하거나, 반대로 ASC 가 항상 유효하다는 전제로 통일해 불필요한 널 검사들을 정리한다.
- **확신도**: 낮음(현재 경로에서 ASC 는 항상 유효 — 의도된 단순화일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`·`.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`.h`, `Source/WxGame/Character/WxCharacterBase.cpp`·`.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`·`.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`·`.h`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`·`.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxExperienceManager.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`·`.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`·`.h`, `Source/WxGame/Framework/WxGameState.cpp`·`.h`, `Source/WxGame/Controller/WxPlayerController.cpp`·`.h`, `Source/WxGame/Controller/WxEnemyController.cpp`·`.h`, `Source/WxGame/Player/WxPlayerState.cpp`·`.h`, `Source/WxGame/Character/WxBossCharacter.cpp`·`.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`·`.h`, `Source/WxGame/Input/WxInputConfig.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`·`.h`, `Source/WxGame/WxGame.cpp`·`.h`
- **교차 확인(모듈 밖, 근거 확보용)**: `Plugins/WxCombat` 의 `AWxWeaponBase::CancelAttack`/`ProcessHit`(권위 게이트 부재 확인), `Plugins/WxUI` 의 `UWxViewModel::BeginDestroy`·`UWxNameplateComponent::InitializeViewModels`, 저장소 전역 `DoJump`·`OnDeath`·`InitAbilityActorInfo` 검색
- **미검토 / 한계**: (1) 정적 분석만 수행했다 — 1·3·4번은 코드 경로로 확인했으나 실제 원격 클라 세션 재현은 하지 않았다. (2) `WxCombat` 의 ASC 확장(`GiveAbilitySet`, `GetAbilityInputActions`, `AbilityInputAction*`)과 `WxInventory` 의 `GrantItems`/`UseItemByDef` 는 호출 규약만 확인하고 내부는 보지 않았다. (3) BP/WBP 디폴트값(`InputConfig`·`RewardRow`·`BehaviorTreeAsset`·Experience 에셋 실제 내용)은 범위 밖이라, 에셋 미지정으로만 가려져 있는 경로가 남아 있을 수 있다. (4) 직전 리뷰(커밋 `c42b5fec`)의 🔴 "UseItem 권위 게이트 누락"은 `WxAbility_UseItem.cpp:79` 에 `HasAuthority` 가드가 들어와 해소, "`OnRep_Pawn` HUD 중복 푸시"와 "Pawn/PlayerState receiver 부재"는 각각 컨트롤러의 HUD 중개 제거와 `AWxCharacterBase`/`AWxPlayerState` 의 receiver 등록으로 해소된 것으로 판단해 제외했다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 60파일 — `/module-review`로 갱신*
