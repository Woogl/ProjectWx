# WxGame — 코드 리뷰

> 조립 모듈답게 파일마다 책임이 얇고, Experience 파이프라인·권위 게이트·MVVM 글루가 주석으로 성실히 문서화돼 있어 전반적으로 건강하다. 직전 리뷰(`1e9b745c`) 이후 새로 들어온 `UWxMetaHumanVisualComponent`(부착물 생성·해제 대칭, 재등록 가드, LOD 매핑 경계 모두 확인)에서는 지적할 결함을 찾지 못했고, 기존 10건은 코드 변경 없이 그대로 남아 있다. 이번 리뷰는 `Source/WxGame` 62개 소스(.h/.cpp) 전부를 열고 프레임워크·캐릭터·어빌리티·MVVM 의 cpp 로직까지 내려가 확인했다(BP/WBP 내부는 범위 밖).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 7 |

## 결과

### 1. 🟡 처치 보상이 항상 0번 플레이어에게 지급된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:63`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 가 `UGameplayStatics::GetPlayerController(this, 0)` 을 `UWxRewardLibrary::GrantReward` 의 `DirectGrantTarget` 으로 넘긴다(`:65`). 그 인자는 픽업 형태가 없는 보상(골드 등 재화)을 곧바로 넣을 인벤토리 대상이므로(`Plugins/WxInventory/Source/WxInventory/Public/WxRewardLibrary.h:33`), 실제 처치자와 무관하게 서버의 0번 플레이어가 모든 재화를 가져간다. 모듈 전반이 서버 권위·소유 클라 복제를 세심히 지키는 것에 비해 여기만 싱글플레이 가정이 남아 있다. (4회 연속 지적, 미해결.)
- **제안**: 사망을 유발한 주체를 보관해 넘긴다. 대미지 경로의 마지막 instigator 를 `AWxCharacterBase` 에 기록하거나, 처형 경로가 이미 다루는 Interactor(`:133`)를 재사용해 그 컨트롤러를 대상으로 쓴다. 당분간 싱글플레이만 지원한다면 그 전제를 주석으로 못박아 두는 편이 낫다.
- **확신도**: 중간(의도된 단순화일 수 있음)

### 2. 🟡 `UpdateCharacterStateBeforeMovement` 가 `Spec.Ability` 를 널 검사 없이 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterMovementComponent.cpp:31`
- **범주**: 성능/안전
- **문제**: `if (Spec.IsActive() && !Spec.Ability->IsA<UWxAbility_LockOn>())` 가 `Spec.Ability` 를 무조건 역참조한다. `FGameplayAbilitySpec::Ability` 는 복제 대상이라 클라에서 스펙 배열이 먼저 도착하고 어빌리티 CDO 참조가 아직 매핑되지 않은 순간에는 널일 수 있으며(엔진 GAS 코드가 곳곳에서 `if (Spec.Ability)` 로 방어하는 이유), 이 코드는 웅크리기 의사가 있을 때 매 이동 틱 돌기 때문에(`:28` 루프) 그 창에 겹치면 이동 업데이트 중 크래시가 난다. 함께 볼 점으로, 판정이 "락온 외 활성 어빌리티가 하나라도 있는가"뿐인데 매 틱 활성화 가능 어빌리티 전체를 선형 순회한다. (4회 연속 지적, 미해결.)
- **제안**: 루프 조건에 `Spec.Ability &&` 를 추가한다. 순회 비용이 신경 쓰이면 ASC 의 블로킹 태그(`AreAbilityTagsBlocked(Ability)` — `AWxCharacterBase::CanJumpInternal_Implementation` 이 이미 쓰는 방식, `Source/WxGame/Character/WxCharacterBase.cpp:138`)로 같은 판정을 상수 시간에 대체할 수 있는지 검토한다.
- **확신도**: 중간(널 창이 짧아 재현은 드묾)

### 3. 🟡 `InitAbilitySystem` 이 멱등하지 않아 재호출 시 이동속도·어빌리티 부여가 중첩된다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:202`
- **범주**: 버그/정확성
- **문제**: 이 함수는 (a) `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed`(`:204`)로 기준 속도를 캡처하고 (b) SPD 어트리뷰트 변경에 `AddUObject` 로 구독하며(`:209`) (c) `GiveAbilitySet()`(`:215`)을 호출하는데, 셋 다 중복 방지가 없다. 진입점이 서버 `PossessedBy`(`:123`)와 클라 `OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:52`) 둘이라, 언포제스 후 재빙의나 폰 재사용, 혹은 `PlayerState` 포인터가 널→유효로 다시 복제되는 경우처럼 두 번째 호출이 생기면 이미 SPD 배율이 곱해진 `MaxWalkSpeed` 를 새 기준값으로 캡처해 속도가 복리로 커지고(500 → SPD 1.2 → 600 을 기준으로 다시 720), SPD 콜백이 두 번 등록되며, `UWxAbilitySystemComponent::GiveAbilitySet` 은 기존 핸들을 비우지 않고 `AbilitySetGrantedHandles` 에 덧붙이므로(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:23`) 어빌리티 스펙이 중복 부여된다. 부수적으로 `BaseWalkSpeed` 는 헤더에서 초기화되지 않는다(`Source/WxGame/Character/WxCharacterBase.h:133`). 현재 저장소에는 재빙의를 일으키는 호출자가 없어 실제 증상은 나지 않는다. (4회 연속 지적, 미해결.)
- **제안**: `BaseWalkSpeed` 를 헤더에서 0 초기화하고 캡처는 생성자/`PostInitializeComponents` 에서 1회만 한다. `InitAbilitySystem` 에는 재진입 가드를 두거나, 재등록 전 `RemoveAll(this)` 로 대칭을 맞춘다.
- **확신도**: 중간(정상 흐름에선 1회만 호출되므로 잠재 결함)

### 4. 🟢 더블 점프 2단 감쇠가 구현되어 있지 않다 (주석만 존재)
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:36`
- **범주**: 버그/정확성
- **문제**: `JumpMaxCount = 2`(`:37`) 위 주석이 "2단 Z속도 절반 적용은 `UWxCharacterMovementComponent::DoJump` 에서 처리한다"고 선언하지만, `UWxCharacterMovementComponent` 에는 `DoJump` 오버라이드가 없다(헤더의 오버라이드는 `GetGravityZ`·`UpdateCharacterStateBeforeMovement` 둘뿐 — `Source/WxGame/Character/WxCharacterMovementComponent.h:23`·`:31`). 저장소 전체(`Source`+`Plugins`)를 검색해도 `DoJump` 정의가 없고 이 주석 한 줄이 유일한 등장이다. 따라서 2단 점프는 1단과 같은 `JumpZVelocity`(640)로 나간다. (4회 연속 지적, 미해결.)
- **제안**: 의도가 유효하면 `DoJump(bool bReplayingMoves, float DeltaTime)` 를 추가해 `CharacterOwner->JumpCurrentCount >= 1` 일 때 Z 속도를 감쇠시킨다. 폐기된 계획이면 주석을 지운다.
- **확신도**: 높음

### 5. 🟢 `RefreshAllItems` 의 `NewItems`/`Retained` 는 내용이 완전히 동일한 중복 배열
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:122`
- **범주**: 중복/복잡도
- **문제**: 같은 루프에서 `NewItems.Add(ChildVM); Retained.Add(ChildVM);`(`:149-150`)로 동일한 원소만 넣고, `Retained` 는 이후 `Retained.Contains(OldVM)`(`:156`) 판정에만 쓰인 뒤 버려진다. 두 배열은 항상 같으므로 `Retained` 는 순수 중복이다. 함께 볼 점으로, 이 함수는 인스턴스마다 `AllItems` 를 선형 탐색(`:134-141`)하고 다시 `Contains` 로 선형 탐색하므로 슬롯 수에 대해 O(N²)이며, 스택이 바뀔 때마다 호출된다. (4회 연속 지적, 미해결.)
- **제안**: `Retained` 를 제거하고 `NewItems.Contains(OldVM)` 으로 판정한다. 슬롯 수가 커질 여지가 있으면 인스턴스→VM `TMap` 으로 탐색을 상수 시간으로 바꾼다.
- **확신도**: 높음

### 6. 🟢 `HandleDeath` 오버라이드가 베이스의 접근 지정자를 좁힌다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.h:47`, 베이스 선언 `Source/WxGame/Character/WxCharacterBase.h:67`
- **범주**: 설계/구조
- **문제**: `AWxCharacterBase::HandleDeath` 는 `public` 인데 `AWxEnemyCharacter` 는 같은 함수를 `protected` 섹션(`:43` 이후)에서 오버라이드한다. "virtual override 는 직접 베이스의 접근 지정자를 그대로 쓴다(좁힘·넓힘 금지)"는 프로젝트 관례에 어긋나고, 베이스 포인터로는 부를 수 있는 함수를 구체 타입 포인터로는 못 부르는 비대칭이 생긴다. (3회 연속 지적, 미해결.)
- **제안**: `AWxEnemyCharacter` 의 선언을 `public` 섹션으로 옮긴다(또는 베이스를 `protected` 로 내리고 호출부를 확인한다).
- **확신도**: 높음

### 7. 🟢 `WxAbility_UseItem` 이 사용 가능 검사보다 먼저 `CommitAbility` 를 호출한다
- **위치**: `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:34`
- **범주**: 버그/정확성
- **문제**: `ActivateAbility` 는 `CommitAbility`(코스트·쿨다운 적용)를 먼저 통과시킨 뒤(`:34`), 그 다음에야 "보유·충전이 없으면 마시기 모션을 내지 않는다"는 인벤토리 검사를 수행한다(`:44` `Inventory->CanUseItemByDef`). 검사에 걸리면 `EndAbility(..., bWasCancelled=true)`(`:46`)로 빠지지만 GAS 는 취소로 커밋된 쿨다운·코스트를 되돌리지 않는다. 즉 빈 병 상태에서 사용 키를 누르면 아무 일도 일어나지 않은 채 쿨다운만 소모된다. 현재 `GA_UseItem` 은 `abilityDataRow.dataTable` 이 null 이라(`.claude/asset_dump/Blueprints/GA_UseItem.json`) `UWxAbilityBase::GetCooldownGameplayEffect` 가 널을 반환하므로 증상이 드러나지 않지만, 기획자가 이 어빌리티에 쿨다운 Row 를 지정하는 순간 발현한다. (2회 연속 지적, 미해결.)
- **제안**: 검사 순서를 뒤집어 `UseMontage`·`ConsumableDef`·`CanUseItemByDef` 를 모두 통과한 뒤 `CommitAbility` 를 호출한다. 클라 예측 단계에서도 막고 싶으면 `CanActivateAbility` 오버라이드로 인벤토리 조건을 올린다.
- **확신도**: 중간(현재 에셋 설정에서는 무증상인 잠재 결함)

### 8. 🟢 획득 알림 VM(`LastAcquiredItem`)이 교체·해제 시 `Deinitialize` 되지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:108`, 해제부 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:58`
- **범주**: 중복/복잡도
- **문제**: `HandleStackChanged` 는 `Delta > 0` 마다 Def 모드 `UWxViewModel_Item` 을 새로 만들어 `LastAcquiredItem` 에 대입하는데(`:108-111`), 그 Def 모드 `Initialize` 는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 에 바인딩한다(`Source/WxGame/MVVM/WxViewModel_Item.cpp:45-46`). 교체 시에도, `Deinitialize` 시에도(`:58` 단순 nullptr 대입) 이전 VM 에 `Deinitialize()` 를 부르지 않는다. 같은 파일의 `AllItems` 는 미보존 VM 을 일일이 `Deinitialize` 하며 대칭을 지키는데(`:154-160`) 획득 VM 만 예외다. `UWxViewModel::BeginDestroy` 가 `Deinitialize` 를 부르므로(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:8-12`) 영구 누수는 아니지만, GC 전까지 무의미한 콜백을 계속 받고 수명 규약이 한 파일 안에서 어긋난다. (4회 연속 지적, 미해결.)
- **제안**: 교체 직전과 `Deinitialize` 시 이전 `LastAcquiredItem` 에 `Deinitialize()` 를 호출해 `AllItems` 경로와 대칭을 맞춘다.
- **확신도**: 중간

### 9. 🟢 `IsAlive()` 가 널 검사보다 먼저 `AbilitySystemComponent` 를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:195`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:85`
- **범주**: 성능/안전
- **문제**: `IsAlive()` 는 `AbilitySystemComponent->GetSet<...>()` 를 널 검사 없이 역참조하는데, 같은 클래스의 `GetOwnedGameplayTags`(`:154`)·`CanJumpInternal_Implementation`(`:128`)은 `if (AbilitySystemComponent)` 로 방어한다. 게다가 `AWxEnemyCharacter::GetEligibleFinisherEventTag` 의 가드가 `if (!IsAlive() || !AbilitySystemComponent)` 순서라, ASC 가 널이면 `!AbilitySystemComponent` 검사에 도달하기 전에 `IsAlive()` 안에서 이미 역참조한다. 실제로 ASC 는 생성자에서 항상 만들어져 널이 되지 않으므로 크래시로 이어지진 않지만, 방어 의도와 실제 동작이 어긋나 읽는 사람을 오도한다. (4회 연속 지적, 미해결.)
- **제안**: `IsAlive()` 에 널 가드를 추가하거나, 반대로 ASC 가 항상 유효하다는 전제로 통일해 불필요한 널 검사들을 정리한다.
- **확신도**: 낮음(현재 경로에서 ASC 는 항상 유효 — 의도된 단순화일 수 있음)

### 10. 🟢 규칙 4 위반 가능 — 입력 바인딩 콜백에 `Handle` prefix 없음
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:43-49`, 바인딩부 `Source/WxGame/Character/WxPlayerCharacter.cpp:82`·`:86`·`:95`·`:110-112`
- **범주**: 규칙 위반
- **문제**: `Move`/`Look`/`ToggleCrouch`/`AbilityInputStarted`/`AbilityInputTriggered`/`AbilityInputReleased` 는 모두 `UEnhancedInputComponent::BindAction` 으로 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 가 없다. 같은 클래스 계층의 다른 델리게이트 콜백(`HandleSPDAttributeChanged`, `HandleEquipVisualChanged`, `HandleMontageCompleted` 등)은 규칙을 지키고 있어 모듈 안에서도 일관되지 않다. 저장소에서 `BindAction` 을 쓰는 곳은 이 파일뿐이라 비교할 선례가 없다. (4회 연속 지적, 미해결.)
- **제안**: 규칙을 문자 그대로 적용하면 `HandleMoveInput` 등으로 개명한다. 입력 핸들러를 규칙 4 대상에서 뺄 생각이면 `CLAUDE.md` 에 예외를 적어 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`·`.h`, `Source/WxGame/Framework/WxGameMode.cpp`·`.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceManager.cpp`·`.h`, `Source/WxGame/Character/WxCharacterBase.cpp`·`.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`·`.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`·`.h`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`·`.h`, `Source/WxGame/Character/WxMetaHumanVisualComponent.cpp`·`.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`·`.h`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`·`.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`·`.h`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`·`.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`·`.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`·`.h`, `Source/WxGame/Framework/WxGameState.cpp`·`.h`, `Source/WxGame/Controller/WxPlayerController.cpp`·`.h`, `Source/WxGame/Player/WxPlayerState.cpp`·`.h`, `Source/WxGame/Character/WxBossCharacter.cpp`·`.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`·`.h`, `Source/WxGame/Input/WxInputConfig.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.h`
- **교차 확인(모듈 밖, 근거 확보용)**: `Plugins/WxCombat` 의 `UWxAbilitySystemComponent::GiveAbilitySet`·`UWxAbilitySet::GiveToAbilitySystem`(중복 부여, 3번 근거)과 `UWxAbilityBase::GetCooldownGameplayEffect`(7번 근거), `Plugins/WxInventory` 의 `UWxRewardLibrary::GrantReward` 시그니처와 `DirectGrantTarget` 의미(1번 근거), `Plugins/WxUI` 의 `UWxViewModel::BeginDestroy`/`Deinitialize`(8번 근거), `.claude/asset_dump/Blueprints/GA_UseItem.json`(7번 발현 여부 확인), 저장소 전역 `DoJump` 검색(4번 근거), 저장소 전역 규칙 스캔(첫 줄 저작권 표기 62/62 통과, `FORCEINLINE`·인라인 정의 0건, 람다 0건)
- **미검토 / 한계**: (1) 정적 분석만 수행했다 — 2·3번은 코드 경로로만 확인했고 원격 클라 세션 재현은 하지 않았다. (2) 규칙 5(`BlueprintCallable`)는 뷰모델 커맨드(`Request~`/`SetCurrentCategory`, 5건)를 예외로 확정한 이전 결정이 있어 제외했다. `WxUI` 의 `UWxViewModel_Ability::RequestActivate` 도 같은 형태라 프로젝트 전반의 확립된 패턴으로 판단했다. (3) `UWxGameFeatureAction_AddComponents::FContextHandles`(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h:65`)는 `Wx` prefix 없는 `F` 타입이지만, USTRUCT 이 아닌 클래스 내부 private 구현 구조체라 규칙 1 위반으로 올리지 않았다 — 판단이 다르면 개명 대상이다. (4) `UWxExperienceManagerComponent::EndPlay` 가 `Loaded` 이전 상태(로딩 중 월드 종료)에서 액션 비활성을 건너뛰고 GF 플러그인만 내리는 비대칭은 Lyra 원본과 동일해 의도로 보고 제외했다. (5) `UWxMetaHumanVisualComponent` 가 생성 부착물에 자신의 `CreationMethod` 를 물려주는 것(`Source/WxGame/Character/WxMetaHumanVisualComponent.cpp:47`)은 주석으로 의도가 명시돼 있고 엔진 재구축 경로에서의 실제 영향을 재현으로 확인하지 못해 발견으로 올리지 않았다. 같은 이유로 이 컴포넌트가 넷모드 가드 없이(커맨드릿 가드만 존재, `:24`) 데디케이티드 서버에서도 페이스·그룸을 조립하는 점도 제외했다 — 현재 `Source/` 에 서버 타겟이 없어 실효가 없다. (6) BP/WBP 디폴트값(`InputConfig`·`RewardRow`·`BehaviorTreeAsset`·`FaceMesh` 등 메타휴먼 슬롯·Experience 에셋 실제 내용)은 범위 밖이라, 에셋 미지정으로만 가려져 있는 경로가 남아 있을 수 있다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 62파일 — `/module-review`로 갱신*
