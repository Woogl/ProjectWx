# WxGame — 코드 리뷰

> 도메인 플러그인을 조립하는 최상위 모듈답게 경계가 잘 지켜져 있고, Experience/GameFeature 파이프라인은 Lyra 이식본으로서 상태기계·정리 경로가 정확하다. 위험한 지점마다 주석으로 전제와 한계가 남아 있어 전반적으로 건강하다. 이번 리뷰는 `Source/WxGame` 아래 70개 파일 전부를 읽었고, Framework 8쌍·캐릭터·컨트롤러·MVVM 의 cpp 는 로직 흐름까지 따라갔다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 `Reset()` 으로 `InitGameState` 가 재진입하면 시작 아이템이 중복 지급된다
- **위치**: `Source/WxGame/Framework/WxGameMode.cpp:30`
- **범주**: 버그/정확성
- **문제**: 엔진 `AGameModeBase::Reset()` 은 `InitGameState()` 를 다시 호출한다(UE 5.8 `GameModeBase.cpp:334-336`). 이 재진입을 대비해 `SetCurrentExperience` 는 멱등 처리돼 있지만(`WxExperienceManagerComponent.cpp:104-107`, 헤더 주석도 이 경로를 명시한다), 바로 다음 줄의 `CallOrRegister_OnExperienceLoaded` 는 방어가 없다. 이미 로드된 상태라면 델리게이트가 **즉시 실행**되어(`WxExperienceManagerComponent.cpp:129-132`) `HandleExperienceLoaded` → `GrantDefaultInventory` 가 접속 중인 전 PlayerController 에 대해 다시 돈다. `UWxInventoryComponent::GrantItems` 는 중복 지급을 거르지 않으므로(`Plugins/WxInventory/.../WxInventoryComponent.cpp:328`) 시작 아이템이 그대로 한 벌 더 쌓인다. 로드 완료 전 재진입이면 델리게이트가 2개 등록돼 같은 결과가 된다. 현재 프로젝트 코드가 `Reset()`/`ResetLevel()` 을 부르는 곳은 없어 잠복 상태다.
- **제안**: 멱등 처리를 `SetCurrentExperience` 와 같은 깊이로 맞춘다 — `InitGameState` 에서 이미 등록했으면 재등록하지 않거나(플래그), `GrantDefaultInventory` 가 "이 컨트롤러에 이미 지급했는가"를 보게 한다.
- **확신도**: 중간

### 2. 🟡 스포너 부착이 적 캐릭터의 이동 복제 경로를 바꾼다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:175-183`
- **범주**: 설계/구조
- **문제**: `OnSpawnedBy` 가 정찰 경로 역참조와 아웃라이너 가독성을 위해 적 캐릭터를 스포너 액터에 `AttachToActor` 한다. 루트가 부착되면 엔진은 "부착 우선" 분기를 타서 `ReplicatedMovement` 대신 `AttachmentReplication` 을 채우고(`ActorReplication.cpp:426-428`), 클라 수신 측도 부착돼 있으면 복제 위치 적용을 건너뛴다(`ActorReplication.cpp:253` — "Attachment trumps global position updates"). ACharacter 는 `ReplicatedBasedMovement` 라는 별도 경로가 있어 지면 위에서는 대개 문제가 드러나지 않지만, 이동 베이스가 없는 구간(낙하·공중 넉백)은 AttachmentReplication 에만 의존하게 된다. 코드 주석도 이 리스크를 "멀티 검증 때 재확인할 것"으로 남겨 둔 상태다.
- **제안**: 부착 대신 링크만 남긴다 — `OwningSpawner` 약참조는 이미 있으므로 `UWxPatrolComponent::FindPatrolComponent` 가 부착 부모가 아니라 이 참조를 타고 오르게 하면 부착 자체가 불필요해진다. 아웃라이너 그룹핑이 목적이면 폴더/라벨로 대체한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `UWxViewModelResolver_Item` 만 `ExpectedType` 으로 인스턴스를 만든다 — 타입 검사가 없다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Item.cpp:182`
- **범주**: 성능/안전
- **문제**: `NewObject<UWxViewModel_Item>(Outer, ExpectedType)` 는 `ExpectedType` 이 `UWxViewModel_Item` 파생이 아니어도 검사 없이 `static_cast` 한다. 그 뒤 `ViewModel->Initialize(...)` 가 잘못된 타입 위에서 호출되면 UB 다. 같은 모듈의 다른 리졸버 6개는 전부 `ExpectedType` 을 쓰지 않고 구체 클래스로 만든다(예: `WxViewModel_Inventory.cpp:221`, `WxViewModel_InteractionList.cpp:159`) — 이 하나만 다르다.
- **제안**: `ExpectedType->IsChildOf(UWxViewModel_Item::StaticClass())` 를 확인하고 아니면 널을 반환하거나, 다른 리졸버와 같이 구체 클래스로 통일한다.
- **확신도**: 중간

### 4. 🟢 `OnRep_Team` 이 빈 구현이라 `ReplicatedUsing` 이 의미 없다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:190-194`, 선언 `WxCharacterBase.h:129,141`
- **범주**: 중복/복잡도
- **문제**: 본문이 `static_cast<void>(PreviousTeam);` 한 줄뿐이고 주석도 "별도 캐시를 갱신할 필요가 없다"고 밝힌다. `GetGenericTeamId` 가 복제 값을 직접 읽으므로 RepNotify 자체가 필요 없는데, `UFUNCTION` 과 인자 있는 OnRep 형태가 남아 "여기서 뭔가 한다"는 오해를 남긴다.
- **제안**: `ReplicatedUsing = OnRep_Team` 을 `Replicated` 로 바꾸고 `OnRep_Team` 을 제거한다.
- **확신도**: 높음

### 5. 🟢 `BaseWalkSpeed` 만 초기화 없이 선언돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:145`
- **범주**: 성능/안전
- **문제**: 같은 블록의 다른 멤버는 전부 기본값을 갖는데 `float BaseWalkSpeed;` 만 초기자가 없다. 실제 대입은 `InitAbilitySystem` 의 1회 래치 안에서만 일어나고(`WxCharacterBase.cpp:234`) 델리게이트 바인딩도 그 직후라 현재는 미초기화 값을 읽는 경로가 없지만, `HandleSPDAttributeChanged` 가 이 값을 곱해 `MaxWalkSpeed` 를 쓰는 구조라 순서가 바뀌면 곧바로 이동 속도 손상으로 번진다.
- **제안**: `float BaseWalkSpeed = 0.f;` 로 선언 시 초기화한다.
- **확신도**: 높음

### 6. 🟢 ViewModel 명령 함수의 `BlueprintCallable` 이 CLAUDE.md 규칙 5와 어긋난다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`, `WxViewModel_InteractionList.h:43,46`, `WxViewModel_Inventory.h:80`, `WxViewModel_Item.h:47`
- **범주**: 규칙 위반
- **문제**: 규칙 5는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수로 한정한다. 위 5곳은 WBP 가 뷰모델에 명령을 보내는 진입점(`RequestAdvance`·`RequestInteract`·`RequestCycle`·`SetCurrentCategory`·`RequestUseConsumable`)이라 지정자 없이는 BP 에서 호출할 방법이 없고, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:39` 도 같은 패턴이라 코드베이스 전반의 관용으로 굳어 있다. 즉 고칠 대상이라기보다 규칙에 예외가 빠져 있는 상태다.
- **제안**: CLAUDE.md 규칙 5에 "MVVM ViewModel 의 BP 호출 명령 함수" 예외를 명문화해 규칙과 코드의 어긋남을 없앤다(이미 `BlueprintSetter`/위젯 setter 예외가 메모리에 남아 있는 것과 같은 성격이다).
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`
- **훑은 파일**: `Source/WxGame` 아래 나머지 전부 — `Framework/WxExperienceDefinition.*`, `Framework/WxExperienceActionSet.*`, `Framework/WxExperienceManager.*`, `Framework/WxGameState.*`, `Framework/WxWorldSettings.*`, `Character/WxBossCharacter.*`, `Character/WxMinion.*`, `Character/WxNpc.*`, `Controller/WxPlayerController.*`, `Player/WxPlayerState.*`, `Cheat/WxCheatManager.*`, `Input/WxInputConfig.*`, `AnimNotify/WxAnimNotify_UseItem.*`, `AbilitySystem/Ability/WxAbility_Interact.*`, `MVVM/` 잔여 뷰모델·리졸버, `WxGame.Build.cs`
- **규칙 전수 검사 결과**: 소스 첫 줄 Copyright 누락 0건, `FORCEINLINE`·헤더 인라인 정의 0건, 람다 0건, `Wx` prefix 누락 0건, 델리게이트 콜백 `Handle` prefix 누락 0건. `override` 에서 `Super::` 를 생략한 3곳(`AWxGameMode::GetDefaultPawnClassForController_Implementation`, `AWxCharacterBase::CanJumpInternal_Implementation` 의 기립 분기)은 모두 의도가 주석으로 명시돼 있어 위반으로 보지 않았다.
- **미검토 / 한계**: Experience·ActionSet 에셋(.uasset)의 실제 내용은 열지 않아, `UWxGameFeatureAction_AddComponents` 가 실제로 어떤 컴포넌트를 어느 receiver 에 꽂는지는 코드 규칙(`WxResolveReceiverClass`)만으로 판단했다. 현재 프로젝트에 `UPawnComponent` 파생이 하나도 없어 Pawn 분기는 사실상 미사용이며, 향후 Pawn 컴포넌트를 추가하면 `AWxCharacterBase` 파생 전체(적·소환수 포함)에 붙는다는 점만 확인해 두었다. MetaHuman 조립(그룸·LODSync)의 런타임 시각 결과와 멀티플레이 실측 검증은 정적 리뷰 범위 밖이다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 70파일 — `/module-review`로 갱신*
