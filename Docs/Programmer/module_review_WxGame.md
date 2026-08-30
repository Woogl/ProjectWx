# WxGame — 코드 리뷰

> 지난 리뷰에서 지적한 Experience 로드의 심각 결함 2건(미해석 플러그인 무시, 종료 후 늦은 콜백)이 실패 상태·종료 가드로 정리돼 프레임워크 부팅 경로의 건강도가 뚜렷이 좋아졌다. 이번에는 Framework에 더해 Character·Controller·MVVM·AbilitySystem·Cheat까지 범위를 넓혀, 캐릭터 초기화와 뷰모델 수명 경로를 중심으로 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 클라이언트에서 SPD 초기값이 이동 속도에 반영되지 않을 수 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:207`
- **범주**: 버그/정확성
- **문제**: `InitAbilitySystem`은 SPD 변경 델리게이트를 구독만 하고(207~213행), 구독 시점의 현재 SPD 값을 한 번 적용하는 경로가 없다. 서버는 구독 뒤 `GiveAbilitySet`이 GE를 적용해 콜백이 반드시 한 번 돌지만, 클라이언트에서 이 경로를 여는 것은 `AWxPlayerCharacter::OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:62`)뿐이다. PlayerState 참조가 아직 unmapped여서 RepNotify가 지연되는 사이 AttributeSet 복제가 먼저 도착하면 SPD 변경 콜백은 이미 지나간 뒤이고, 이후 SPD가 다시 바뀌지 않는 한 클라이언트의 `MaxWalkSpeed`는 배율이 빠진 `BaseWalkSpeed` 그대로 남아 서버와 이동 속도가 어긋난다. 같은 파일이 Ragdoll·Death 태그에 대해서는 "late join 시 구독보다 먼저 태그가 실려 왔을 수 있어 1회 즉시 확인한다"(66~70행, 76~80행)는 대칭 처리를 이미 하고 있어, SPD만 그 처리가 빠진 형태다.
- **제안**: 구독 직후 `AbilitySystemComponent->GetNumericAttribute(GetSPDAttribute())`로 현재 값을 읽어 `MaxWalkSpeed`를 한 번 계산해 둔다(태그 쪽과 같은 "구독 + 1회 시드" 패턴). 겸사겸사 `BaseWalkSpeed`(`Source/WxGame/Character/WxCharacterBase.h:124`)에 기본값 초기화를 넣어 미초기화 읽기 가능성을 원천 차단한다.
- **확신도**: 중간

### 2. 🟡 `Team` 이 복제되지 않아 런타임 팀 변경이 클라이언트 판정과 어긋난다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:120`
- **범주**: 설계/구조
- **문제**: `Team`은 `EditDefaultsOnly` 일반 프로퍼티라 복제 지정자가 없고, `AWxCharacterBase`는 `GetLifetimeReplicatedProps`도 오버라이드하지 않는다. 그래서 `SetGenericTeamId`(`Source/WxGame/Character/WxCharacterBase.cpp:168`)로 런타임에 바꾼 팀은 서버에만 남는다. 실제 런타임 변경 지점이 존재한다 — 소환수는 스폰 직후 소환자의 팀을 물려받는데(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:200`), 이 스폰은 권위 전용이라 클라이언트의 소환수는 자기 클래스 기본 팀(예: `AWxEnemyCharacter`의 `EWxTeam::Enemy`)에 머문다. 팀 판정은 클라이언트에서도 도는 경로가 있어(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:263`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:81`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_Team.cpp:18`), 소환수가 클라에서만 적으로 보여 록온 후보에 잡히거나 로컬 피격 판정이 서버와 갈릴 수 있다.
- **제안**: `Team`을 `UPROPERTY(Replicated)`로 올리고 `GetLifetimeReplicatedProps`에 등록한다. 팀이 항상 클래스 기본값 고정이라는 것이 의도라면, `SetGenericTeamId`를 노옵으로 두거나 권위 전용임을 주석·로그로 명시해 소환 경로가 조용히 어긋나지 않게 한다.
- **확신도**: 중간

### 3. 🟡 동일 GameFeatureAction 인스턴스가 중복 수집되면 수명주기 훅을 중복 실행한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:366`
- **범주**: 버그/정확성
- **문제**: `CollectActions`는 Experience 본체와 모든 ActionSet의 액션 포인터를 그대로 누적하며 중복을 제거하지 않는다(368~390행). 두 데이터 에셋의 검증도 널 항목만 본다(`Source/WxGame/Framework/WxExperienceDefinition.cpp:21`, `Source/WxGame/Framework/WxExperienceActionSet.cpp:16`). 같은 ActionSet을 두 번 참조하거나 같은 액션이 두 배열에 들어가면 `FinishExperienceLoad`(313행)의 `OnGameFeatureRegistering`·`OnGameFeatureLoading`·`OnGameFeatureActivating`과 `EndPlay`(59행)의 비활성·해제 훅이 같은 인스턴스에 두 번 호출된다. `UWxGameFeatureAction_AddComponents`는 이때 같은 컴포넌트 요청 핸들을 두 벌 쌓고(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:151`), `OnGameFeatureActivating`이 `ContextHandles.FindOrAdd` 뒤 `GameInstanceStartHandle`을 덮어써(52~55행) 앞선 `OnStartGameInstance` 구독이 해제되지 못한 채 남는다.
- **제안**: `CollectActions`에서 순서를 보존한 포인터 기준 중복 제거를 적용한다. 덧붙여 `IsDataValid`에서 같은 Experience 안의 중복 ActionSet·중복 액션 참조를 검증 오류로 올리면 데이터 단계에서 걸린다.
- **확신도**: 높음

### 4. 🟡 Experience 로드 실패가 외부 소비자에게 노출되지 않아 대기 콜백이 조용히 사라진다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:127`
- **범주**: 설계/구조
- **문제**: 실패 시 매니저는 `Failed`로 전이하며 등록된 델리게이트를 `Clear()`로 버린다(240~243행, 287~291행). 그런데 `CallOrRegister_OnExperienceLoaded`는 실패 여부를 구분하지 않고 `IsExperienceLoaded()`가 거짓이면 새 델리게이트를 계속 저장한다(129~136행). `LoadState`는 private이고 실패 델리게이트도 없어(`Source/WxGame/Framework/WxExperienceManagerComponent.h:90`), 소비자는 "로드 중"과 "영구 실패"를 구분할 수단이 없다. 실제 결과로 `AWxGameMode::InitGameState`가 건 대기 콜백이 버려지고(`Source/WxGame/Framework/WxGameMode.cpp:30`), `HandleStartingNewPlayer_Implementation`의 스폰 게이트가 영영 열리지 않아(`Source/WxGame/Framework/WxGameMode.cpp:56`) 플레이어는 폰 없이 남는다 — 로그 한 줄 외에 아무 신호가 없다.
- **제안**: 읽기 전용 로드 상태 접근자나 `OnExperienceLoadFailed` 델리게이트를 공개하고, `Failed` 상태에서의 등록은 즉시 실패로 되돌린다. GameMode가 실패를 알 수 있어야 부팅 실패 화면·재시도 같은 상위 정책을 붙일 수 있다.
- **확신도**: 높음

### 5. 🟡 ViewModel 인스턴스 멤버 함수에 `BlueprintCallable` 을 사용한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Item.h:47`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 규칙 5는 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action 팩토리 함수에만 허용한다. 본 모듈의 `UWxViewModel_Item::RequestUseConsumable`(`WxViewModel_Item.h:47`), `UWxViewModel_InteractionList::RequestInteract`·`RequestCycle`(`WxViewModel_InteractionList.h:44`, `:47`), `UWxViewModel_Inventory::SetCurrentCategory`(`WxViewModel_Inventory.h:83`), `UWxViewModel_Dialogue::RequestAdvance`(`WxViewModel_Dialogue.h:29`)은 모두 그 대상이 아닌 인스턴스 멤버 함수다. 같은 위반이 `Docs/Programmer/module_review_WxUI.md` 3번으로도 남아 있어, 모듈 하나의 문제가 아니라 규칙 자체를 다시 정할지 결정할 시점이다.
- **제안**: 규칙을 지킬 것이라면 UMG의 호출 진입점을 BP Function Library로 옮기거나 `BlueprintSetter`/`FieldNotify` 경로로 대체한다. MVVM 특성상 예외가 불가피하다고 판단되면 `CLAUDE.md` 규칙 5에 "ViewModel 요청 함수" 예외를 명시해 규칙과 코드를 일치시킨다.
- **확신도**: 높음

### 6. 🟢 델리게이트 콜백 이름이 프로젝트 접두사 규칙과 다르다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:22`
- **범주**: 규칙 위반
- **문제**: `WxHandleDeactivationPauserCompleted`는 `FGameFeatureDeactivatingContext` 생성자에 콜백으로 전달되지만(71행), 규칙 4의 `Handle` 접두사 대신 `WxHandle`로 시작한다. 파일 내 static 함수라 `Wx` 타입 접두사 규칙의 대상도 아니다.
- **제안**: `HandleDeactivationPauserCompleted`로 바꾼다.
- **확신도**: 높음

### 7. 🟢 "도착 신호 관찰 → 연결" 보일러플레이트가 뷰모델마다 복제돼 있다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:12`
- **범주**: 중복/복잡도
- **문제**: `UWxViewModel_Inventory`(12~29행, 191~210행)와 `UWxViewModel_InteractionList`(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:9`, 101~120행)는 `ObservedController` + `ReadyHandle` 멤버, `StartObserving`/`StopObserving`/`Handle*Ready`(오너 비교 후 `StopObserving`→`Initialize`)까지 구조가 한 글자 차이로 같다. 리졸버 `CreateInstance`도 세 곳(`WxViewModel_Inventory.cpp:212`, `WxViewModel_InteractionList.cpp:154`, `WxViewModel_BossCharacter.cpp:76`)이 같은 모양이다. 컴포넌트 종류가 늘 때마다 같은 코드를 복제해야 하고, 한쪽만 고치면 조용히 갈린다.
- **제안**: "PC에서 컴포넌트를 찾고, 없으면 클래스 차원 ready 신호를 오너로 필터링해 기다린다"를 템플릿 헬퍼(또는 중간 베이스 뷰모델)로 한 번만 쓰고 각 VM은 `Initialize`만 구현하게 한다.
- **확신도**: 높음

### 8. 🟢 일부 뷰모델의 `Initialize` 에만 재초기화 가드가 빠져 있다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:8`
- **범주**: 버그/정확성
- **문제**: 이 모듈과 베이스의 다른 뷰모델은 `Initialize` 첫머리에서 `Deinitialize()`를 불러 이전 구독을 끊는다(`WxViewModel_Item.cpp:20`, `:41`, `WxViewModel_Inventory.cpp:38`, `WxViewModel_InteractionList.cpp:35`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:15`). 반면 `UWxViewModel_Dialogue::Initialize`(8~21행)와 `UWxViewModel_Quest::Initialize`(`Source/WxGame/MVVM/WxViewModel_Quest.cpp:10`)에는 그 호출이 없어, 두 번째 호출이 오면 `CachedSession`/`CachedQuestComponent`만 덮이고 이전 대상의 구독은 남는다. 지금은 각 리졸버가 VM당 한 번만 부르므로 잠재적이지만, 두 VM만 규약이 다르다는 점 자체가 다음 수정에서 밟기 쉬운 함정이다.
- **제안**: 두 `Initialize`에도 첫머리 `Deinitialize()` 호출을 넣어 나머지 뷰모델과 규약을 맞춘다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`
- **규칙 대조 결과**: 64개 소스 전부 첫 줄 저작권 표기가 있고(`WxPlayerCharacter.h`·`WxEnemyCharacter.h`는 앞에 UTF-8 BOM이 붙어 있으나 내용은 동일), 람다·`FORCEINLINE`·인라인 정의는 하나도 없다. 델리게이트 바인딩 16곳 모두 `Handle` 접두사를 지킨다(예외는 6번). 위반은 5·6번 두 건이다.
- **미검토 / 한계**: Experience·ActionSet·GameFeature 데이터 에셋의 실제 조합과 AssetManager 스캔 설정, BP/WBP 내부 위젯·이벤트 그래프는 범위 밖이다. `UWxMetaHumanComponent`가 `OnRegister` 안에서 부착 컴포넌트를 생성·등록하고 `CreationMethod`를 물려주는 부분은 엔진 등록 순서에 민감하지만 코드에 의도가 명시돼 있고 에디터 실행 없이는 검증할 수 없어 발견으로 올리지 않았다. 네트워크 관련 지적(1·2번)도 실제 멀티 세션 검증 없이 코드 근거만으로 판단했다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 64파일 — `/module-review`로 갱신*
