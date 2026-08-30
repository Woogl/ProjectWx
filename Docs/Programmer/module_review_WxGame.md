# WxGame — 코드 리뷰

> Experience 부팅 경로는 실패 상태와 종료 가드를 갖춰 전반적으로 방어적이지만, 멀티플레이 팀·속도 동기화와 PIE GameFeature 참조 정리에 정합성 결함이 남아 있다. README와 Build.cs를 진입점으로 Framework·Character·MVVM·AbilitySystem의 수명주기와 권위 경로를 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 런타임 팀 변경이 복제되지 않아 클라이언트 판정이 서버와 갈린다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:120`
- **범주**: 버그/정확성
- **문제**: `Team`은 `EditDefaultsOnly` 일반 프로퍼티이고 `AWxCharacterBase`에는 이를 등록하는 `GetLifetimeReplicatedProps`가 없다. 그런데 `SetGenericTeamId`는 런타임 값을 실제로 바꾸며(`Source/WxGame/Character/WxCharacterBase.cpp:168`), 소환 경로는 서버에서 새 액터에 소환자의 팀을 주입한다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:200`). 따라서 소환수의 서버 팀과 클라이언트의 클래스 기본 팀이 다를 수 있고, 클라이언트에서 수행되는 록온 필터·투사체/무기 적대 판정이 서버 결과와 어긋난다.
- **제안**: `Team`을 `ReplicatedUsing`으로 복제하고 `OnRep`에서 팀 변경 소비자에게 갱신을 알린다. 팀이 클래스 기본값으로만 고정돼야 한다면 런타임 `SetGenericTeamId` 사용을 제거해 계약을 하나로 만든다.
- **확신도**: 높음

### 2. 🟡 SPD 초기 복제가 구독보다 빠르면 클라이언트 이동 속도에 배율이 빠진다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:207`
- **범주**: 버그/정확성
- **문제**: `InitAbilitySystem`은 SPD 변경 델리게이트를 등록하지만 구독 시점의 현재 SPD를 한 번 적용하지 않는다. 서버는 곧이어 `GiveAbilitySet`을 호출해 변경 이벤트가 발생하지만, 소유 클라이언트는 `OnRep_PlayerState`에서 뒤늦게 이 함수를 호출한다(`Source/WxGame/Character/WxPlayerCharacter.cpp:62`). 그 전에 AttributeSet 초기값이 복제됐다면 해당 변경 이벤트는 이미 지나갔고, 다음 SPD 변경 전까지 `MaxWalkSpeed`가 `BaseWalkSpeed` 그대로 남아 서버와 달라진다. 같은 초기 복제 문제를 Ragdoll·Death 태그는 구독 직후 현재 값을 확인해 보완하고 있어 SPD 경로만 비대칭이다.
- **제안**: 델리게이트 등록 직후 현재 SPD를 `GetNumericAttribute`로 읽어 `HandleSPDAttributeChanged`와 같은 계산을 한 번 실행한다.
- **확신도**: 중간

### 3. 🟡 중복 ActionSet이 같은 GameFeatureAction 수명주기를 여러 번 실행한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:368`
- **범주**: 버그/정확성
- **문제**: `CollectActions`는 Experience와 ActionSet의 액션 포인터를 `Add`로 누적하고 중복을 제거하지 않는다. Experience가 같은 ActionSet을 두 번 참조하면 동일 액션 인스턴스에 등록·로딩·활성 및 비활성·해제 훅이 반복 호출된다. `UWxGameFeatureAction_AddComponents`는 반복 활성 시 `ContextHandles.FindOrAdd`로 같은 저장소를 재사용하면서 `GameInstanceStartHandle`을 덮어쓰고(`Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp:52`), 앞선 전역 델리게이트 핸들과 중복 컴포넌트 요청을 정상적으로 회수할 수 없게 된다.
- **제안**: `CollectActions`에서 포인터 기준 순서 보존 중복 제거를 적용하고, `UWxExperienceDefinition::IsDataValid`에서 중복 ActionSet 참조를 오류로 검출한다.
- **확신도**: 높음

### 4. 🟡 마지막 PIE 요청자가 활성 실패하면 이미 활성된 플러그인이 남을 수 있다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:303`
- **범주**: 버그/정확성
- **문제**: 실패 정리는 `ReleaseGameFeaturePluginRequests(false)`를 호출하고, 참조 카운트가 0이 되어도 현재 컴포넌트의 `ActivatedGameFeaturePluginURLs`에 URL이 없으면 비활성화를 건너뛴다. 다중 PIE에서 A가 플러그인을 활성화한 뒤 B가 같은 URL을 로드하는 동안 A가 먼저 종료되면 A의 카운트는 2→1이라 비활성화되지 않는다. 이후 B의 활성화가 실패하면 B가 1→0으로 마지막 요청을 해제하지만 `bWasActivated`가 거짓이라 `DeactivateGameFeaturePlugin`을 호출하지 않아, 요청자는 0명인데 A가 활성화한 플러그인이 남는다.
- **제안**: 마지막 참조 해제 시 현재 컴포넌트의 성공 여부가 아니라 플러그인의 전역 활성 상태를 기준으로 비활성화한다. 가장 단순하게는 카운트가 0이면 비활성화를 요청하고, 필요하면 `UWxExperienceManager`가 URL별 활성 성공 여부까지 함께 추적한다.
- **확신도**: 높음

### 5. 🟡 Experience 실패 상태를 소비자가 관찰할 수 없어 대기자가 영구 정지한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:127`
- **범주**: 설계/구조
- **문제**: 플러그인 해석·활성 실패 시 매니저는 `Failed`로 전이하면서 `OnExperienceLoaded`를 비우지만(`Source/WxGame/Framework/WxExperienceManagerComponent.cpp:240`, `:287`), `CallOrRegister_OnExperienceLoaded`는 실패 상태도 로딩 중으로 취급해 이후 델리게이트를 다시 저장한다. `LoadState`는 private이고 실패 델리게이트나 조회 API가 없어 GameMode는 실패와 진행 중을 구분할 수 없다. 결과적으로 `HandleStartingNewPlayer_Implementation`의 로드 게이트는 영구히 닫히고, 로그 외에는 실패 UI·재시도·안전한 세션 종료 정책을 연결할 방법이 없다.
- **제안**: 성공/실패를 함께 전달하는 완료 결과 델리게이트를 제공하거나 `OnExperienceLoadFailed`와 읽기 전용 상태 접근자를 추가한다. 이미 `Failed`인 상태에서 등록한 소비자에는 즉시 실패를 전달한다.
- **확신도**: 높음

### 6. 🟡 ViewModel 인스턴스 함수가 금지된 `BlueprintCallable`을 사용한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Item.h:47`
- **범주**: 규칙 위반
- **문제**: 루트 `AGENTS.md`는 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action의 팩토리 함수에서만 허용한다. 그러나 `UWxViewModel_Item::RequestUseConsumable`, `UWxViewModel_InteractionList::RequestInteract`·`RequestCycle`(`Source/WxGame/MVVM/WxViewModel_InteractionList.h:43`, `:46`), `UWxViewModel_Inventory::SetCurrentCategory`(`Source/WxGame/MVVM/WxViewModel_Inventory.h:80`), `UWxViewModel_Dialogue::RequestAdvance`(`Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`)는 모두 일반 ViewModel 인스턴스 함수이다.
- **제안**: 요청 진입점을 허용된 Function Library로 옮기거나 MVVM 바인딩 구조를 바꾼다. ViewModel 요청 함수를 의도적으로 허용할 필요가 있다면 코드보다 먼저 `AGENTS.md`에 좁은 예외를 명시한다.
- **확신도**: 높음

### 7. 🟢 폰이 바뀌어도 이전 Enhanced Input MappingContext가 남는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:88`
- **범주**: 버그/정확성
- **문제**: `SetupPlayerInputComponent`는 LocalPlayer 서브시스템에 `InputConfig->MappingContext`를 추가하지만, `NotifyControllerChanged`·`EndPlay`·빙의 해제 경로 어디에서도 `RemoveMappingContext`를 호출하지 않는다. 같은 LocalPlayer가 다른 MappingContext를 쓰거나 입력 구성이 없는 폰으로 전환되면 이전 컨텍스트가 계속 적용돼 키를 소비하거나 새 폰의 액션과 충돌할 수 있다.
- **제안**: 추가한 MappingContext와 LocalPlayer 서브시스템을 추적해 빙의 해제·종료 시 대칭적으로 제거하고, 재설정 시 이전 컨텍스트를 먼저 정리한다.
- **확신도**: 중간

### 8. 🟢 델리게이트 콜백 이름이 `Handle` 접두사 규칙을 따르지 않는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:96`
- **범주**: 규칙 위반
- **문제**: Enhanced Input에 바인딩되는 `Move`, `Look`, `ToggleCrouch`, `AbilityInputTriggered`, `AbilityInputReleased`는 프로젝트가 정의한 콜백이지만 `Handle`로 시작하지 않는다. `FGameFeatureDeactivatingContext`에 전달되는 `WxHandleDeactivationPauserCompleted`도 동일하다(`Source/WxGame/Framework/WxExperienceManagerComponent.cpp:22`). 이는 델리게이트 콜백 함수에 `Handle` 접두사를 요구하는 루트 `AGENTS.md` 규칙과 다르다.
- **제안**: 프로젝트 소유 입력 콜백을 `HandleMove`·`HandleLook`·`HandleToggleCrouch`·`HandleAbilityInputTriggered`·`HandleAbilityInputReleased`로, 정적 콜백을 `HandleDeactivationPauserCompleted`로 바꾼다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxExperienceManager.h`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.h`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.h`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxGameState.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.h`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxBossCharacter.h`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/Controller/WxPlayerController.h`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.h`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.h`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **미검토 / 한계**: Experience·ActionSet·GameFeature의 실제 데이터 에셋 조합과 BP/WBP 내부 구조는 범위 밖이다. 멀티플레이·다중 PIE 실행 검증은 하지 않았으며, 네트워크 및 수명주기 발견은 C++ 제어 흐름과 엔진 API 계약을 기준으로 판단했다.

---
*문서 기준 커밋 `66c0f6fd` · 리뷰일 2026-08-30 · 소스 64파일 — `/module-review`로 갱신*
