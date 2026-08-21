# WxGame — 코드 리뷰

> 조립 전용 모듈답게 각 파일이 얇고 책임 경계가 또렷하다. Lyra Experience 파이프라인 이식부는 서버/클라 대칭·PIE 다중 세션까지 충실히 옮겨져 있고, 저작권 헤더·람다 금지·인라인 정의 금지·델리게이트 `Handle` prefix 는 사실상 전 파일이 지키고 있다(람다·FORCEINLINE 0건). 이번 리뷰는 66개 소스 전부를 훑고 Framework(Experience 부트스트랩)·Character 계층·MVVM 뷰모델·어빌리티의 cpp 로직까지 내려가 확인했다. 직전 리뷰에서 지적됐던 CMC 의 매 이동 갱신 ASC 탐색과 ASC 무력 널 가드는 현재 코드에서 해소돼 있어 제외했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 Experience 미확정이면 폰이 영영 스폰되지 않는데 진단은 Warning 한 줄뿐
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:82`, `Source/WxGame/Framework/WxGameMode.cpp:55`
- **범주**: 버그/정확성
- **문제**: `ResolveExperienceId()` 가 무효 ID 를 돌려주면 `SetCurrentExperience` 가 `LoadState = Unloaded` 인 채로 빠져나간다. 그 뒤 `HandleStartingNewPlayer_Implementation` 은 `IsExperienceLoaded()` 가 false 라 매번 조기 반환하고, `HandleExperienceLoaded` 는 영원히 호출되지 않는다. 결과는 폰도 입력도 없는 죽은 세션인데 남는 단서는 `"Experience 미설정. 프레임워크 컴포넌트가 주입되지 않음."` Warning 한 줄이다. 이 문구가 실제 증상(폰 미스폰)을 지목하지 않아, WorldSettings 에 Experience 를 안 꽂은 신규 맵에서 원인 추적이 오래 걸린다. `GetDefaultPawnClassForController_Implementation` 도 이 경우 "InitGameState 가 이미 경고한 상태다"라며 침묵한다(`WxGameMode.cpp:41`).
- **제안**: 확정 실패를 Error 로 올리고 문구에 결과를 명시한다(예: "Experience 를 확정하지 못해 이 세션에서는 플레이어 폰이 스폰되지 않는다 — WorldSettings/GameMode DefaultExperience 확인"). 또는 `HandleStartingNewPlayer` 조기 반환 시 1회 한정으로 대기 사유를 로그에 남긴다.
- **확신도**: 높음

### 2. 🟡 처치 보상이 킬러가 아니라 항상 0번 플레이어에게 지급된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:91`
- **범주**: 설계/구조
- **문제**: `HandleDeath` 는 `HasAuthority()` 로 서버 권위를 지킨 뒤 `UGameplayStatics::GetPlayerController(this, 0)` 로 보상 수령자를 정한다. 데디케이티드 서버에서 인덱스 0 은 "먼저 접속한 아무 플레이어"이고 리슨 서버에서는 항상 호스트다. 즉 누가 죽였는지와 무관하게 한 명이 전부 가져간다. 이 모듈은 나머지 곳(Mixed/Full 복제 모드, `OnInteracted` 의 실제 instigator 재판정, `WxAbility_Interact` 의 서버 거리·자격 검증)에서 멀티 권위 모델을 꽤 엄격히 지키고 있어 이 지점만 단일 플레이어 가정에 묶여 있다.
- **제안**: 가해자를 이미 아는 경로가 있다 — `HandleIncomingDamageChanged` 가 쓰는 `FGameplayEffectContext::GetInstigator()`(`WxEnemyCharacter.cpp:64-65`). 마지막 유효 가해자를 사망 시점까지 들고 있다가 그 컨트롤러에 지급하거나, 최소한 "단일 플레이어 전제"임을 주석으로 못 박고 TODO 로 남긴다.
- **확신도**: 중간 (현 단계가 싱글플레이 스코프라면 의도된 단순화일 수 있음)

### 3. 🟡 Experience 로드 파이프라인에 취소 경로가 없다 — 세션 종료 중 로드가 계속 밀고 나간다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:28-66`, `:172-186`, `:189-217`
- **범주**: 버그/정확성
- **문제**: `StartExperienceLoad` 가 받은 `FStreamableHandle` 은 지역 변수로 즉시 버려지고(`:172`), 컴포넌트는 진행 중인 로드를 취소할 수단을 갖지 않는다. `EndPlay` 는 `LoadState == Loaded` 일 때만 액션을 비활성화하고 `GameFeaturePluginURLs` 만 되돌리므로, 로드 중 종료(PIE 정지·레벨 전환)에는 두 구멍이 난다.
  - `Loading` 중 종료: `GameFeaturePluginURLs` 가 아직 비어 있어 `EndPlay` 는 아무것도 되돌리지 않는다. 이후 번들 콜백이 살아 있으면 `HandleExperienceAssetsLoaded` 가 `NotifyOfPluginActivation`(`:213`)으로 카운트를 올리고 플러그인을 활성화하는데, 그 참조를 내릴 주체는 이미 사라졌다. 그러면 다음 PIE 시작 때 `UWxExperienceManager::OnPlayInEditorBegun` 의 `ensure(GameFeaturePluginRequestCountMap.IsEmpty())`(`WxExperienceManager.cpp:11`)가 터지고 플러그인이 세션을 넘겨 켜진 채 남는다.
  - `LoadingGameFeatures` 중 종료: `EndPlay` 가 참조를 내려 `DeactivateGameFeaturePlugin` 을 부른 뒤에도 in-flight `HandleGameFeaturePluginLoaded` 가 `FinishExperienceLoad` 를 돌려(`:227-230`) 이미 정리된 월드에 액션을 활성화하고 `OnExperienceLoaded` 를 방송한다.
  - 실제로는 EndPlay 직후 컴포넌트가 garbage 로 마킹되면 `CreateUObject` 약참조 바인딩이 콜백을 걸러 주는 경우가 많아 항상 재현되지는 않는다 — 즉 타이밍에 기대고 있는 상태다.
- **제안**: 핸들을 멤버로 보관하고 `EndPlay` 에서 `CancelHandle()` + `LoadState` 를 종료 상태로 전이시켜, `HandleExperienceAssetsLoaded`·`HandleGameFeaturePluginLoaded`·`FinishExperienceLoad` 진입부에서 조기 반환하게 한다. `Loading` 중 종료 경로에서도 이미 요청한 플러그인 참조가 있으면 함께 되돌린다.
- **확신도**: 중간

### 4. 🟢 Public 헤더가 Private 의존 모듈의 헤더를 include 한다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.h:7`, `Source/WxGame/WxGame.Build.cs:11`·`:43`
- **범주**: 설계/구조
- **문제**: `WxGame.Build.cs:11` 이 `PublicIncludePaths.AddRange(ModuleDirectory)` 로 모듈 전 헤더를 공개하는데 `MetaHumanSDKRuntime` 은 `PrivateDependencyModuleNames`(`:43`)에 있다. 그런데 공개된 `WxMetaHumanComponent.h` 가 그 모듈의 `MetaHumanComponentUE.h` 를 include 한다. 현재 유일한 외부 소비자인 `WxEditor` 는 `Framework/WxExperienceManager.h` 만 포함해(`Source/WxEditor/WxEditor.cpp:8`) 문제가 드러나지 않지만, 에디터 툴이나 GameFeature 플러그인이 캐릭터 계층을 건드리는 순간 "cannot open include file" 로 터진다.
- **제안**: `MetaHumanSDKRuntime` 을 `PublicDependencyModuleNames` 로 올린다(`.cpp` 에서만 쓰이는 `HairStrandsCore` 는 그대로 둔다). 또는 이 컴포넌트 헤더가 엔진 베이스를 노출하지 않도록 감싼다.
- **확신도**: 중간

### 5. 🟢 `InitAbilitySystem()` 이 멱등하지 않다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:192-207`, `Source/WxGame/Character/WxCharacterBase.h:124`, `Source/WxGame/Character/WxEnemyCharacter.cpp:46-53`
- **범주**: 버그/정확성
- **문제**: 이 함수는 헤더 주석대로 서버 `PossessedBy`(`WxCharacterBase.cpp:104-109`)와 클라 `OnRep_PlayerState`(`WxPlayerCharacter.cpp:60-65`) 양쪽의 진입점인데 재호출 방어가 없다. 두 번 불리면 (a) SPD 어트리뷰트 델리게이트가(에너미는 IncomingDamage 델리게이트까지) 중복 구독되고, (b) `BaseWalkSpeed = MaxWalkSpeed` 재기준화가 이미 SPD 배율이 곱해진 값을 새 기준으로 잡아 이동 속도가 누적 드리프트하며, (c) 서버 경로에서는 `GiveAbilitySet()`(`UWxAbilitySystemComponent::GiveAbilitySet` 에 중복 방어 없음)이 어빌리티 세트를 두 번 부여한다. 현 프로젝트 코드에 명시적 재빙의 경로는 없지만(`Possess`/`UnPossess` 직접 호출 0건), PlayerState 포인터가 null→유효로 두 번 복제되는 클라 경로와 폰 재사용 리스폰 도입 시 조용히 깨진다. 덧붙여 `BaseWalkSpeed` 는 초기화자가 없다(UObject 메모리가 0 으로 채워져 현재는 무해).
- **제안**: 1회 실행 가드를 두거나, 구독을 `PostInitializeComponents`(래그돌/사망 구독과 같은 자리)로 옮기고 `InitAbilitySystem` 은 `RefreshAbilityActorInfo` + `GiveAbilitySet` 만 남긴다. `BaseWalkSpeed` 에는 명시적 초기화자를 준다.
- **확신도**: 낮음 (현 리스폰 설계에서는 재호출 경로가 확인되지 않아 의도된 전제일 수 있음)

### 6. 🟢 관찰형 뷰모델의 부트스트랩 코드가 통째로 중복된다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:12-29`·`:191-210`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:9-26`·`:101-120`
- **범주**: 중복/복잡도
- **문제**: "이미 붙어 있으면 즉시 연결, 아니면 클래스 차원 Ready 신호를 구독 → 오너가 내 PC 인지 확인 → StopObserving 후 Initialize" 패턴이 두 VM 에서 구조·변수명·주석 문장까지 동일하게 반복된다(`StartObserving`/`HandleXReady`/`StopObserving` + `ObservedController`/`XReadyHandle`). 헤더 주석도 서로를 "같은 구조"라고 명시한다. 세 번째 늦게-도착하는 PC 컴포넌트가 생기면 그대로 한 벌 더 복사된다.
- **제안**: `UWxViewModel` 파생 공통 베이스나 템플릿 헬퍼로 "PC 소유 컴포넌트 도착 대기" 로직을 한 곳에 모은다(구독 대상 정적 델리게이트와 연결 콜백만 파생이 지정). 지금 당장 고칠 필요는 없으나 세 번째 사례가 생기면 그때 반드시 접는다.
- **확신도**: 높음

### 7. 🟢 인벤토리 스택이 바뀔 때마다 전체 아이템 VM 목록을 재구축·재방송한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:98-165`
- **범주**: 성능/안전
- **문제**: `HandleStackChanged` 는 어떤 변경이든 `RefreshAllItems()` 를 부르고, 이 함수는 인벤토리 전 인스턴스를 순회하며 기존 VM 을 `AllItems` 선형 탐색으로 매칭한 뒤(O(N²)) 슬롯 구성이 그대로여도 `AllItems`·`CategorizedItems` 를 항상 브로드캐스트한다. 골드 같은 재화 획득처럼 인벤토리 창이 열려 있지도 않은 변경까지 ListView 재빌드를 유발한다. 또 `Delta > 0` 마다 토스트용 `UWxViewModel_Item` 을 새로 만들어 인벤토리 델리게이트에 구독시키는데(`:108-112`), 교체된 이전 VM 은 `Deinitialize()` 없이 버려져 GC(`UWxViewModel::BeginDestroy` → `Deinitialize`) 전까지 구독이 남는다.
- **제안**: 인스턴스 집합이 실제로 바뀐 경우에만 `AllItems` 를 재구축·재방송하고, 수량만 바뀐 변경은 해당 항목 VM 의 필드 갱신으로 끝낸다. 토스트 VM 은 교체 직전 이전 인스턴스를 `Deinitialize()` 한다.
- **확신도**: 낮음 (항상 브로드캐스트는 "ListView 엔트리 재연결" 목적으로 주석에 명시된 의도적 선택이라, 실측 없이 되돌리면 UMG 쪽이 깨질 수 있음)

### 8. 🟢 ViewModel 진입 함수에 `BlueprintCallable` 사용
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:51`·`:54`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:83`, `Source/WxGame/MVVM/WxViewModel_Item.h:47`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h:39`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 규칙 5 는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수로 한정한다. 위 5개는 WBP 가 Enhanced Input·MVVM 이벤트 바인딩으로 직접 호출하는 VM 요청 함수라 어느 쪽도 아니다. 다만 `UWxViewModel_Inventory::SetCurrentCategory` 는 `BlueprintSetter` 로 지정돼 엔진이 `BlueprintCallable` 을 요구하는 케이스라 성격이 다르다.
- **제안**: 규칙을 그대로 지킬 것이라면 `UWxUILibrary` 류 파사드를 경유시키고, 반대로 "MVVM 바인딩 수신 진입점"을 허용 범주로 볼 것이라면 CLAUDE.md 규칙 5 에 그 예외(`BlueprintSetter` 포함)를 명시해 판단 기준을 한 곳에 모은다.
- **확신도**: 낮음 (`WxUI` 등 다른 모듈의 VM 도 같은 패턴이라 프로젝트 차원의 묵인된 예외일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/WxGame.Build.cs`
- **훑은 파일**: `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Controller/WxEnemyController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`
- **미검토 / 한계**:
  - 이 모듈은 9개 도메인 플러그인의 API 를 호출하는 접착 코드가 대부분이다. 호출되는 쪽(`UWxInventoryManagerComponent::GrantItems`, `UWxInteractionScannerComponent`, `UWxNameplateComponent::InitializeViewModels` 등)의 내부 계약은 시그니처·주석 수준으로만 확인했고 각 플러그인 리뷰에 맡긴다. 예외로 발견 2·5·7 의 근거 검증을 위해 `UWxRewardLibrary::GrantReward`, `UWxInventoryManagerComponent::FindInventory`, `UWxAbilitySystemComponent::GiveAbilitySet`, `UWxViewModel::BeginDestroy`(WxUI) 구현까지 읽었다.
  - `UWxMetaHumanComponent` 의 LOD 매핑·그룸 바인딩은 MetaHuman 어셈블 BP 규약에 의존해 코드만으로 정오를 판정할 수 없어 구조 검토에 그쳤다.
  - 발견 3 의 두 실패 시나리오는 코드 경로 추적으로 도출했고 실제 PIE 정지 타이밍으로 재현 검증하지는 않았다. `CreateUObject` 약참조 바인딩이 상황에 따라 콜백을 걸러 주기 때문에 재현 난이도가 있다.
  - BP/WBP 내부 바인딩 구성(어느 위젯이 어느 VM 필드를 읽는지)은 범위 밖이라, 발견 7 의 브로드캐스트 축소가 UMG 에 미치는 영향은 판단하지 않았다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 66파일 — `/module-review`로 갱신*
