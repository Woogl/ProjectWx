# WxGame — 코드 리뷰

> 전반적으로 건강한 모듈이다. Lyra Experience 파이프라인 이식은 로드 상태 머신·PIE 격리·실패 경로까지 원본에 충실하고, 캐릭터·컨트롤러 계층은 서버/클라 분기와 시뮬 프록시 처리가 일관되며, 규칙 위반(FORCEINLINE·람다·Copyright 누락)은 입력 콜백 명명 한 건을 빼면 전무하다. 이번 리뷰는 71개 소스 전부를 훑고 Framework(Experience 파이프라인)·Character·MVVM 뷰모델의 cpp 로직까지 내려가 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 토스트용 아이템 뷰모델이 Deinitialize 없이 버려져 인벤토리 델리게이트에 남는다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Inventory.cpp:104-113`
- **범주**: 성능/안전
- **문제**: 획득(`Delta > 0`)마다 새 `UWxViewModel_Item` 을 만들어 `Initialize(Inventory, ItemDef)` 를 부르는데, 그 Def 모드 초기화는 `OnInventoryStackChanged`·`OnInventoryChargeChanged` 두 델리게이트에 자신을 등록한다(`Source/WxGame/MVVM/WxViewModel_Item.cpp:45-46`). 직전 `LastAcquiredItem` 은 `Deinitialize()` 없이 교체만 되므로, GC 가 수거해 `BeginDestroy → Deinitialize`(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp:31-35`)가 돌기 전까지 버려진 VM 들이 계속 구독 상태로 쌓인다. 특히 `HandleChargeChanged` 는 바인딩 인스턴스가 없으면 ItemDef 로 매칭하므로(`WxViewModel_Item.cpp:126`), 죽은 토스트 VM 들이 충전 변경마다 `RefreshChargeIcon → RequestImageAsync` 로 스트리밍 요청까지 다시 낸다. 포션을 연타로 마시는 구간에서 구독자 수가 획득 횟수만큼 선형으로 늘어난다.
- **제안**: `LastAcquiredItem` 교체 직전에 이전 VM 의 `Deinitialize()` 를 부른다. 더 나은 방향은 토스트 VM 을 구독 없는 스냅샷으로 만드는 것 — 표시에 필요한 것은 `ApplyStaticDataFromDef` 결과와 `AcquiredCount` 뿐이라 델리게이트 등록이 애초에 필요 없다.
- **확신도**: 높음

### 2. 🟡 스폰된 적을 스포너에 Attach 해 이동 복제가 AttachmentReplication 경로로 바뀐다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:183`
- **범주**: 설계/구조
- **문제**: 코드 주석이 이미 "멀티 검증 때 재확인할 것"으로 남긴 위험의 구체적 결과다. 루트가 attach 되면 엔진은 절대 위치(`ReplicatedMovement`)를 채우지 않고 상대 오프셋(`AttachmentReplication`)만 보낸다(UE 5.8 `ActorReplication.cpp` 의 "If we are attached, don't replicate absolute position, use AttachmentReplication instead" 분기). 그 결과 시뮬레이트 프록시 적의 위치는 `ACharacter` 의 네트워크 스무딩이 아니라 `OnRep_AttachmentReplication` 의 상대 트랜스폼 하드 세팅으로 갱신되어, 리슨/데디 서버 클라에서 이동이 끊겨 보인다. 정작 부착이 필요한 이유는 `UWxPatrolComponent::FindPatrolComponent` 의 역참조 하나뿐이며, 그 링크는 이미 `OwningSpawner` 약참조가 쥐고 있다(`WxEnemyCharacter.cpp:178`).
- **제안**: `AttachToActor` 를 걷어내고 정찰 경로 조회를 `GetOwningSpawner()` 경유로 바꾼다. 아웃라이너 그룹핑이 목적이면 부착 대신 `SetOwner`/에디터 폴더로 대체한다.
- **확신도**: 중간 (싱글·호스트 플레이에선 증상이 없어 의도된 절충일 수 있음)

### 3. 🟡 입력 바인딩 콜백에 `Handle` 접두사가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:62-67`, 바인딩 지점 `Source/WxGame/Character/WxPlayerCharacter.cpp:111,115,124,129,130`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 4는 "Delegate 에 바인딩되는 Callback 함수는 `Handle` 을 Prefix 로" 한다. `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 는 Enhanced Input `BindAction` 이 만드는 델리게이트의 콜백인데 접두사가 없다. 모듈 내 다른 델리게이트 바인딩은 예외 없이 `Handle*` 이라(예: `Source/WxGame/Character/WxCharacterBase.cpp:71,82,91,231`, `Source/WxGame/Controller/WxAIController.cpp:16,35`) 이 5개만 규약에서 벗어난다.
- **제안**: `HandleMoveInput`·`HandleLookInput` 등으로 개명하거나, Enhanced Input 바인딩은 규칙 예외임을 CLAUDE.md 에 명시해 판정을 고정한다.
- **확신도**: 중간 (UE 관용 명명이라 의도적 예외일 수 있음)

### 4. 🟢 보스 뷰모델이 월드 전체 스폰 훅을 끝까지 유지한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp:17,42-47`
- **범주**: 성능/안전
- **문제**: `AddOnActorSpawnedHandler` 는 월드의 모든 액터 스폰마다 불린다. 보스를 이미 물었어도 해제하지 않아 오픈월드 스트리밍 중 스폰되는 전 액터에 대해 `Cast<AWxBossCharacter>` 가 계속 돈다. 같은 "늦게 도착하는 소스를 기다린다" 문제를 푸는 형제 뷰모델들은 연결 성공 시 `StopObserving()` 으로 관찰을 끝내고 클래스 정적 준비 신호(`OnAnyInventoryReady`·`OnAnyScannerReady`)를 쓴다(`Source/WxGame/MVVM/WxViewModel_Inventory.cpp:28,199`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:105-115`). 보스 VM 만 그 규약 밖이다.
- **제안**: `AWxBossCharacter` 가 BeginPlay 에서 클래스 정적 델리게이트를 쏘게 하고 VM 은 그것만 구독한다 — 전 액터 스폰 훅이 사라지고 다른 VM 들과 패턴도 맞는다.
- **확신도**: 중간

### 5. 🟢 `_Quest`·`_Dialogue` 뷰모델만 `Initialize` 재진입 가드가 없다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp:10-24`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:8-21`
- **범주**: 중복/복잡도
- **문제**: 형제 뷰모델 셋(`_Item`·`_Inventory`·`_InteractionList`)은 `Initialize` 첫머리에서 `Deinitialize()` 를 불러 재초기화·소스 교체를 안전하게 만드는데(`WxViewModel_Item.cpp:20,41`, `WxViewModel_Inventory.cpp:38`, `WxViewModel_InteractionList.cpp:35`) 이 둘만 없다. 지금은 리졸버가 1회만 부르므로 무해하지만, 소스 교체가 생기면 `AddDynamic` 이 중복 등록돼 통지가 두 번 들어온다.
- **제안**: 동일하게 `Deinitialize()` 선행 호출을 넣어 규약을 통일한다.
- **확신도**: 중간 (현재 호출 경로에선 발현하지 않음)

### 6. 🟢 클래스가 스스로 선언한 불변식과 어긋나는 널 검사가 남아 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:89,94,169,304`
- **범주**: 중복/복잡도
- **문제**: 헤더가 "생성자 서브오브젝트라 수명 내내 널이 아니다 — 파생 전부가 널 검사 없이 역참조한다"고 명시하고(`Source/WxGame/Character/WxCharacterBase.h:77`) 실제로 `AbilitySystemComponent` 는 같은 파일에서 무가드로 역참조하는데(`WxCharacterBase.cpp:128,133,164`), `EquipmentComponent`·`WeaponActor` 만 네 곳에서 널 검사를 한다. 도달 불가능한 분기라 읽는 쪽에 "널일 수 있다"는 잘못된 신호를 준다.
- **제안**: 넷 다 제거하거나, 반대로 헤더의 불변식 주석을 실제 코딩 관행에 맞게 고쳐 하나로 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/README.md`, `Source/WxGame/Framework/WxExperienceDefinition.*`, `Source/WxGame/Framework/WxExperienceActionSet.*`, `Source/WxGame/Framework/WxGameState.*`, `Source/WxGame/Framework/WxWorldSettings.*`, `Source/WxGame/Controller/WxPlayerController.*`, `Source/WxGame/Player/WxPlayerState.*`, `Source/WxGame/Character/WxNpc.*`, `Source/WxGame/Character/WxMinion.*`, `Source/WxGame/Character/WxBossCharacter.*`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Input/WxInputConfig.*`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.*`, `Source/WxGame/MVVM/WxViewModel_Quest.*`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.*`, `Source/WxGame/MVVM/WxViewModel_Dialogue.*`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.*`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.*`
- **미검토 / 한계**:
  - 참조 확인 목적으로 `WxUI`·`WxInventory`·`WxWorld` 의 일부 진입점(`UWxViewModel`, `UWxInventoryComponent::FindInventory`, `UWxInteractionScannerComponent` 의 broadcast 지점)만 열어 봤고 그 모듈들 자체는 리뷰 대상이 아니다.
  - `UWxMetaHumanComponent::OnRegister` 가 등록 도중 다른 컴포넌트를 생성·등록하는 재진입 패턴은 엔진 `UMetaHumanComponentUE` 와 같은 방식이라 문제로 보지 않았으나, 레벨 스트리밍·BP 리컴파일 반복 시나리오는 실측으로 확인하지 않았다.
  - 발견 2의 클라 스냅 증상은 엔진 소스(`ActorReplication.cpp` 의 attach 분기)로 메커니즘만 확인했고, 실제 멀티 세션 재현은 하지 않았다.
  - Experience/ActionSet 에셋 데이터와 BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 71파일 — `/module-review`로 갱신*
