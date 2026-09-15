# WxGame — 코드 리뷰

> 조립 모듈로서 도메인 경계와 수명·정리 경로는 여전히 탄탄하고, CLAUDE.md 규칙 위반은 없다. 이번 히트스톱 정리로 이동 컴포넌트의 정지 분기와 직전 리뷰 4·6번(`Move` 가드 주석, 즉시 태그 확인)은 해소됐다. 하지만 새 `CustomTimeDilation` 방식에서도 원격 클라 플레이어가 정지 경계에서 위치 보정을 받는 문제는 원인만 바뀌어 남는다. 변경 파일(이동 컴포넌트·캐릭터 베이스·플레이어 캐릭터)은 WxCombat 히트스톱 코드와 UE 5.8 CMC·Actor 소스까지 따라가며 깊게 봤다. 직전 발견이 걸린 파일은 현재 코드로 재검증했고, 나머지는 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 CustomTimeDilation 히트스톱이 원격 클라 폰의 서버 사본에도 걸려, 태그 도착 전에 클라가 보낸 무브를 서버가 잘라 실행한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:39`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp:67`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:245`
- **범주**: 버그/정확성
- **문제**: 이번 변경으로 `UWxCharacterMovementComponent::PerformMovement`의 태그 게이트가 사라졌다. 이제는 `AWxCharacterBase`가 붙이는 `UWxHitStopComponent`가 액터의 `CustomTimeDilation`을 0.001배로 낮춰 멈춘다. 직전 리뷰 1번의 원인이던 이동 컴포넌트 분기는 없어졌지만, 서버와 클라가 서로 다른 시점에 멈추는 문제는 남는다.
  - GE는 서버에서만 걸린다(`WxEffect_HitStop.cpp:26`). 그래서 서버 사본의 배율이 먼저 내려가고, 클라는 태그가 복제될 때까지 정상 속도로 무브를 보낸다.
  - 서버는 받은 무브의 델타를 `p.NetServerMaxMoveDeltaTimeScalar(1.75) × MaxMoveDeltaTime(0.125) × ActorTimeDilation`으로 자른다(엔진 `CharacterMovementComponent.cpp:10022`, `:12641`). 배율이 0.001이면 이 한도가 약 0.2ms라, 그 구간에 도착한 16ms 무브가 0.2ms어치만 실행된다. 결국 클라가 보고한 위치와 서버 결과가 어긋나 보정이 온다.
  - 어긋나는 구간은 대략 min(RTT, 정지 길이)다. RTT 100ms면 기본 0.1초 정지 전체가 여기에 든다. 반대로 해제 쪽은 클라가 작은 델타를 보내고 서버도 그 델타를 그대로 쓰므로 어긋나지 않는다.
  - 적중마다 공격자 자신에게도 `InstigatorHitStop`이 걸린다(`WxWeaponBase.cpp:245`). 그래서 원격 클라 플레이어는 공격을 맞힐 때와 맞을 때마다 되당겨질 수 있다.
  - 시뮬 프록시는 `SimulatedTick`도 배율을 받아, 직전 리뷰에서 지적한 정지 중 외삽은 사라졌다. 다만 태그가 늦게 도착하는 만큼 서버보다 늦게 멈춘다.
  - 지금은 새 게임 진입이 스탠드얼론 전용이라(`Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp:37`) 드러나지 않는 잠재 결함이다. 워크로그 `.codex/worklog/2026-09-16-HitStop-CustomTimeDilation.md`도 2인 PIE 위치 보정을 미확인 항목으로 남겼다.
- **제안**: 서버에서는 원격 클라가 조종하는 폰에 배율을 걸지 않는다. 이 폰은 클라 무브의 타임스탬프대로만 이동·포즈를 진행하므로, 클라가 느려지면 서버도 무브 델타를 따라 같은 만큼 느려진다. 판별에는 b5f00cfc에서 쓰던 엔진 비트(메시의 `bOnlyAllowAutonomousTickPose`)를 쓰고, 고칠 곳은 WxCombat의 `UWxHitStopComponent::SetFrozen`이다. 멀티 정책이 정해질 때까지 미룬다면 이 한계를 문서에 적어 둔다. 어느 쪽이든 2인 PIE(`Net PktLag=100`, `p.NetShowCorrections 1`)에서 보정 빈도를 실측한다.
- **확신도**: 중간 (엔진 경로는 소스로 확인했으나 네트워크 PIE 실측은 하지 않았다)

### 2. 🟡 InventoryItem 뷰모델의 세 진입점이 같은 구독 코드를 복제하고, 그중 두 곳의 도착 구독은 아무 일도 하지 못한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:12`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:23`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:34`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:100`
- **범주**: 중복/복잡도
- **문제**:
  - `StartObserving`·`Initialize(Instance)`·`Initialize(ItemDef)`는 모두 같은 순서를 되풀이한다: `Deinitialize` → 대상 설정 → `ApplyStaticDataFromDef` → `OnAnyInventoryReady`/`OnAnyInventoryEnded` 구독 → `BindSource`. 구독 두 줄(`:18`–`:19`, `:29`–`:30`, `:39`–`:40`)은 글자까지 같다.
  - 두 `Initialize`는 첫 줄의 `Deinitialize()`가 `StopObserving()`(`:115` → `:95`)으로 `ObservedController`를 비운 뒤 다시 세우지 않는다. `HandleInventoryReady`는 `ObservedController.IsValid()`일 때만 연결하므로(`:100`), 이 두 경로의 도착 구독은 신호를 받아도 아무것도 하지 않는다.
  - `UWxViewModel_Inventory`는 슬롯마다(`WxViewModel_Inventory.cpp:151`), 획득마다(`WxViewModel_Inventory.cpp:115`) 이 경로로 자식을 만든다. 그만큼 쓸모없는 정적 델리게이트 바인딩이 쌓이고, 구독 조건이 바뀌면 세 곳을 함께 고쳐야 한다.
- **제안**: "정적 데이터 적용 + `OnAnyInventoryEnded` 구독 + `BindSource`"를 private 헬퍼 하나로 모은다. `OnAnyInventoryReady` 구독은 `ObservedController`를 세우는 `StartObserving`에만 둔다.
- **확신도**: 높음

### 3. 🟢 AI 청각은 여전히 Neutral 팀을 적대로 듣는다
- **위치**: `Source/WxGame/Controller/WxAIController.h:27`, `Source/WxGame/Controller/WxAIController.cpp:31`, `Source/WxGame/Controller/WxAIController.cpp:49`, `Source/WxGame/Character/WxCharacterBase.cpp:166`
- **범주**: 버그/정확성
- **문제**: 시야는 `GetTeamAttitudeTowards`를 빙의 폰의 규칙에 맡겨(`:49`) Neutral을 걸러낸다. 청각은 이 규칙을 거치지 않는다.
  - 청각은 `FAISenseAffiliationFilter::ShouldSenseTeam(Listener.TeamIdentifier, Event.TeamIdentifier, ...)`로 팀 ID끼리 판정한다(엔진 `AISense_Hearing.cpp:157`, `AIPerceptionTypes.h:231`).
  - 프로젝트는 솔버를 등록하지 않으므로 기본 솔버 `A != B ? Hostile : Friendly`(엔진 `AIInterfaces.cpp:32`)가 쓰인다. `EWxTeam::Neutral = 255`는 그저 다른 팀이 되고, 이벤트 팀은 소음을 낸 액터의 팀 ID다(`AISense_Hearing.cpp:29`).
  - 따라서 Neutral 폰의 발소리(`WxAnimNotify_ReportNoise`)는 적 AI에게 적대로 들리고, 청각의 `bDetectNeutrals = false`(`:31`)는 효과가 없다. 반대로 Neutral 팀 AI는 모든 팀의 소리를 적대로 듣는다.
  - `UWxBTService_UpdateTargetActor::FindPerceivedTarget`(`Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:57`–`:60`)은 적대 여부를 다시 보지 않고 그 대상을 타겟으로 잡는다. 하지만 공격은 `IsHostile`에 막혀 추격만 되풀이한다.
  - 헤더 주석(`WxAIController.h:27`)이 이 차이를 적어 두었다. 코드에서 Neutral 팀을 쓰는 곳은 판정문 하나(`WxCharacterBase.cpp:166`)뿐이라 아직 잠재 결함이다.
- **제안**: `AWxCharacterBase::GetTeamAttitudeTowards`의 Neutral 규칙을 `FGenericTeamId::SetAttitudeSolver`로 등록해, 팀 ID 경로도 같은 규칙을 쓰게 한다. 청각의 차이를 그대로 둘 거라면 Neutral 팀을 도입하기 전에 타겟 선정에서 적대 판정을 한 번 더 거친다.
- **확신도**: 중간 (엔진 경로는 확인했으나 Neutral 팀 콘텐츠가 아직 없다)

### 4. 🟢 처형 프롬프트와 처형 자격이 서로 다른 근거로 판정된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:97`, `Source/WxGame/Character/WxEnemyCharacter.cpp:131`, `Source/WxGame/Character/WxEnemyCharacter.cpp:134`
- **범주**: 설계/구조
- **문제**:
  - `GetInteractionPrompt`(`:134`)는 로컬 플레이어 폰의 ASC에서 `UWxAbility_Finisher` 스펙을 찾아 문구를 읽고, 없으면 빈 `FText`를 돌려준다.
  - 반면 `CanInteract`(`:97`)는 적대·생존·태그·후방 원뿔만 보고, 상호작용자가 처형 어빌리티를 가졌는지는 보지 않는다.
  - 그래서 처형 어빌리티가 없는 폰이 적의 뒤로 다가가면 스캐너 목록에 빈 줄이 뜬다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:80`). 그 항목으로 상호작용하면 서버 검증은 통과해 `Event.Finisher`(`:131`)를 보내지만, 받을 어빌리티가 없어 아무 일도 일어나지 않는다.
- **제안**: `CanInteract`에서 `Interactor`의 ASC에 `UWxAbility_Finisher` 스펙이 있는지도 확인해, 프롬프트와 자격이 같은 근거를 쓰게 한다.
- **확신도**: 중간 (모든 플레이어 폰의 AbilitySet이 처형 어빌리티를 부여한다면 증상은 없다)

### 5. 🟢 InteractionList의 도착 신호 대기 경로는 실행되지 않고, 즉시 연결 경로는 `ObservedController`를 곧바로 비운다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:17`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:22`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:26`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:31`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:105`
- **범주**: 중복/복잡도
- **문제**:
  - `StartObserving`은 `ObservedController = PC`(`:17`)를 세운 뒤 스캐너가 있으면 `Initialize(Scanner)`(`:22`)를 부른다. 그런데 `Initialize` 첫 줄의 `Deinitialize()`(`:31`)가 `StopObserving()`(`:49` → `:111`)을 거쳐 그 필드를 다시 비운다.
  - 스캐너가 없을 때 쓰는 `OnAnyScannerReady` 대기(`:26`)는 실제로 실행되지 않는다. 같은 함수 주석(`:19`)대로 스캐너는 `AWxPlayerController` 생성자 컴포넌트이기 때문이다. 생성자에 스캐너가 없는 PC라면 신호가 영영 오지 않는다.
  - `HandleScannerReady`의 `StopObserving()`(`:105`)은 바로 뒤 `Initialize`가 같은 일을 하므로 중복이다.
  - 지금 증상은 없다. 하지만 재연결 경로를 넣으려는 사람이 이 필드를 믿으면 그대로 함정에 빠진다.
- **제안**: 주석의 전제를 계약으로 삼아 `StartObserving`을 "스캐너를 찾아 `Initialize`"로 줄이고, `ObservedController`·`ScannerReadyHandle`·`HandleScannerReady`를 걷어낸다. 대기 경로를 남길 거라면 `Initialize` 뒤에 `ObservedController`를 다시 세운다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**:
  - 이번 변경분: `Source/WxGame/Character/Component/WxCharacterMovementComponent.h`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`와 7d1d0374..e0106372a diff(삭제된 `Source/WxGame/Tests/WxHitStopMovementTest.cpp` 포함).
  - 직전 발견 재검증과 핵심 로직: `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Controller/WxAIController.h`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.h`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`.
  - 발견 검증용 외부 코드:
    - WxCombat·WxAI·WxWorld·WxUI·WxInventory: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxHitStopComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`.
    - 엔진: `CharacterMovementComponent.cpp`(서버 무브 델타 한도·시간 불일치 검출), `Actor.h`·`Actor.cpp`(`CustomTimeDilation`·`GetActorTimeDilation`), `AIPerceptionTypes.h`, `AISense_Hearing.cpp`, `AIInterfaces.cpp`, `GenericTeamAgentInterface.h`.
    - 워크로그: `.codex/worklog/2026-09-16-HitStop-CustomTimeDilation.md`, `.claude/worklog/2026-09-15-히트스톱-시간소유-머신-정지.md`, `.claude/worklog/2026-09-15-Move-히트스톱-가드-제거.md`.
- **훑은 파일**:
  - 소스: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Controller/WxPlayerController.h`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/WxNpc.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.h`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`.
  - 일괄 검사: 소스 첫 줄 Copyright는 57파일 모두 통과했다. `FORCEINLINE`·`inline`·헤더 본문 정의·람다는 0건이다. 나머지 헤더(`WxGame.h`, `WxTeamTypes.h`, `WxPlayerState.h` 등)는 이 일괄 검사로만 확인했다.
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE·네트워크 실행은 하지 않았다. 발견 1의 보정 규모와 빈도는 실측이 필요하다.
  - BP·WBP 내부와 BP 디폴트 값(플레이어 폰별 AbilitySet 구성, `Team` 설정, 공격 몽타주의 루트모션 여부)은 범위 밖이다. 따라서 발견 1·3·4가 실제로 얼마나 드러나는지는 콘텐츠 확인이 필요하다.
  - 히트스톱 배율 적용은 WxCombat 코드라, 액터의 다른 틱 컴포넌트(락온·입력 버퍼·무기 자식 액터 VFX)가 함께 느려지는 영향은 WxCombat 리뷰에서 다룬다.
  - 직전 리뷰에서 뺀 두 항목은 코드에 그대로 남아 있지만, 실질 영향이 없어 이번에도 뺐다: `UWxMetaHumanComponent::OnUnregister`의 틱 옵션 복원 비대칭(`WxMetaHumanComponent.cpp:56`·`:131`), `AWxEnemyCharacter::BeginPlay`의 무동작 `SetReplicationMode(Full)`(`WxEnemyCharacter.cpp:45`).
  - 사용자 메모리의 히트스톱 기록(GlobalAnimRateScale 방식)과 처형 기록(독립 Backstab 클래스)은 현재 코드와 다르다. 발견은 현재 코드를 기준으로 판정했다.

---
*문서 기준 커밋 `e0106372a` · 리뷰일 2026-09-16 · 소스 57파일 — `/module-review`로 갱신*
