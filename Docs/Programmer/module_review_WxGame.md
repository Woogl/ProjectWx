# WxGame — 코드 리뷰

> Experience 파이프라인·프레임워크 실체·MVVM 브리지 모두 의도가 주석으로 촘촘히 박혀 있고 권위 가드와 넷모드 분기가 대체로 일관되게 잡혀 있는, 건강한 편의 게임 모듈이다. 이번 리뷰는 Framework/Character/MVVM/AbilitySystem 전 폴더의 헤더를 훑고 로직 밀도가 높은 12개 cpp를 라인 단위로 읽었으며, 필요한 곳은 WxCombat·WxInventory·엔진(UE 5.8) 소스까지 따라가 근거를 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 `UWxMetaHumanComponent` 가 `AWxNpc` 에서 항상 무동작한다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.cpp:37`
- **범주**: 버그/정확성
- **문제**: 조립의 출발점인 리더 메시를 `Cast<ACharacter>(GetOwner())->GetMesh()` 로만 구한다. `AWxNpc` 는 `ACharacter` 가 아니라 `AActor` 파생이므로(`Source/WxGame/Character/WxNpc.h:22`) 캐스팅이 널이 되고, 함수는 조기 반환해 바디·페이스·그룸·복장·LODSync 를 하나도 만들지 않는다. `BodyComponentName`/`FaceComponentName` 도 채워지지 않아 엔진 메타휴먼 컴포넌트의 페이스 리그로직·넥 보정까지 죽는다. 그런데 `AWxNpc` 는 이 컴포넌트를 직접 합성하고(`Source/WxGame/Character/WxNpc.cpp:41`) 클래스 doc 에도 "메타휴먼 부착물을 바디 메시에 조립하는 컴포넌트"라고 명시해 둔 상태다(`Source/WxGame/Character/WxNpc.h:41`). 로그도 남기지 않아 조용히 실패한다 — 현재 `BP_Npc` 의 메타휴먼 슬롯이 비어 있어 증상이 드러나지 않았을 뿐, 에셋을 채우는 순간 "아무것도 안 나온다"로 나타난다.
- **제안**: 리더 메시 획득을 `ACharacter` 캐스팅에서 떼어낸다(예: `GetOwner()->FindComponentByClass<USkeletalMeshComponent>()`, 또는 오너가 리더 메시를 명시로 넘기는 진입점). `OnUnregister:131` 의 같은 캐스팅도 함께 맞춘다. 최소한 리더 메시를 못 찾은 경우 에러 로그를 남겨 조용한 무동작을 없앤다.
- **확신도**: 높음

### 2. 🟡 `Reset()` 재진입 시 시작 아이템이 이중 지급된다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:29`
- **범주**: 버그/정확성
- **문제**: 엔진의 `AGameModeBase::Reset()` 은 `InitGameState()` 를 다시 부르고, `ResetLevel()` 은 `BlueprintCallable` 이라 BP 에서도 열려 있다. `SetCurrentExperience` 에는 이 경로를 겨냥한 멱등 가드가 있지만(`Source/WxGame/Framework/WxExperienceManagerComponent.cpp:77`), 바로 다음 줄의 `CallOrRegister_OnExperienceLoaded` 에는 없다. 이미 로드된 상태면 이 함수는 델리게이트를 즉시 실행하므로(`Source/WxGame/Framework/WxExperienceManagerComponent.cpp:102`), `HandleExperienceLoaded` 가 한 번 더 돌아 접속 중인 전 플레이어에게 `GrantDefaultInventory` 를 재실행한다(`Source/WxGame/Framework/WxGameMode.cpp:115`). 즉 절반만 멱등이라, 가드가 막으려던 경로에서 정확히 아이템이 두 벌 들어간다.
- **제안**: 지급 완료를 GameMode 쪽에서 1회로 게이트하거나(예: 이미 등록/실행했으면 재등록을 건너뜀), 지급 자체를 "이 컨트롤러에 이미 줬는지" 기준으로 판단하게 한다.
- **확신도**: 중간(현재 저장소에 `Reset()`/`ResetLevel()` 을 부르는 코드는 없다. 다만 `SetCurrentExperience` 의 가드 주석이 그 경로를 전제로 쓰였다)

### 3. 🟡 `InitAbilitySystem()` 이 멱등하지 않다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:197`
- **범주**: 버그/정확성
- **문제**: 진입점이 둘이다 — 서버는 `PossessedBy`(`Source/WxGame/Character/WxCharacterBase.cpp:107`), 클라는 `AWxPlayerCharacter::OnRep_PlayerState`(`Source/WxGame/Character/WxPlayerCharacter.cpp:64`). RepNotify 는 참조가 unmapped(null)로 먼저 도착했다가 매핑 후 다시 불릴 수 있고 재빙의 시에도 반복되는데, 함수 안에 재진입 가드가 없다. 결과는 셋이다. (a) `BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed` 가 이미 SPD 로 스케일된 값을 다시 기준으로 잡아 배수가 누적된다(SPD 기본값이 1.0 이라 SPD≠1 인 순간에 재진입할 때만 드러난다). (b) SPD 어트리뷰트 변경 델리게이트가 중복 등록된다. (c) 서버에서는 `GiveAbilitySet()` 이 재실행되는데 `UWxAbilitySet::GiveToAbilitySystem` 에 재진입 가드가 없어(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:36`) 어빌리티·GE 가 중복 부여되고 어트리뷰트 초기화 행이 현재값을 덮어쓴다. 덧붙여 `BaseWalkSpeed` 는 인클래스 초기화자가 없다(`Source/WxGame/Character/WxCharacterBase.h:123`).
- **제안**: 1회 초기화 가드를 두거나, `BaseWalkSpeed` 캡처와 델리게이트 등록을 반복되지 않는 지점(`PostInitializeComponents`)으로 옮기고 `InitAbilitySystem` 은 부여만 담당하게 나눈다. `BaseWalkSpeed` 에 초기값도 준다.
- **확신도**: 중간

### 4. 🟡 처치 보상이 가해자와 무관하게 항상 0번 플레이어로 간다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:58`
- **범주**: 설계/구조
- **문제**: `UGameplayStatics::GetPlayerController(this, 0)` 을 직접 지급 대상으로 넘긴다. 이 모듈은 그 외 거의 모든 지점에서 `HasAuthority` 로 사이드를 가르고 복제 전제를 주석으로 명시하는데, 유독 보상만 로컬 0번 플레이어에 하드코딩돼 있다. 멀티에서는 누가 죽였든 재화가 0번 플레이어에게 들어가고, 데디케이티드 서버에서는 0번 컨트롤러 자체가 임의의 접속자다. 가해자를 추적하는 경로도 없다(`HandleDeath` 는 인자가 없다).
- **제안**: 사망 시 가해자(마지막 대미지 instigator 또는 처형 주체)를 받아 그 컨트롤러에 지급하도록 진입점을 넓힌다. 단일 플레이어 전제를 유지할 거라면 그 전제를 주석으로 못 박아 둔다.
- **확신도**: 중간(현재 단일 플레이어 전제의 의도적 단순화일 수 있음)

### 5. 🟢 클라이언트에서 에너미의 `InitAbilitySystem` 이 전혀 호출되지 않는다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:96`
- **범주**: 설계/구조
- **문제**: 베이스 doc 은 "클라이언트: 파생 클래스에서 OnRep 을 통해 호출"이라는 계약을 선언하지만, 그 오버라이드를 제공하는 것은 `AWxPlayerCharacter` 하나뿐이다. 에너미는 PlayerState 가 없어 `OnRep_PlayerState` 경로가 없고 `PossessedBy` 는 서버 전용이므로, 클라이언트에서는 SPD → `MaxWalkSpeed` 연동이 아예 걸리지 않고 `BaseWalkSpeed` 도 미초기화로 남는다(읽는 쪽이 없어 크래시는 없다). 이동 자체는 복제로 수렴하므로 지금은 무증상이지만, 계약과 구현이 어긋나 있다.
- **제안**: 에너미도 클라에서 1회 초기화하도록 경로를 만들거나, 반대로 베이스 doc 이 플레이어 한정 계약임을 명시한다.
- **확신도**: 높음(사실 관계). 영향 범위는 낮음

### 6. 🟢 `IsAlive()` 가 ASC 널 검사보다 먼저 역참조된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:80`
- **범주**: 버그/정확성
- **문제**: `if (!IsAlive() || !AbilitySystemComponent)` 인데 `AWxCharacterBase::IsAlive()` 가 이미 `AbilitySystemComponent->GetSet<>()` 를 무가드로 역참조한다(`Source/WxGame/Character/WxCharacterBase.cpp:190`). ASC 가 널일 수 있다는 전제가 맞다면 이 순서로는 못 막고, 널일 수 없다면(실제로 기본 서브오브젝트라 널이 아니다) 뒤쪽 검사는 죽은 코드다. 같은 파일 안에서 `GetOwnedGameplayTags`·`CanJumpInternal` 은 가드를 두고 `IsAlive` 는 두지 않아 전제도 엇갈린다.
- **제안**: ASC 가 널이 될 수 없다는 쪽으로 정리해 `GetEligibleFinisherEventTag` 의 뒤 검사와 다른 곳의 잉여 가드를 함께 지운다.
- **확신도**: 높음

### 7. 🟢 토스트용 아이템 VM 이 인벤토리 델리게이트를 물고 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104`
- **범주**: 성능/안전
- **문제**: 아이템 획득마다 `UWxViewModel_Item` 을 새로 만들어 `Initialize(Inventory, ItemDef)` 로 스택·충전 델리게이트 2건을 구독시키고 아이콘 비동기 스트리밍까지 건다. 그런데 이 VM 의 용도는 `AcquiredCount` 를 한 번 읽히는 정지 스냅샷이라 라이브 구독이 필요 없다. 교체된 이전 VM 은 GC 가 돌아 `BeginDestroy → Deinitialize` 가 불릴 때까지 계속 구독 상태로 남으므로, 픽업이 잦은 구간에서는 GC 주기 동안 인벤토리 브로드캐스트 수신자가 계속 늘어난다.
- **제안**: `LastAcquiredItem` 을 교체하기 직전에 이전 VM 을 `Deinitialize()` 하거나, 토스트용으로 구독 없는 스냅샷 초기화 경로를 따로 둔다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp` 및 전 헤더
- **미검토 / 한계**:
  - `CLAUDE.md` 코딩 규칙(Wx prefix·`Handle` prefix·`BlueprintCallable`·람다·인라인 정의·저작권 첫 줄)은 전수 grep 으로 확인했고 위반 없음. `BlueprintCallable` 5건은 전부 뷰모델의 Command/Setter 로, 프로젝트에 이미 확립된 예외 범주라 발견으로 올리지 않았다. `AWxGameMode::GetDefaultPawnClassForController` 와 `AWxCharacterBase::CanJumpInternal` 의 `Super::` 미호출도 근거 주석이 붙은 의도적 설계라 제외했다.
  - BP/WBP 에셋 내부(무기 `ChildActorClass` 지정, WBP View Bindings 배선, `BP_Npc` 의 메타휴먼 슬롯 값)는 리뷰 범위 밖이다. 발견 1의 현재 무증상 여부만 `BP_Npc.uasset` 문자열로 간접 확인했다.
  - 멀티플레이·데디케이티드 서버 실측은 하지 않았다. 발견 2·3·4·5 의 재현 조건은 엔진 소스(UE 5.8 `GameModeBase.cpp`)와 코드 경로에 근거한 추론이다.
  - GameFeature 쿠킹 경로(`UpdateAssetBundleData`·`AddAdditionalAssetBundleData` 가 실제 쿠킹 의존성을 만드는지)는 빌드/쿠킹 없이 확인할 수 없어 미검증이다.
  - `PublicIncludePaths` 가 모듈 전체를 공개하는데 `WxMetaHumanComponent.h` 가 private 의존(`MetaHumanSDKRuntime`)의 타입을 상속해 노출한다. `WxEditor` 가 현재 그 헤더를 포함하지 않아 무증상이라 발견으로 올리지 않았지만, 포함하는 순간 빌드가 깨지는 잠재 위험으로 남겨 둔다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 66파일 — `/module-review`로 갱신*
