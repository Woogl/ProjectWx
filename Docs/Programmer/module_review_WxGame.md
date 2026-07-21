# WxGame — 코드 리뷰

> 게임 조립 모듈로서 전반적으로 건강하다. 프레임워크 주입·GAS 초기화·네트워크 권위 경계·MVVM 리졸버 수명 관리가 일관된 규약으로 잘 정리돼 있고, 심각(🔴) 결함은 발견되지 않았다. 프레임워크 클래스(GameMode/GameState/Controller/CharacterBase)와 어빌리티·MVVM·월드오브젝트까지 구현부를 통독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 획득 알림 VM(`LastAcquiredItem`)이 교체·해제 시 `Deinitialize` 되지 않는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:82`, 해제부 `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:36`
- **범주**: 중복/복잡도 · 수명주기
- **문제**: `HandleStackChanged`는 `Delta>0`마다 `NewObject<UWxViewModel_Item>` 로 Def 모드 VM 을 새로 만들어 `LastAcquiredItem` 에 대입한다. 이 Def 모드 `Initialize`는 인벤토리의 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 델리게이트에 바인딩한다(`WxViewModel_Item.cpp:46-47`). 그러나 기존 `LastAcquiredItem`을 교체할 때도, `UWxViewModel_Inventory::Deinitialize`에서 정리할 때도(단순히 nullptr 대입만 함) 이전 VM 에 `Deinitialize()`를 호출하지 않는다. 같은 파일의 `AllItems`는 교체 시 미보존 VM 을 일일이 `Deinitialize` 하며(`WxViewModel_Inventory.cpp:128-134`) 대칭을 지키는데, 획득 VM 만 예외다. 결과적으로 아이템을 획득할 때마다 이전 획득 VM 이 GC 로 수거되기 전까지 인벤토리 델리게이트에 남아 무의미한 콜백(`HandleStackChanged`/`HandleChargeChanged`)을 계속 수신한다. 동적/UObject 바인딩이 무효 대상을 자동 정리하므로 영구 누수는 아니지만, 수명 규약이 코드 전반과 어긋나 잠재적 낭비·혼선을 만든다.
- **제안**: 교체 직전과 `Deinitialize` 시 이전 `LastAcquiredItem`에 대해 `Deinitialize()`를 호출해 `AllItems` 경로와 대칭을 맞춘다.
- **확신도**: 중간(기능 오작동은 아니며 낭비·수명 비대칭 관점).

### 2. 🟢 `IsAlive()`가 널 검사보다 먼저 `AbilitySystemComponent`를 역참조한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:181`, 호출부 `Source/WxGame/Character/WxEnemyCharacter.cpp:116`
- **범주**: 성능/안전 · 방어 코드 정합성
- **문제**: `IsAlive()`는 `AbilitySystemComponent->GetSet<...>()`을 널 검사 없이 곧바로 역참조한다. 반면 같은 클래스의 다른 함수들(`GetOwnedGameplayTags` 등)은 `if (AbilitySystemComponent)`로 방어한다. 게다가 `WxEnemyCharacter::GetEligibleFinisherEventTag`의 가드가 `if (!IsAlive() || !AbilitySystemComponent)` 순서라, ASC 가 널이라면 `!AbilitySystemComponent` 검사에 도달하기 전에 `IsAlive()` 내부에서 이미 역참조한다(가드 순서 역전). 실제로 ASC 는 생성자에서 항상 만들어져 널이 되지 않으므로 크래시로 이어지지 않지만, 방어 의도와 실제 동작이 어긋난다.
- **제안**: `IsAlive()` 내부에 `if (AbilitySystemComponent)` 가드를 추가하거나, 다른 함수처럼 ASC 널을 전제하지 않도록 정리한다.
- **확신도**: 낮음(현재 코드 경로에서 ASC 는 항상 유효 — 의도된 단순화일 수 있음).

### 3. 🟢 에너미의 SPD 이동속도 콜백이 클라이언트에서 등록되지 않는다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:195`(`InitAbilitySystem`), 호출 경로 `WxCharacterBase.cpp:105-110`(`PossessedBy`)
- **범주**: 설계/구조 · 초기화 순서
- **문제**: `HandleSPDAttributeChanged` 바인딩과 `RefreshAbilityActorInfo`는 `InitAbilitySystem`에서 이뤄지는데, 이 함수는 서버 `PossessedBy`와 플레이어의 `OnRep_PlayerState`(`WxPlayerCharacter.cpp:80-85`)에서만 호출된다. AI 빙의 전용 에너미는 클라이언트에서 `PossessedBy`도 `OnRep_PlayerState`도 타지 않아 `InitAbilitySystem`이 호출되지 않으며, 그 결과 시뮬레이트 프록시 에너미의 `MaxWalkSpeed`에 SPD 배수가 반영되지 않는다. 사망/래그돌 감지는 `PostInitializeComponents`로 분리돼 전 머신에서 동작하지만(의도적 설계), SPD 반영은 이 분리에 포함되지 않았다. 이동은 서버 권위 위치 복제로 처리되므로 시각적 영향은 미미하나, 클라 측 애니메이션/예측이 `MaxWalkSpeed`에 의존하면 어긋날 수 있다.
- **제안**: 클라이언트 프록시에서도 SPD 반영이 필요하다면 SPD 델리게이트 등록을 `PostInitializeComponents`(래그돌/사망 감지와 동일 위치)로 옮기는 것을 검토한다.
- **확신도**: 낮음(시뮬레이트 프록시는 복제 이동을 쓰므로 의도된 범위일 수 있음).

### 4. 🟢 `WxAbility_UseItem` 주석이 `WxAbility_Interact`의 넷 정책을 잘못 기술한다
- **위치**: `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp:12`
- **범주**: 규칙 위반 없음 · 문서 정확성
- **문제**: `NetExecutionPolicy = ServerInitiated` 위 주석이 "서버에서 활성화한다(WxAbility_Interact 와 동일)"이라 적혀 있으나, `WxAbility_Interact`는 `LocalPredicted`이다(`WxAbility_Interact.cpp:29`). 넷 정책을 오해하게 만드는 부정확한 주석이다.
- **제안**: "WxAbility_Interact 와 동일" 문구를 제거하거나 정확한 근거로 교체한다.
- **확신도**: 높음(코드 대조로 확인 — 동작에는 영향 없음).

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxGameMode.cpp`·`.h`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`·`.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`·`.h`, `Source/WxGame/Controller/WxPlayerController.cpp`·`.h`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`·`.h`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`
- **훑은 파일**: `Source/WxGame/Character/WxCharacterMovementComponent.cpp`·`.h`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/WorldObject/WxCheckPoint.cpp`, `Source/WxGame/WorldObject/WxLaserCorridor.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/WxGame.Build.cs`
- **미검토 / 한계**: `WxViewModel_Inventory`(`WxViewModelResolver_InteractionList`)가 참조하는 각 플러그인(`WxInventory`·`WxCombat`·`WxWorld` 등)의 내부 계약(델리게이트 발화 시점, ASC 복제 세부)은 본 모듈 밖이라 확정 검증하지 못했다. `.uasset`(BP 디폴트값, 컴포넌트 트리) 실제 지정값은 코드만으로 확인 불가.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 44파일 — `/module-review`로 갱신*
