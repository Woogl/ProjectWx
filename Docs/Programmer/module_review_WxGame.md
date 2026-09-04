# WxGame — 코드 리뷰

> 여전히 건강한 모듈이다. 지난 리뷰의 지적 두 건(보스 식별의 월드 전체 스폰 훅, 베이스 캐릭터의 도달 불가 널 검사)은 실제로 정리됐고, Lyra Experience 파이프라인·캐릭터 계층·MVVM 브리지 모두 구조적 결함은 보이지 않는다. 이번 리뷰는 71개 소스 전부를 훑고 `c486a5c7` 이후 변경분(WxBossComponent 전환·CheatManager GE 경로 교체·널 검사 정리)을 우선 확인한 뒤, Framework·Character·Controller·MVVM 의 cpp 로직까지 내려가 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 스폰된 적을 스포너에 Attach 해 이동 복제가 AttachmentReplication 경로로 바뀐다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:183`
- **범주**: 설계/구조
- **문제**: 코드 주석이 "멀티 검증 때 재확인할 것"으로 남긴 위험의 구체적 결과다. 루트가 attach 되면 엔진은 절대 위치(`ReplicatedMovement`)를 채우지 않고 상대 오프셋(`AttachmentReplication`)만 보낸다(UE 5.8 `AActor::GatherCurrentMovement` 의 "If we are attached, don't replicate absolute position" 분기). 그 결과 시뮬레이트 프록시 적의 위치는 `ACharacter` 의 네트워크 스무딩이 아니라 `OnRep_AttachmentReplication` 의 상대 트랜스폼 하드 세팅으로 갱신되어, 리슨/데디 서버 클라에서 이동이 끊겨 보인다.
- **제안**: 지난 리뷰의 제안("정찰 조회를 `GetOwningSpawner()` 경유로")은 **채택 불가**다 — 이번에 확인한 바로 `UWxPatrolComponent::FindPatrolComponent` 는 `Pawn->GetAttachParentActor()` 로 경로 소유자를 찾는데(`Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp:23-28`), `AWxSpawner` 는 WxWorld 소속이라 WxAI 가 직접 볼 수 없다. 즉 attachment 는 도메인 간 참조를 피하려고 고른 우회 통로다. 고치려면 그 링크를 엔진 attachment 가 아닌 다른 통로로 옮겨야 한다 — (a) 경로 소유자를 WxCore 인터페이스로 추상화해 `FindPatrolComponent` 가 그것으로 묻게 하거나, (b) 스폰 시점에 폰의 Blackboard/컴포넌트에 정찰 경로를 직접 실어 주는 방식. 어느 쪽이든 `AttachToActor` 를 걷어내면 복제 경로가 정상으로 돌아온다.
- **확신도**: 중간 (싱글·호스트 플레이에선 증상이 없어 의도된 절충일 수 있음)

### 2. 🟡 토스트용 아이템 뷰모델이 Deinitialize 없이 버려져 인벤토리 델리게이트에 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-113`
- **범주**: 성능/안전
- **문제**: 획득(`Delta > 0`)마다 새 `UWxViewModel_Item` 을 만들어 `Initialize(Inventory, ItemDef)` 를 부르는데, 그 Def 모드 초기화는 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 두 델리게이트에 자신을 등록한다(`Source/WxGame/MVVM/WxViewModel_Item.cpp:45-46`). 직전 `LastAcquiredItem` 은 `Deinitialize()` 없이 교체만 되므로, GC 가 수거해 `BeginDestroy → Deinitialize`(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:31-35`)가 돌기 전까지 버려진 VM 들이 계속 구독 상태로 쌓인다. 특히 `HandleChargeChanged` 는 바인딩 인스턴스가 없으면 ItemDef 로 매칭하므로(`WxViewModel_Item.cpp:126`), 죽은 토스트 VM 들이 충전 변경마다 `RefreshChargeIcon → RequestImageAsync` 로 스트리밍 요청까지 다시 낸다. 포션을 연타로 마시는 구간에서 구독자 수가 획득 횟수만큼 선형으로 늘어난다.
- **제안**: `LastAcquiredItem` 교체 직전에 이전 VM 의 `Deinitialize()` 를 부른다. 더 나은 방향은 토스트 VM 을 구독 없는 스냅샷으로 만드는 것 — 표시에 필요한 것은 `ApplyStaticDataFromDef` 결과와 `AcquiredCount` 뿐이라 델리게이트 등록이 애초에 필요 없다.
- **확신도**: 높음

### 3. 🟡 입력 바인딩 콜백에 `Handle` 접두사가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:62-67`, 바인딩 지점 `Source/WxGame/Character/WxPlayerCharacter.cpp:111,115,124,129,130`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 4는 "Delegate 에 바인딩되는 Callback 함수는 `Handle` 을 Prefix 로" 한다. `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 는 Enhanced Input `BindAction` 이 만드는 델리게이트의 콜백인데 접두사가 없다. 모듈 전체의 델리게이트 바인딩 30여 곳을 훑어보면 이 5개만 규약에서 벗어난다(나머지는 예외 없이 `Handle*` — 예: `Source/WxGame/Character/WxCharacterBase.cpp:71,82,89`, `Source/WxGame/Controller/WxAIController.cpp:16,35`). `Jump`·`StopJumping` 은 엔진 함수라 대상이 아니다.
- **제안**: `HandleMoveInput`·`HandleLookInput` 등으로 개명하거나, Enhanced Input 바인딩은 규칙 예외임을 CLAUDE.md 에 명시해 판정을 고정한다.
- **확신도**: 중간 (UE 관용 명명이라 의도적 예외일 수 있음)

### 4. 🟢 뷰모델의 UI 커맨드 함수가 `BlueprintCallable` 을 쓴다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:43,46`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:80`, `Source/WxGame/MVVM/WxViewModel_Item.h:47`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5는 `BlueprintCallable` 을 Blueprint Function Library 와 Blueprint Async Action 팩토리 함수로 한정한다. `RequestAdvance`·`RequestInteract`·`RequestCycle`·`SetCurrentCategory`·`RequestUseConsumable` 은 위젯 BP 가 버튼 이벤트에서 호출하는 VM 커맨드로, 그 예외 범주에 들지 않는다. 모듈 내 `BlueprintCallable` 은 이 5개가 전부다.
- **제안**: 실무상 위젯 BP 가 VM 커맨드를 부르려면 이 지정자가 필요하므로, 개별 수정보다 "MVVM 뷰모델의 커맨드 함수"를 규칙 5의 명시적 예외로 CLAUDE.md 에 적어 판정을 고정하는 편이 낫다. 이미 위젯 서브클래스의 1-arg setter 예외가 관행으로 있는 것과 같은 맥락이다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 대안 경로가 사실상 없다)

### 5. 🟢 `_Quest`·`_Dialogue` 뷰모델만 `Initialize` 재진입 가드가 없다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp:10-23`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:8-21`
- **범주**: 중복/복잡도
- **문제**: 형제 뷰모델들은 `Initialize` 첫머리에서 `Deinitialize()` 를 불러 재초기화·소스 교체를 안전하게 만드는데(`WxViewModel_Item.cpp:20,41`, `WxViewModel_Inventory.cpp:38`, `WxViewModel_InteractionList.cpp:35`, 그리고 베이스 `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:15`) 이 둘만 없다. 지금은 리졸버가 1회만 부르므로 무해하지만, 소스 교체가 생기면 `AddDynamic` 이 중복 등록돼 통지가 두 번 들어온다.
- **제안**: 동일하게 `Deinitialize()` 선행 호출을 넣어 규약을 통일한다.
- **확신도**: 중간 (현재 호출 경로에선 발현하지 않음)

### 6. 🟢 클래스 불변식과 어긋나는 널 검사가 한 곳 남았다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:178`
- **범주**: 중복/복잡도
- **문제**: 베이스 헤더가 "서브오브젝트 멤버는 모두 생성자에서 만들어 수명 내내 널이 아니다 — 파생 전부가 널 검사 없이 역참조한다"고 명시하고(`Source/WxGame/Character/WxCharacterBase.h:34`) 지난 리뷰 이후 베이스의 도달 불가 검사는 전부 제거됐는데, `AWxPlayerCharacter::Look` 의 `if (LockOnComponent)` 만 남았다. 같은 함수 바로 위에서 `AbilitySystemComponent` 는 무가드로 역참조하므로(`WxPlayerCharacter.cpp:176`) 한 함수 안에서 규약이 갈린다.
- **제안**: 검사를 제거해 나머지와 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxBossComponent.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h/.cpp`, `Source/WxGame/README.md`, `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxMinion.*`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Input/WxInputConfig.*`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.*`, `Source/WxGame/MVVM/WxViewModel_Quest.*`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.*`, `Source/WxGame/MVVM/WxViewModel_Dialogue.*`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.*`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.*`
- **미검토 / 한계**:
  - 이번에 확인해 둔 사실(재활용용): (1) `AWxBossCharacter` → `UWxBossComponent` 전환으로 월드 전체 스폰 훅이 사라졌고 PIE 월드 필터(`WxViewModel_BossCharacter.cpp:47`)도 형제 VM 들과 같은 규약을 따른다. (2) ASC 는 `UAbilitySystemComponent::InitializeComponent` 가 owner=avatar 로 자체 초기화하므로 `InitAbilitySystem` 이 `RefreshAbilityActorInfo` 만 부르는 것은 정상이다. (3) `EGameplayTagEventType::NewOrRemoved` 는 0↔1 경계에서만 발화하므로 사망 태그 중복 부여로 `HandleDeath` 가 두 번 도는 경로는 없다. (4) FORCEINLINE·람다·Copyright 헤더 누락은 모듈 전체에 0건이다.
  - 참조 확인 목적으로 `WxUI`·`WxInventory`·`WxCombat`·`WxAI`·`WxDialogue` 의 일부 진입점만 열어 봤고 그 모듈들 자체는 리뷰 대상이 아니다.
  - `UWxMetaHumanComponent::OnRegister` 가 등록 도중 다른 컴포넌트를 생성·등록하는 재진입 패턴은 엔진 `UMetaHumanComponentUE` 와 같은 방식이라 문제로 보지 않았으나, 레벨 스트리밍·BP 리컴파일 반복 시나리오는 실측으로 확인하지 않았다.
  - 발견 1의 클라 스냅 증상은 엔진 소스(`GatherCurrentMovement` 의 attach 분기)로 메커니즘만 확인했고, 실제 멀티 세션 재현은 하지 않았다.
  - Experience/ActionSet 에셋 데이터와 BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `3d9e73c0` · 리뷰일 2026-09-04 · 소스 71파일 — `/module-review`로 갱신*
