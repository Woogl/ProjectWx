# WxGame — 코드 리뷰

> Experience 부팅 파이프라인은 실패 상태·종료 가드·월드 한정 컨텍스트까지 갖춘 견고한 이식이고 캐릭터·MVVM 계층도 수명주기 정리가 일관되지만, 로드 실패 이후의 복구 경로와 GameFeature 활성 콜백의 재진입에 정합성 구멍이 남아 있다. README·Build.cs 를 진입점으로 Framework 전부와 Character·Controller·Player·MVVM·AbilitySystem 의 수명주기·권위·입력 라우팅을 코드 단위로 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 Experience 로드 실패를 소비자가 관찰할 수 없어 스폰 게이트가 영구히 닫힌다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:127`
- **범주**: 설계/구조
- **문제**: 플러그인 이름 해석 실패(`:240`)와 활성 실패(`:287`)는 `LoadState` 를 `Failed` 로 옮기면서 `OnExperienceLoaded` 를 비운다. 그런데 `CallOrRegister_OnExperienceLoaded` 는 `IsExperienceLoaded()` 만 보고 실패 상태를 "아직 로딩 중"으로 취급해 델리게이트를 다시 저장한다. `LoadState` 는 private 이고 실패 델리게이트도 상태 조회 API 도 없어(`Source/WxGame/Framework/WxExperienceManagerComponent.h:90`) `AWxGameMode` 는 실패와 진행 중을 구분하지 못한다. 결과적으로 `HandleStartingNewPlayer_Implementation` 의 로드 게이트(`Source/WxGame/Framework/WxGameMode.cpp:60`)는 영원히 열리지 않아 폰도 HUD 도 없는 상태로 멈추고, 단서는 로그 한 줄뿐이다.
- **제안**: 성공/실패를 함께 싣는 완료 델리게이트를 두거나 `OnExperienceLoadFailed` 와 읽기 전용 상태 접근자를 추가한다. 이미 `Failed` 인 상태에서 등록한 소비자에게는 즉시 실패를 전달해, 실패 UI·재시도·세션 종료 정책을 붙일 자리를 만든다.
- **확신도**: 높음

### 2. 🟡 GameFeature 활성 콜백이 동기로 돌아오면 순회 중인 배열을 그 자리에서 리셋한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:254`
- **범주**: 버그/정확성
- **문제**: `HandleExperienceAssetsLoaded` 는 `GameFeaturePluginURLs` 를 ranged-for 로 돌면서 `LoadAndActivateGameFeaturePlugin` 을 호출한다. 엔진의 `UGameFeaturesSubsystem::ChangeGameFeatureTargetState` 는 플러그인이 허용되지 않거나 프로토콜 옵션 갱신이 실패하면 완료 델리게이트를 **동기로** 실행한다. 마지막 URL 이 그렇게 실패하면 `HandleGameFeaturePluginLoaded` 가 아직 루프 안에서 `ReleaseGameFeaturePluginRequests(false)`(`:289`)를 타고, 그 함수가 같은 배열을 `Reset()` 한다(`:309`). 개발 빌드에선 `TArray` 의 ranged-for 검사가 "Array has changed during ranged-for iteration!" ensure 를 띄워, 진짜 원인인 활성 실패 로그를 덮는다.
- **제안**: 루프를 `GameFeaturePluginURLs` 의 지역 복사본 위에서 돌리거나, 실패 정리를 `HandleGameFeaturePluginLoaded` 안에서 즉시 하지 말고 다음 틱으로 미룬다.
- **확신도**: 중간

### 3. 🟡 중복 ActionSet 참조가 같은 액션 수명주기와 기본 지급을 두 번 실행한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:366`
- **범주**: 버그/정확성
- **문제**: `CollectActions` 는 Experience 본체와 ActionSet 들의 액션 포인터를 중복 제거 없이 `Add` 로 쌓는다. 한 Experience 가 같은 ActionSet 을 두 번 참조하면 동일 액션 인스턴스에 등록·로딩·활성 훅이 반복 호출되고, `UWxGameFeatureAction_AddComponents` 는 `ContextHandles.FindOrAdd` 로 같은 저장소를 재사용하면서 `GameInstanceStartHandle` 을 덮어써(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:54`) 먼저 등록한 전역 델리게이트를 회수할 수 없게 된다. 같은 원인으로 `AWxGameMode::GrantDefaultInventory` 의 `DefaultInventoryItems` 도 두 번 지급된다(`Source/WxGame/Framework/WxGameMode.cpp:124`). `UWxExperienceDefinition::IsDataValid` 는 빈 항목만 보고 중복은 잡지 않는다.
- **제안**: `CollectActions` 에서 포인터 기준 순서 보존 중복 제거를 하고, `UWxExperienceDefinition::IsDataValid` 에서 같은 ActionSet 이 두 번 실린 경우를 에디터 에러로 드러낸다.
- **확신도**: 높음

### 4. 🟡 마지막 PIE 요청자가 활성에 실패하면 이미 활성된 플러그인이 남는다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:303`
- **범주**: 버그/정확성
- **문제**: 정리 조건이 `RequestToDeactivatePlugin(...) && (bDeactivateAllRequestedPlugins || bWasActivated)` 라, 참조 카운트가 0 이 되어도 **이 컴포넌트의** 활성이 성공하지 않았으면 비활성화를 건너뛴다. 다중 PIE 에서 A 가 플러그인을 활성화한 뒤 B 가 같은 URL 을 로드하는 동안 A 가 먼저 종료되면 A 의 카운트는 2→1 이라 비활성화되지 않고, 이어서 B 의 활성화가 실패하면 B 가 1→0 으로 마지막 요청을 풀지만 `bWasActivated` 가 거짓이라 `DeactivateGameFeaturePlugin` 을 부르지 않는다. 요청자는 0 명인데 플러그인은 활성인 채로 에디터 세션에 남는다.
- **제안**: 마지막 참조 해제 시에는 이 컴포넌트의 성공 여부가 아니라 플러그인의 전역 활성 상태를 기준으로 비활성화한다. 가장 단순하게는 카운트가 0 이면 무조건 비활성화를 요청하고, 필요하면 `UWxExperienceManager` 가 URL 별 활성 성공 여부까지 함께 추적하게 한다.
- **확신도**: 높음

### 5. 🟡 SPD 구독 직후 현재 값을 한 번 적용하지 않아 클라이언트 이동 속도에 배율이 빠질 수 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:221`
- **범주**: 버그/정확성
- **문제**: `InitAbilitySystem` 은 SPD 변경 델리게이트만 등록하고 등록 시점의 SPD 를 적용하지 않는다. 서버는 곧이어 `GiveAbilitySet()` 이 변경 이벤트를 일으켜 문제가 드러나지 않지만, 소유 클라이언트는 `OnRep_PlayerState` 에서 뒤늦게 이 함수를 부른다(`Source/WxGame/Character/WxPlayerCharacter.cpp:66`). 그전에 AttributeSet 초기값이 복제됐다면 그 변경 이벤트는 이미 지나갔고, 다음 SPD 변경 전까지 `MaxWalkSpeed` 가 `BaseWalkSpeed` 로 남아 서버와 어긋난다. 같은 초기 복제 문제를 Ragdoll·Death 태그는 구독 직후 현재 값을 1 회 확인해 메우고 있어(`:68`, `:78`) SPD 경로만 비대칭이다.
- **제안**: 델리게이트 등록 블록 안에서 현재 SPD 를 읽어 `HandleSPDAttributeChanged` 와 같은 계산을 한 번 실행한다.
- **확신도**: 중간

### 6. 🟢 폰이 바뀌어도 이전 Enhanced Input MappingContext 가 남는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:88`
- **범주**: 버그/정확성
- **문제**: `SetupPlayerInputComponent` 는 LocalPlayer 서브시스템에 `InputConfig->MappingContext` 를 추가하지만, 저장소 전체에 `RemoveMappingContext` 호출이 한 건도 없다. 빙의 해제·종료·입력 구성이 다른 폰(폰 없는 Experience 의 스펙테이터 포함)으로의 전환 어디에서도 이전 컨텍스트가 걷히지 않아, 키를 계속 소비하거나 새 폰의 액션과 충돌할 수 있다.
- **제안**: 추가한 MappingContext 와 대상 LocalPlayer 서브시스템을 기억해 빙의 해제·`EndPlay` 에서 대칭적으로 제거하고, 재설정 시 이전 컨텍스트를 먼저 정리한다.
- **확신도**: 중간

### 7. 🟢 `HandleDeathTagChanged` 가 베이스의 동명 비가상 함수를 가린다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.h:63`
- **범주**: 중복/복잡도
- **문제**: `AWxEnemyCharacter::HandleDeathTagChanged` 는 `AWxCharacterBase::HandleDeathTagChanged`(`Source/WxGame/Character/WxCharacterBase.h:104`)와 이름·시그니처가 같은 별개의 비가상 함수다. 베이스는 `PostInitializeComponents` 에서 사망 처리용으로(`Source/WxGame/Character/WxCharacterBase.cpp:75`), 파생은 `BeginPlay` 에서 네임플레이트 갱신용으로(`Source/WxGame/Character/WxEnemyCharacter.cpp:50`) 각각 같은 태그 이벤트에 바인딩한다. 지금은 바인딩이 클래스를 명시해 동작에 문제가 없지만, 파생 클래스 문맥에서 `&ThisClass::HandleDeathTagChanged` 를 쓰면 조용히 베이스 쪽 의도가 사라진다.
- **제안**: 파생 쪽을 역할이 드러나는 이름(예: `HandleDeathTagChangedForNameplate`)으로 바꾸거나, 두 구독을 하나로 합쳐 베이스 콜백이 파생 훅을 부르게 한다.
- **확신도**: 높음

### 8. 🟢 획득 토스트용 ViewModel 이 인벤토리 구독을 남긴 채 버려진다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:108`
- **범주**: 성능/안전
- **문제**: `HandleStackChanged` 는 획득(`Delta > 0`)마다 `UWxViewModel_Item` 을 새로 만들어 `Initialize` 한다. 그 안에서 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 를 구독하는데, `LastAcquiredItem` 이 다음 획득으로 교체될 때 이전 인스턴스에 `Deinitialize` 를 부르지 않는다. 버려진 VM 은 GC 가 회수하기 전까지 구독 목록에 남아, 이후의 모든 스택 변경 브로드캐스트가 그만큼 더 길어진다. 픽업이 잦은 오픈월드에선 GC 주기 사이에 계속 쌓인다.
- **제안**: 토스트용 VM 은 정적 표시 데이터만 채우고 델리게이트 구독은 하지 않게 하거나, `LastAcquiredItem` 교체 직전에 이전 인스턴스를 `Deinitialize` 한다(표시가 끝난 뒤여야 하면 위젯이 소비한 시점을 신호로 삼는다).
- **확신도**: 중간

### 9. 🟢 사용하지 않는 공개 빌드 의존성이 모듈 경계를 실제보다 넓게 보이게 한다
- **위치**: `Source/WxGame/WxGame.Build.cs:34`
- **범주**: 설계/구조
- **문제**: `WxSave` 는 `PublicDependencyModuleNames` 에 있지만 `Source/WxGame` 의 어떤 소스도 WxSave 헤더나 타입을 참조하지 않는다(등장은 주석·README 뿐). `CommonUI`(`:16`) 도 마찬가지로 실제 include 가 없다. 게임 모듈이 도메인 플러그인을 엮는 자리라 의존 목록이 곧 결합 관계 문서로 읽히는데, 실제로는 없는 결합을 선언해 두면 "WxGame 이 세이브를 주도한다"는 오독을 부른다.
- **제안**: 두 항목을 제거한다. 빌드가 깨진다면 그것이 곧 실제 의존이 어디서 들어오는지에 대한 답이므로, 그 지점을 근거로 다시 넣는다.
- **확신도**: 높음

### 10. 🟢 입력 델리게이트 콜백이 `Handle` 접두사 규칙을 따르지 않는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:50`
- **범주**: 규칙 위반
- **문제**: Enhanced Input 에 바인딩되는 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased`(`Source/WxGame/Character/WxPlayerCharacter.cpp:96`, `:100`, `:109`, `:114`, `:115`)는 전부 프로젝트가 소유한 델리게이트 콜백인데 `Handle` 로 시작하지 않는다. `CLAUDE.md` 코딩 규칙 4 는 델리게이트에 바인딩되는 콜백에 `Handle` 접두사를 요구한다. 같은 모듈의 다른 콜백(`HandleSPDAttributeChanged`·`HandleAITargetChanged`·`HandleInventoryReady` 등)은 모두 규칙을 지키고 있어 여기만 예외다.
- **제안**: `HandleMove`·`HandleLook`·`HandleToggleCrouch`·`HandleAbilityInputTriggered`·`HandleAbilityInputReleased` 로 바꾼다. `Jump`·`StopJumping` 은 엔진 가상 함수라 대상이 아니다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxExperienceManager.h`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.h`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.h`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/WxGame.Build.cs`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxBossCharacter.h`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.h`, `Source/WxGame/Controller/WxPlayerController.h`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Controller/WxEnemyController.h`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/Cheat/WxCheatManager.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.h`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.h`
- **이전 리뷰 대비**: 팀(`Team`) 런타임 변경 미복제 지적은 `ReplicatedUsing = OnRep_Team` 도입으로 해소돼 제외했다. ViewModel 인스턴스 함수의 `BlueprintCallable` 지적은 `Request~` 명령 함수를 예외로 두기로 이미 결정된 사안이라 반복하지 않는다(`CLAUDE.md` 에 예외 문구가 없다는 점만 남는다).
- **미검토 / 한계**: Experience·ActionSet·GameFeature 의 실제 데이터 에셋 조합과 BP/WBP 내부 구조는 범위 밖이다. 멀티플레이·다중 PIE 를 실제로 돌려 검증하지 않았으며, 네트워크·수명주기 판단은 C++ 제어 흐름과 UE 5.8 엔진 소스(`GameFeaturesSubsystem.cpp`)의 계약을 근거로 했다. `AWxNpc`·`AWxBossCharacter` 처럼 조립만 하는 얇은 클래스는 훑는 데 그쳤다.

---
*문서 기준 커밋 `cad31360` · 리뷰일 2026-08-31 · 소스 64파일 — `/module-review`로 갱신*
