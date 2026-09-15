# WxGame — 코드 리뷰

> 조립 모듈로서 도메인 경계와 수명·정리 경로는 여전히 탄탄하고, CLAUDE.md 규칙 위반은 없다. 이번 변경 중 AI 시야의 피아 판정 위임은 직전 지적의 절반(시야)만 해소했고, 새 히트스톱 이동 정지는 복제 태그가 도착하는 시점에 묶여 있어 원격 클라이언트에서 이동 보정이 구조적으로 남는다. 변경 파일(이동 컴포넌트·AI 컨트롤러·히트스톱 테스트)과 캐릭터·MVVM 핵심은 깊게 읽었고, WxCombat·WxAI 연관 코드와 UE 5.8 엔진 이동·퍼셉션 소스까지 따라가 검증했다. 나머지 파일은 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 2 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 히트스톱 이동 정지가 복제 태그 도착 시점에 묶여 원격 클라에서 위치 보정·외삽 튐을 만든다
- **위치**: `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp:60`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp:63`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:245`
- **범주**: 버그/정확성
- **문제**: 정지 여부는 `Effect.HitStop` 태그로만 판정한다. 그런데 GE는 서버에서만 걸리므로(`WxEffect_HitStop.cpp:26`) 클라에는 태그가 붙을 때도, 풀릴 때도 복제 지연만큼 늦게 반영된다. 자율 프록시의 이동은 클라가 먼저 `ReplicateMoveToServer` → `PerformMovement`(엔진 `CharacterMovementComponent.cpp:8907`, `:9024`)로 시뮬레이션하고, 서버는 그 무브가 도착한 순간의 서버 태그 상태로 `MoveAutonomous`(`:10664`, `:10692`)를 다시 돌린다. 정지 여부는 세이브드 무브에 실리지 않는다. 그래서 히트스톱 시작 직후에는 서버만 멈추고, 끝날 무렵에는 클라만 멈춘 무브가 생긴다. 이 구간에서 클라가 보고한 위치와 서버 결과가 어긋나 보정이 온다. 보정 뒤의 재생(`ClientUpdatePositionAfterServerUpdate` → `MoveAutonomous`, `:8686`)도 과거 무브를 현재 태그로 다시 돌리므로, 경계 무브는 또 다르게 재현된다. 적중마다 공격자 자신에게도 `InstigatorHitStop`이 걸리므로(`WxWeaponBase.cpp:245`) 원격 클라 플레이어는 공격이 맞을 때마다 앞뒤로 보정을 받을 수 있다. 워크로그(`.codex/worklog/2026-09-14-히트스톱-이동-네트워크-시간-유지.md`)가 미실측으로 남긴 "방향키 유지 중 적중 후 위치 어긋남"이 남는 구조다. 또 루트모션 몽타주 없이 움직이던 시뮬 프록시는 `PerformMovement`가 아니라 `SimulateMovement`(`:2055`)를 탄다. 이 경로는 태그를 보지 않으므로, 서버가 보존해 둔 속도로 `MoveSmooth(Velocity)`(`:2297`) 외삽을 계속한다. 서버 위치·속도가 그대로라 새 복제 갱신도 오지 않고, 해제 뒤 되당겨진다. 스포너에 부착되지 않은 캐릭터(레벨 배치 적, 다른 클라가 보는 플레이어)가 해당한다. 회귀 테스트(`Source/WxGame/Tests/WxHitStopMovementTest.cpp:52`)는 넷 롤 없이 `PerformMovement`를 직접 부르므로 이 경로를 잡지 못한다.
- **제안**: 프로젝트 방침상 정지 여부를 무브 데이터에 싣는 예측 장치를 들이지 않는다면, 둘 중 하나를 고른다. (a) 원격 조종 폰에 정지 태그가 있는 동안에는 서버에서 엔진 표준 옵션인 `bIgnoreClientMovementErrorChecksAndCorrection`(또는 `bServerAcceptClientAuthoritativePosition`)을 켜 경계 보정을 끈다. (b) 이동 정지는 보정 문제가 없는 머신(스탠드얼론, 서버가 직접 조종하는 폰·AI 폰)에만 걸고, 원격 자율 프록시는 애니메이션 정지와 입력 가드로 대신한다. 시뮬 프록시는 `SimulateMovement` 경로에도 같은 태그 가드를 둔다. 어느 쪽이든 2인 네트워크 PIE에서 보정 발생 여부를 실측한다.
- **확신도**: 중간 (엔진 경로는 소스로 확인했다. 현재 게임 진입은 스탠드얼론 전용이라 네트워크 PIE에서만 드러나며, 실측은 하지 않았다.)

### 2. 🟡 InventoryItem 뷰모델의 세 진입점이 같은 구독 코드를 복제하고, 그중 두 곳의 도착 구독은 아무 일도 하지 못한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:12`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:23`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:34`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:100`
- **범주**: 중복/복잡도
- **문제**: `StartObserving`·`Initialize(Instance)`·`Initialize(ItemDef)`는 모두 `Deinitialize` → 대상 설정 → `ApplyStaticDataFromDef` → `OnAnyInventoryReady`/`OnAnyInventoryEnded` 구독 → `BindSource` 순서를 반복한다. 구독 두 줄(`:18`–`:19`, `:29`–`:30`, `:39`–`:40`)은 글자까지 같다. 게다가 두 `Initialize`는 첫 줄의 `Deinitialize()`가 `StopObserving()`(`:115` → `:95`)으로 `ObservedController`를 비운 뒤 다시 세우지 않는다. `HandleInventoryReady`는 `ObservedController.IsValid()`일 때만 연결하므로(`:100`), 이 두 경로의 도착 구독은 신호를 받아도 아무것도 하지 않는다. `UWxViewModel_Inventory`는 슬롯마다(`WxViewModel_Inventory.cpp:151`), 획득마다(`WxViewModel_Inventory.cpp:115`) 이 경로로 자식을 만든다. 그만큼 쓸모없는 정적 델리게이트 바인딩이 쌓이고, 구독 조건이 바뀌면 세 곳을 함께 고쳐야 한다.
- **제안**: "정적 데이터 적용 + `OnAnyInventoryEnded` 구독 + `BindSource`"를 private 헬퍼 하나로 모은다. `OnAnyInventoryReady` 구독은 `ObservedController`를 세우는 `StartObserving`에만 둔다.
- **확신도**: 높음

### 3. 🟢 AI 청각은 여전히 Neutral 팀을 적대로 듣는다
- **위치**: `Source/WxGame/Controller/WxAIController.h:27`, `Source/WxGame/Controller/WxAIController.cpp:31`, `Source/WxGame/Controller/WxAIController.cpp:49`, `Source/WxGame/Character/WxCharacterBase.cpp:176`
- **범주**: 버그/정확성
- **문제**: 이번 변경으로 시야는 `GetTeamAttitudeTowards`를 빙의 폰의 규칙에 맡겨(`:49`) Neutral을 걸러낸다. 하지만 청각은 `FAISenseAffiliationFilter::ShouldSenseTeam(Listener.TeamIdentifier, Event.TeamIdentifier, ...)`(엔진 `AISense_Hearing.cpp:157`)로 팀 ID끼리 판정한다. 프로젝트는 솔버를 등록하지 않으므로 기본 솔버 `A != B ? Hostile : Friendly`(엔진 `AIInterfaces.cpp:32`)가 쓰이고, `EWxTeam::Neutral = 255`는 그저 다른 팀이 된다. 이벤트 팀은 소음을 낸 액터의 팀 ID다(`AISense_Hearing.cpp:29`). 따라서 Neutral 폰의 발소리(`WxAnimNotify_ReportNoise`, 조깅 애니메이션에 배치됨)는 적 AI에게 적대로 들리고, `bDetectNeutrals = false`(`:31`)는 효과가 없다. 반대로 Neutral 팀 AI는 모든 팀의 소리를 적대로 듣는다. 적대 여부를 다시 보지 않는 `UWxBTService_UpdateTargetActor::FindPerceivedTarget`(`Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:57`)은 그 대상을 그대로 타겟으로 잡지만, 공격은 `IsHostile`에 막혀 추격만 반복한다. 헤더 주석(`WxAIController.h:27`)이 이 차이를 적어 두었고, `Content`에서 `EWxTeam::Neutral`을 쓰는 에셋은 검색되지 않아 아직 잠재 결함이다.
- **제안**: `AWxCharacterBase::GetTeamAttitudeTowards`의 Neutral 규칙을 `FGenericTeamId::SetAttitudeSolver`로 등록해 팀 ID 경로도 같은 규칙을 쓰게 한다. 청각 차이를 받아들이려면 Neutral 팀 도입 전에 타겟 선정에서 적대 판정을 한 번 더 거친다.
- **확신도**: 중간 (엔진 경로는 확인했으나 Neutral 팀 콘텐츠가 아직 없다)

### 4. 🟢 `Move`의 히트스톱 가드 주석이 사라진 "이동 틱 정지" 구조를 근거로 든다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:155`, `Source/WxGame/Character/WxPlayerCharacter.cpp:156`
- **범주**: 중복/복잡도
- **문제**: 주석은 "히트스톱이 이동 틱을 세우는 동안 입력이 소비되지 않고 쌓인다"를 가드의 이유로 든다. 하지만 985aabe9 이후 이동 틱은 유지되고(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxHitStopComponent.h:19`), `TickComponent`가 틱마다 맨 앞에서 `ConsumeInputVector()`로 입력을 비운다(엔진 `CharacterMovementComponent.cpp:1662`). 입력이 쌓이지 않으므로 적힌 근거는 더 이상 성립하지 않는다. 가드에 남은 효과는 정지 중 가속을 0으로 만드는 것뿐이다. `Jump`의 가드(`:135`)는 `CheckJumpInput`이 `PerformMovement` 밖(엔진 `:6448`)에서 속도·이동 모드를 바꾸므로 여전히 필요하다.
- **제안**: 가드를 지운다. 정지 중 가속을 0으로 두려는 이유가 따로 있다면(애님이 가속을 읽는 경우 등) 그 이유로 주석을 고친다.
- **확신도**: 높음

### 5. 🟢 처형 프롬프트와 처형 자격이 서로 다른 근거로 판정된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:97`, `Source/WxGame/Character/WxEnemyCharacter.cpp:131`, `Source/WxGame/Character/WxEnemyCharacter.cpp:134`
- **범주**: 설계/구조
- **문제**: `GetInteractionPrompt`(`:134`)는 로컬 플레이어 폰의 ASC에서 `UWxAbility_Finisher` 스펙을 찾아 문구를 읽고, 없으면 빈 `FText`를 돌려준다. 반면 `CanInteract`(`:97`)는 적대·생존·태그·후방 원뿔만 보고, 상호작용자가 처형 어빌리티를 가졌는지는 보지 않는다. 처형 어빌리티를 받지 않은 폰이 적의 뒤로 다가가면 스캐너 목록에 빈 줄이 뜬다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:80`). 그 항목으로 상호작용하면 서버 검증은 통과해 `Event.Finisher`(`:131`)를 보내지만, 받을 어빌리티가 없어 아무 일도 일어나지 않는다.
- **제안**: `CanInteract`에서 `Interactor`의 ASC에 `UWxAbility_Finisher` 스펙이 있는지도 확인해, 프롬프트와 자격이 같은 근거를 쓰게 한다.
- **확신도**: 중간 (모든 플레이어 폰의 AbilitySet이 처형 어빌리티를 부여한다면 증상은 없다)

### 6. 🟢 `PostInitializeComponents`의 즉시 태그 확인 두 곳은 참이 될 수 없다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:60`, `Source/WxGame/Character/WxCharacterBase.cpp:61`, `Source/WxGame/Character/WxCharacterBase.cpp:71`
- **범주**: 중복/복잡도
- **문제**: 주석(`:60`)은 "late join 시 구독보다 먼저 초기 복제로 태그가 실려 왔을 수 있다"를 근거로 들지만, 어느 머신에서도 그런 순서가 생기지 않는다. 권위 측은 ASC가 막 생성된 상태다. 적은 `Super::PostInitializeComponents()`(`:54`) 안의 자동 빙의(엔진 `Pawn.cpp:157`)로 어빌리티셋 부여까지만 끝났을 뿐이라 `Ability.Death`·`State.Ragdoll`이 서 있을 수 없다. 원격 측에서 동적 액터는 지연 생성 없이 스폰되므로(엔진 `PackageMapClient.cpp:768`) 초기 복제가 이 함수 뒤에 적용되고, 레벨 배치 액터는 로드 시점에 이미 이 함수를 지난다. 태그는 항상 바로 위에서 등록한 콜백으로 들어온다. 결국 두 분기는 실행되지 않는 코드이고, 실제 안전망은 등록 콜백과 `AWxEnemyCharacter::BeginPlay`의 재확인(`WxEnemyCharacter.cpp:50`–`:55`)이다. 주석이 없는 안전망을 있다고 믿게 만든다.
- **제안**: 두 즉시 확인을 지우고, 주석에는 콜백을 여기서 등록하는 이유만 남긴다.
- **확신도**: 중간 (Iris 미사용 기준의 엔진 스폰 순서에 근거한다)

### 7. 🟢 InteractionList의 도착 신호 대기 경로는 실행되지 않고, 즉시 연결 경로는 `ObservedController`를 곧바로 비운다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:17`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:22`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:26`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:31`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:105`
- **범주**: 중복/복잡도
- **문제**: `StartObserving`은 `ObservedController = PC`(`:17`)를 세운 뒤 스캐너가 있으면 `Initialize(Scanner)`(`:22`)를 부른다. 그런데 `Initialize` 첫 줄의 `Deinitialize()`(`:31`)가 `StopObserving()`(`:49` → `:111`)을 거쳐 그 필드를 다시 비운다. 스캐너가 없을 때의 `OnAnyScannerReady` 대기(`:26`)는, 같은 함수 주석(`:19`)대로 스캐너가 `AWxPlayerController` 생성자 컴포넌트라 실제로 실행되지 않는다. 생성자에 스캐너가 없는 PC라면 신호가 영영 오지 않는다. `HandleScannerReady`의 `StopObserving()`(`:105`)도 곧이어 `Initialize`가 같은 일을 반복한다. 지금 증상은 없지만, 재연결 경로를 넣으려는 사람이 이 필드를 믿으면 그대로 함정에 빠진다.
- **제안**: 주석의 전제를 계약으로 삼아 `StartObserving`을 "스캐너를 찾아 `Initialize`"로 줄이고, `ObservedController`·`ScannerReadyHandle`·`HandleScannerReady`를 걷어낸다. 대기 경로를 유지할 것이라면 `Initialize` 뒤에 `ObservedController`를 다시 세운다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/Component/WxCharacterMovementComponent.h`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.h`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Tests/WxHitStopMovementTest.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.h`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp` 및 대응 헤더. 발견 검증을 위해 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`와 엔진 `CharacterMovementComponent.cpp`·`AIPerceptionComponent.cpp`·`AIPerceptionTypes.h`·`AISense_Sight.cpp`·`AISense_Hearing.cpp`·`AIInterfaces.cpp`·`PackageMapClient.cpp`·`Pawn.cpp`·`AbilitySystemComponent.cpp`, 그리고 히트스톱 관련 `.codex/worklog` 두 건을 읽었다.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp` 및 대응 헤더. 일괄 검사 결과, 소스 첫 줄 Copyright는 58파일 모두 통과했고 `FORCEINLINE`·`inline`·헤더 본문 정의·람다는 0건이다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE·네트워크 실행은 하지 않았다. 따라서 발견 1의 보정 규모와 빈도는 실측이 필요하다. BP·WBP 내부와 BP 디폴트 값(플레이어 폰별 AbilitySet 구성, `Team` 설정, 공격·피격 몽타주의 루트모션 여부)은 범위 밖이라 발견 1·3·5의 실제 발현 범위는 콘텐츠 확인이 필요하다. 직전 리뷰의 `UWxMetaHumanComponent::OnUnregister` 표시·틱 옵션 복원 비대칭(`WxMetaHumanComponent.cpp:54`·`:56`·`:131`)과 `AWxEnemyCharacter::BeginPlay`의 무동작 `SetReplicationMode(Full)`(`WxEnemyCharacter.cpp:45`)은 코드에 그대로 남아 있다. 다만 실질 영향이 없어 이번 목록에서는 뺐다. `WxMetaHumanComponent`의 LOD 매핑 산식과 엔진 MetaHuman 컴포넌트 내부 동작은 에디터 실행 없이는 검증하지 못했다.

---
*문서 기준 커밋 `7d1d0374` · 리뷰일 2026-09-15 · 소스 58파일 — `/module-review`로 갱신*
