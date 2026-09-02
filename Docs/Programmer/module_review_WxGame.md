# WxGame — 코드 리뷰

> Lyra Experience/GameFeature 파이프라인을 충실히 이식한 조립 모듈로, 로드 상태기계·정리 경로·복제 권한이 대체로 정확하고 주석이 의도를 잘 남긴다. 이번 리뷰는 Framework(Experience 파이프라인)·Character·MVVM·AbilitySystem 의 cpp 를 정독하고 나머지 헤더·소형 파일을 훑는 수준으로 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 Experience 재확정 가드가 반쪽이라 시작 아이템이 중복 지급될 수 있다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:29`, `Source/WxGame/Framework/WxExperienceManagerComponent.h:46`
- **범주**: 버그/정확성
- **문제**: `SetCurrentExperience` 는 "엔진이 `InitGameState` 를 재호출하는 경로"를 명시적으로 전제하고 `CurrentExperience` 가 이미 있으면 무시하는 멱등 처리를 넣어 뒀다. 그런데 바로 다음 줄의 `CallOrRegister_OnExperienceLoaded` 에는 같은 가드가 없다. 그 전제대로 `InitGameState` 가 두 번 돌면, 이미 `Loaded` 인 상태에서는 델리게이트가 즉시 실행되어 `HandleExperienceLoaded` → `GrantDefaultInventory` 가 접속 중인 모든 컨트롤러에 다시 돌고(`WxGameMode.cpp:101-118`), 아직 로딩 중이면 델리게이트가 두 개 등록돼 완료 시 2회 방송된다. 어느 쪽이든 액션셋의 `DefaultInventoryItems` 가 두 번 들어간다.
- **제안**: 등록 자체를 1회로 묶는다 — `SetCurrentExperience` 가 실제로 값을 확정했을 때만 `CallOrRegister_OnExperienceLoaded` 를 부르거나, `HandleExperienceLoaded` 안에서 이미 지급했는지 판별한다. 반대로 재호출 경로가 실제로 없다고 확인되면 `SetCurrentExperience` 쪽 가드 주석을 정정해 전제를 하나로 맞추는 편이 낫다.
- **확신도**: 중간

### 2. 🟡 보스 뷰모델이 "보스는 한 마리"를 암묵 전제해 오픈월드 스트리밍과 어긋난다
- **위치**: `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp:19`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp:42`
- **범주**: 설계/구조
- **문제**: `StartObserving` 은 `TActorIterator` 로 찾은 **첫 번째** 보스만 잡고, `HandleActorSpawned` 는 새 `AWxBossCharacter` 가 스폰될 때마다 현재 보스가 살아 있어도 **무조건 교체**한다. 오픈월드에서 레벨 스트리밍으로 보스가 로드되면 그것도 스폰으로 들어오므로, 교전 중인 보스의 체력바가 멀리서 스트리밍-인 된 다른 보스로 갈아타는 표시 오류가 난다. 또 `AddOnActorSpawnedHandler` 가 VM 수명 내내 월드의 모든 스폰에 대해 캐스트를 돌린다.
- **제안**: 교체 조건을 좁힌다 — 이미 살아 있는 보스를 물고 있으면 새 스폰을 무시하거나, 플레이어와의 거리·`HasAITarget` 같은 "교전 중" 신호를 붙어야 할 대상의 기준으로 삼는다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 한 판에 보스 하나라는 전제라면 현행이 맞다)

### 3. 🟡 치트 매니저에 쓰이지 않는 멤버와 그것만을 위한 의존이 남아 있다
- **위치**: `Source/WxGame/Cheat/WxCheatManager.h:47`
- **범주**: 중복/복잡도
- **문제**: `ClearedAbilities` 는 선언·주석("치트가 걷어 둔 동안…")만 있고 `WxCheatManager.cpp` 어디서도 읽거나 쓰지 않는다. 이 멤버 하나 때문에 헤더의 `GameplayTagContainer.h`·`Templates/SubclassOf.h` include 와 `class UGameplayAbility` 전방 선언도 함께 남아 있어, 존재하지 않는 "어빌리티 제거/복구 치트"가 있는 것처럼 읽힌다.
- **제안**: 멤버와 딸린 include·전방 선언을 함께 제거한다. 어빌리티 제거 치트를 되살릴 계획이면 그때 함께 넣는다.
- **확신도**: 높음

### 4. 🟢 입력 델리게이트 콜백에 `Handle` prefix 가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:111`, `:115`, `:124`, `:129`, `:130`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `EIC->BindAction(...)` 으로 묶이는 `Move`/`Look`/`ToggleCrouch`/`AbilityInputTriggered`/`AbilityInputReleased` 가 여기 해당한다. 모듈 내 다른 모든 델리게이트 콜백(`HandleDeathTagChanged`, `HandleExperienceAssetsLoaded`, `HandleStackChanged` 등)은 규칙을 지키고 있어 이곳만 예외로 남는다.
- **제안**: `HandleMoveInput`·`HandleLookInput`·`HandleCrouchInput`·`HandleAbilityInputTriggered`·`HandleAbilityInputReleased` 로 개명하거나, Enhanced Input 바인딩은 규칙 예외임을 `CLAUDE.md` 에 명시한다.
- **확신도**: 높음

### 5. 🟢 Private 의존 모듈의 타입이 공개 include 경로의 헤더에 노출돼 있다
- **위치**: `Source/WxGame/WxGame.Build.cs:38`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h:7`
- **범주**: 설계/구조
- **문제**: `PublicIncludePaths` 가 모듈 루트 전체라 `WxMetaHumanComponent.h` 는 외부에서 include 가능한 헤더인데, 이 헤더가 `MetaHumanSDKRuntime`(Private 의존)의 `MetaHumanComponentUE.h` 를 include 하고 베이스 클래스로 쓴다. `WxEditor` 는 이미 `WxGame` 에 의존하므로, 어느 시점에 이 헤더를 한 줄 include 하는 순간 include 경로가 없어 빌드가 깨진다. `HairStrandsCore`·`EnhancedInput` 도 같은 구조다(현재는 cpp 에서만 쓰여 표면화되지 않았다).
- **제안**: `MetaHumanSDKRuntime` 를 `PublicDependencyModuleNames` 로 올리거나, 메타휴먼 조립 컴포넌트를 Private 전용 헤더로 옮겨 공개 표면에서 뺀다.
- **확신도**: 높음

### 6. 🟢 쓰이지 않는 include 가 남아 있다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.h:7`
- **범주**: 중복/복잡도
- **문제**: `#include "Engine/TimerHandle.h"` 가 있으나 헤더·cpp 어디에도 `FTimerHandle` 이 없다. 타이머 기반 처리가 있는 것처럼 오해를 준다.
- **제안**: 제거한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxMinion.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Input/WxInputConfig.*`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.*`, `Source/WxGame/MVVM/WxViewModel_Dialogue.*`, `Source/WxGame/MVVM/WxViewModel_Quest.*`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.*`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.*`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.*`
- **미검토 / 한계**:
  - 멀티플레이 정합성은 코드 독해로만 봤고 실제 dedicated/listen 세션 검증은 하지 않았다. 특히 `AWxPlayerCharacter::OnJumped_Implementation` 의 무적 GE 는 예측 키 없이 클라에서 로컬 적용된 뒤 서버본이 복제돼 인스턴스가 겹칠 여지가 있는데, 프로젝트 전반이 "서버가 곧 클라" 전제라 발견으로 올리지 않았다.
  - `UWxMetaHumanComponent::OnRegister` 가 다른 컴포넌트의 등록 도중 `RegisterComponent()` 를 연쇄 호출하는 구조는 에디터 리컴파일·레벨 스트리밍 왕복에서만 드러나는 종류라 정적 독해로는 판정하지 못했다.
  - `UWxViewModel_Inventory::RefreshAllItems` 의 전체 재구축(O(N²) 매칭 + 무조건 브로드캐스트)은 인벤토리 규모가 커지기 전까지는 비용이 드러나지 않아 발견으로 세지 않았다.
  - BP/WBP 자산 내부(위젯 계층·MVVM 바인딩 행·Experience 에셋 실제 값)는 범위 밖이다.

---
*문서 기준 커밋 `79bab788` · 리뷰일 2026-09-03 · 소스 71파일 — `/module-review`로 갱신*
