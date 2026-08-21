# WxGame — 코드 리뷰

> 조립 전용 모듈답게 각 파일이 얇고 책임 경계가 또렷하다. Lyra Experience 파이프라인 이식부는 서버/클라 대칭·PIE 다중 세션까지 충실히 옮겨져 있고, 델리게이트 `Handle` prefix·저작권 헤더·람다/FORCEINLINE 금지 등 코딩 규칙 위반은 사실상 없다. 이번 리뷰는 66개 소스 전부를 훑고 Framework(Experience 부트스트랩)·Character 계층·MVVM 뷰모델의 cpp 로직까지 내려가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 Experience 미확정이면 폰이 영영 스폰되지 않는데 진단은 Warning 한 줄뿐
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:82`, `Source/WxGame/Framework/WxGameMode.cpp:55`
- **범주**: 버그/정확성
- **문제**: `ResolveExperienceId()` 가 무효 ID 를 돌려주면 `SetCurrentExperience` 가 `LoadState = Unloaded` 인 채로 빠져나간다. 그 뒤 `HandleStartingNewPlayer_Implementation` 은 `IsExperienceLoaded()` 가 false 라 매번 조기 반환하고, `HandleExperienceLoaded` 는 영원히 호출되지 않는다. 결과는 폰도 입력도 없는 죽은 세션인데, 남는 단서는 `"Experience 미설정. 프레임워크 컴포넌트가 주입되지 않음."` 이라는 Warning 한 줄이다. 이 문구는 실제 증상(폰 미스폰)을 지목하지 않아, WorldSettings 에 Experience 를 안 꽂은 신규 맵에서 원인 추적이 오래 걸린다. `GetDefaultPawnClassForController_Implementation` 도 이 경우 `"InitGameState 가 이미 경고한 상태다"` 라며 침묵한다(`WxGameMode.cpp:41`).
- **제안**: 확정 실패를 Error 로 올리고 문구에 결과를 명시한다(예: "Experience 를 확정하지 못해 이 세션에서는 플레이어 폰이 스폰되지 않는다 — WorldSettings/GameMode DefaultExperience 확인"). 또는 `HandleStartingNewPlayer` 조기 반환 시 1회 한정으로 대기 사유를 로그에 남긴다.
- **확신도**: 높음

### 2. 🟡 처치 보상이 킬러가 아니라 항상 0번 플레이어에게 지급된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:91`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 는 `HasAuthority()` 로 서버 권위를 지킨 뒤 `UGameplayStatics::GetPlayerController(this, 0)` 로 보상 수령자를 정한다. 데디케이티드 서버에서 인덱스 0 은 "먼저 접속한 아무 플레이어"이고, 리슨 서버에서는 항상 호스트다. 즉 누가 죽였는지와 무관하게 한 명이 전부 가져간다. 이 모듈은 나머지 곳(Mixed/Full 복제 모드, `OnInteracted` 의 실제 instigator 재판정, `WxAbility_Interact` 의 서버 거리·자격 검증)에서 멀티 권위 모델을 꽤 엄격히 지키고 있어 이 지점만 단일 플레이어 가정에 묶여 있다.
- **제안**: 가해자를 이미 알고 있는 경로가 있다 — `HandleIncomingDamageChanged` 가 쓰는 `FGameplayEffectContext::GetInstigator()`. 마지막 유효 가해자를 사망 시점까지 들고 있다가 그 컨트롤러에 지급하거나, 최소한 "단일 플레이어 전제"임을 주석으로 못 박아 두고 TODO 로 남긴다.
- **확신도**: 중간 (현 단계가 싱글플레이 스코프라면 의도된 단순화일 수 있음)

### 3. 🟢 널 가드가 역참조 뒤에 있어 무력하다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:113`, `Source/WxGame/Character/WxCharacterBase.cpp:191`
- **범주**: 버그/정확성
- **문제**: `if (!IsAlive() || !AbilitySystemComponent)` 에서 `IsAlive()` 가 먼저 평가되는데, `IsAlive()` 자체가 `AbilitySystemComponent->GetSet<...>()` 로 무가드 역참조를 한다. 즉 뒤에 붙은 `!AbilitySystemComponent` 검사는 도달 불가능한 죽은 가드다. 같은 클래스의 `GetOwnedGameplayTags`·`CanJumpInternal_Implementation` 은 반대로 널 검사를 하고 있어 규약도 엇갈린다. 실제로 ASC 는 생성자 서브오브젝트라 널이 될 수 없으므로 현재 크래시로 이어지지는 않는다.
- **제안**: 순서를 뒤집거나(`!AbilitySystemComponent || !IsAlive()`), ASC 가 불변 서브오브젝트라는 전제를 받아들여 모듈 전반의 널 검사를 걷어내 한쪽으로 통일한다.
- **확신도**: 중간

### 4. 🟢 `InitAbilitySystem()` 이 멱등하지 않다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:198`, `Source/WxGame/Character/WxEnemyCharacter.cpp:46`, `Source/WxGame/Character/WxCharacterBase.h:123`
- **범주**: 버그/정확성
- **문제**: 이 함수는 헤더 주석대로 서버 `PossessedBy` 와 클라 `OnRep_PlayerState` 양쪽에서 불리는 진입점인데, 재호출 방어가 없다. 두 번 불리면 (a) SPD 어트리뷰트 델리게이트와(에너미는 IncomingDamage 델리게이트까지) 중복 구독되고, (b) `BaseWalkSpeed = MaxWalkSpeed` 재기준화가 이미 SPD 배율이 곱해진 값을 새 기준으로 잡아 속도가 누적 드리프트한다. 현재 리스폰이 맵 리로드를 거쳐 새 폰을 만들므로 재빙의 경로는 보이지 않지만, 폰 재사용 리스폰을 도입하는 순간 조용히 깨진다. 덧붙여 `BaseWalkSpeed` 는 초기화자가 없다(UObject 메모리가 0 으로 채워져 현재는 무해).
- **제안**: 1회 실행 가드를 두거나, 구독을 `PostInitializeComponents`(래그돌/사망 구독과 같은 자리)로 옮기고 `InitAbilitySystem` 은 `RefreshAbilityActorInfo` + `GiveAbilitySet` 만 남긴다. `BaseWalkSpeed` 에는 명시적 초기화자를 준다.
- **확신도**: 낮음 (현 리스폰 설계에서는 재호출 경로가 없어 의도된 전제일 수 있음)

### 5. 🟢 이동 갱신마다 ASC 를 탐색한다
- **위치**: `Source/WxGame/Character/WxCharacterMovementComponent.cpp:51`
- **범주**: 성능/안전
- **문제**: `UpdateCharacterStateBeforeMovement` 는 캐릭터 전원(플레이어 + 전 AI)에 대해 매 이동 갱신마다 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner)` 를 호출한다. 클라 예측 리플레이 중에는 한 프레임에 여러 번 돈다. 이 CMC 는 `AWxCharacterBase` 가 클래스를 교체해 붙이므로 오너 타입이 확정돼 있고, ASC 는 생성자 서브오브젝트라 수명 내내 불변이다 — 매번 인터페이스 캐스트/컴포넌트 탐색을 돌 이유가 없다.
- **제안**: 첫 접근 시 `CharacterOwner` 에서 한 번 해석해 약참조로 캐시하거나, `AWxCharacterBase` 캐스트로 직접 받는다(`OnMovementModeChanged` 도 같은 캐시를 쓴다).
- **확신도**: 중간

### 6. 🟢 Public 헤더가 Private 의존 모듈의 헤더를 include 한다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.h:7`, `Source/WxGame/WxGame.Build.cs:39`
- **범주**: 설계/구조
- **문제**: `WxGame.Build.cs` 는 `PublicIncludePaths.AddRange(ModuleDirectory)` 로 모듈 전 헤더를 공개하지만 `MetaHumanSDKRuntime` 은 `PrivateDependencyModuleNames` 에 있다. 그런데 공개된 `WxMetaHumanComponent.h` 가 그 모듈의 `MetaHumanComponentUE.h` 를 include 한다. 현재 유일한 소비자인 `WxEditor` 는 `Framework/WxExperienceManager.h` 만 포함해 문제가 드러나지 않지만, 에디터 툴이 캐릭터 계층을 건드리는 순간 "cannot open include file" 로 터진다.
- **제안**: `MetaHumanSDKRuntime`(그리고 `.cpp` 에서만 쓰이는 `HairStrandsCore` 는 그대로 두고) 을 `PublicDependencyModuleNames` 로 올리거나, 이 컴포넌트 헤더가 엔진 베이스를 노출하지 않도록 감싼다.
- **확신도**: 중간

### 7. 🟢 ViewModel 진입 함수에 `BlueprintCallable` 사용
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:51`·`:54`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:90`, `Source/WxGame/MVVM/WxViewModel_Item.h:49`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:39`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수로 한정한다. 위 5개는 WBP 가 Enhanced Input·MVVM Event 바인딩으로 직접 호출하는 VM 요청 함수라 어느 쪽도 아니다.
- **제안**: 규칙을 그대로 지킬 것이라면 `UWxUILibrary` 류 파사드를 경유시키고, 반대로 "MVVM 바인딩 수신 진입점"을 허용 범주로 볼 것이라면 CLAUDE.md 규칙 5 에 그 예외를 명시해 판단 기준을 한 곳에 모은다.
- **확신도**: 낮음 (`WxUI` 등 다른 모듈의 VM 도 같은 패턴이라 프로젝트 차원의 묵인된 예외일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/WxGame.Build.cs`
- **훑은 파일**: `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Controller/WxEnemyController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`
- **미검토 / 한계**:
  - 이 모듈은 8개 도메인 플러그인의 API 를 호출하는 접착 코드가 대부분이다. 호출되는 쪽(`UWxInventoryManagerComponent::GrantItems`, `UWxRewardLibrary::GrantReward`, `UWxInteractionScannerComponent`, `UWxAbilitySystemComponent` 등)의 내부 계약은 시그니처·주석 수준으로만 확인했고 각 플러그인 리뷰에 맡긴다. 예외로 `UWxViewModel::RequestImageAsync`(WxUI)와 `GetAbilityInputActions`(WxCombat)는 이 모듈의 의심 지점(아이콘 요청 경합, 클라 입력 바인딩 타이밍)을 검증하느라 구현까지 읽었고, 둘 다 문제 없음을 확인했다.
  - `UWxMetaHumanComponent` 의 LOD 매핑·그룸 바인딩은 MetaHuman 어셈블 BP 규약에 의존해 코드만으로 정오를 판정할 수 없어 구조 검토에 그쳤다.
  - Experience 파이프라인의 PIE 다중 세션 참조 카운팅은 코드상 대칭성만 확인했고 실제 2인 PIE 종료 순서는 실행 검증하지 않았다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 66파일 — `/module-review`로 갱신*
