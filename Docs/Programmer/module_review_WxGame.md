# WxGame — 코드 리뷰

> 조립 모듈로서 도메인 경계와 수명·정리 경로가 전반적으로 탄탄하고, 이번에도 치명적 결함은 확인하지 못했다. 직전 리뷰 지적 중 프론트엔드 위젯 하드코딩과 Quest·Dialogue 주석 불일치는 해소됐고, 새로 AI 퍼셉션의 피아 판정이 캐릭터의 Neutral 규칙을 따르지 않는 잠재 결함을 찾았다. 모듈 아래 h/cpp 58개를 모두 열어 읽었고, 캐릭터·AI 컨트롤러·MVVM 뷰모델·게임 흐름은 연관 플러그인과 UE 5.8 엔진 소스까지 따라가 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 AI 시야·청각의 피아 판정이 캐릭터의 Neutral 규칙을 따르지 않는다
- **위치**: `Source/WxGame/Controller/WxAIController.cpp:26`, `Source/WxGame/Controller/WxAIController.cpp:31`, `Source/WxGame/Controller/WxAIController.cpp:66`, `Source/WxGame/Character/WxCharacterBase.cpp:176`
- **범주**: 버그/정확성
- **문제**: 팀 규칙은 `AWxCharacterBase::GetTeamAttitudeTowards`에만 있다 — 어느 한쪽이 `EWxTeam::Neutral`이면 `Neutral`을 돌려준다. 그런데 엔진 시야·청각 센스는 리스너의 팀 주체를 퍼셉션 컴포넌트 오너인 `AWxAIController`로 잡고(엔진 `AIPerceptionTypes.cpp:140`), `AAIController`는 `GetTeamAttitudeTowards`를 재정의하지 않아 `FGenericTeamId::GetAttitude(팀ID, 팀ID)`로 떨어진다. 프로젝트는 `FGenericTeamId::SetAttitudeSolver`를 한 번도 부르지 않으므로 엔진 기본 솔버 `A != B ? Hostile : Friendly`(엔진 `AIInterfaces.cpp:32`)가 쓰이고, `Neutral = 255`는 그저 다른 팀이라 `Hostile`이 된다. 결과적으로 `bDetectNeutrals = false`(`:26`, `:31`)는 Wx 폰을 한 번도 걸러내지 못한다. 반면 대미지 센스 보고(`Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp:176`)와 대미지 판정(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:27`)은 폰의 규칙을 쓴다. 실패 시나리오: BP에서 `Team`을 `Neutral`로 둔 폰이 생기면 적 AI는 그 폰을 시야로 적으로 감지하고, 적대 여부를 다시 보지 않는 `UWxBTService_UpdateTargetActor::FindPerceivedTarget`(`Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:60`)이 그대로 타겟으로 확정하지만 공격은 `IsHostile`에 막혀 피해 없이 추격만 반복한다. 반대로 Neutral 팀 AI는 플레이어를 감지·추격하되 때리지 못한다. 현재 `Content`에서 `EWxTeam::Neutral`을 쓰는 에셋은 검색되지 않아 잠재 결함이다.
- **제안**: `AWxAIController`에서 `GetTeamAttitudeTowards`를 재정의해 빙의 폰의 판정에 위임하거나, Neutral 규칙을 `FGenericTeamId::SetAttitudeSolver`로 등록해 캐릭터·컨트롤러·센스가 한 솔버를 공유하게 한다.
- **확신도**: 중간 (엔진 경로는 소스로 확인했으나 Neutral 팀 콘텐츠가 아직 없다)

### 2. 🟡 InventoryItem 뷰모델의 세 진입점이 같은 구독 코드를 복제하고, 그중 두 곳의 도착 구독은 동작할 수 없다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:12`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:23`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:34`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:100`
- **범주**: 중복/복잡도
- **문제**: `StartObserving`·`Initialize(Instance)`·`Initialize(ItemDef)`가 `Deinitialize` → 타깃 설정 → `ApplyStaticDataFromDef` → `OnAnyInventoryReady`/`OnAnyInventoryEnded` 구독 → `BindSource` 순서를 세 번 반복하며, 구독 두 줄(`:18`–`:19`, `:29`–`:30`, `:39`–`:40`)은 글자까지 같다. 게다가 두 `Initialize`는 첫 줄의 `Deinitialize()`가 `StopObserving()`(`:115` → `:95`)으로 `ObservedController`를 비운 뒤 다시 세우지 않는데, `HandleInventoryReady`(`:100`)는 `ObservedController.IsValid()`일 때만 연결하므로 이 두 경로의 도착 구독은 신호를 받기만 하고 아무것도 하지 않는다. `UWxViewModel_Inventory`가 슬롯마다(`WxViewModel_Inventory.cpp:151`), 획득마다(`WxViewModel_Inventory.cpp:115`) 이 경로로 자식을 만들기 때문에 무의미한 정적 델리게이트 바인딩이 그 수만큼 붙는다. 신호나 구독 조건이 바뀌면 세 곳을 함께 고쳐야 하고, 한 곳을 빠뜨려도 컴파일은 통과한다.
- **제안**: 같은 클래스의 private 헬퍼 하나로 "정적 데이터 적용 + 구독 + `BindSource`"를 모은다. `OnAnyInventoryReady` 구독은 `ObservedController`를 세우는 `StartObserving` 경로에만 두고, 자기 정리에 필요한 `OnAnyInventoryEnded` 구독은 공통 헬퍼에 남긴다.
- **확신도**: 높음

### 3. 🟢 처형 프롬프트와 처형 자격이 서로 다른 근거로 판정된다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:97`, `Source/WxGame/Character/WxEnemyCharacter.cpp:131`, `Source/WxGame/Character/WxEnemyCharacter.cpp:134`
- **범주**: 설계/구조
- **문제**: `GetInteractionPrompt`(`:134`)는 로컬 플레이어 폰의 ASC에서 `UWxAbility_Finisher` 스펙을 찾아 문구를 읽고 못 찾으면 빈 `FText`를 돌려주지만, `CanInteract`(`:97`)는 적대·생존·태그·후방 원뿔만 보고 상호작용자가 처형 어빌리티를 가졌는지는 보지 않는다. 처형 어빌리티를 부여받지 않은 폰(프론트엔드에서 고를 수 있는 다른 캐릭터 등)이 적의 뒤로 다가가면 스캐너 목록에 빈 줄이 뜨고, 그 항목으로 상호작용하면 서버 검증은 통과해 `Event.Finisher`(`:131`)를 보내지만 받을 어빌리티가 없어 아무 일도 일어나지 않는다.
- **제안**: `CanInteract`에서 `Interactor`의 ASC에 `UWxAbility_Finisher` 스펙이 있는지 함께 확인해, 프롬프트와 자격이 같은 근거를 공유하게 한다.
- **확신도**: 중간 (모든 플레이어 폰의 AbilitySet이 처형 어빌리티를 부여한다면 증상은 없다)

### 4. 🟢 MetaHuman 해제가 리더 메시에 건 설정을 절반만, 원래 값과 무관하게 되돌린다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:54`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 조립 시 리더 메시에 표시 끄기(`:54`)와 `AlwaysTickPoseAndRefreshBones`(`:56`)를 함께 건다. 해제(`:131`)는 표시만 무조건 `true`로 되돌리고 틱 옵션은 그대로 둔다. 그래서 해제 후 리더는 화면 밖에서도 포즈·본 갱신을 계속하고, 이 컴포넌트가 손대기 전부터 숨겨져 있던 리더는 해제 후 보이게 된다. 액터 파괴에서는 리더도 함께 사라지고 재등록에서는 두 설정이 다시 걸리므로 실제 창은 좁지만, 짝지어 건 설정의 비대칭은 남는다.
- **제안**: 해제에서 표시와 `VisibilityBasedAnimTickOption`을 함께 되돌리되, 캐시 필드를 새로 두기보다 리더 메시 아키타입(`LeaderMesh->GetArchetype()`)의 값을 읽어 복원한다.
- **확신도**: 높음

### 5. 🟢 `PostInitializeComponents`의 즉시 태그 확인 두 곳은 참이 될 수 없다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:60`, `Source/WxGame/Character/WxCharacterBase.cpp:61`, `Source/WxGame/Character/WxCharacterBase.cpp:71`
- **범주**: 중복/복잡도
- **문제**: 주석(`:60`)은 "late join 시 구독보다 먼저 초기 복제로 태그가 실려 왔을 수 있어"를 근거로 들지만, 두 역할 모두에서 그 상황이 생기지 않는다. 권위 측은 ASC가 막 생성된 상태이고, 적은 `Super::PostInitializeComponents()`(`:54`) 안의 자동 빙의(엔진 `Pawn.cpp:157`)로 어빌리티셋 부여까지 끝났을 뿐이라 `Ability.Death`·`State.Ragdoll`이 서 있을 수 없다. 원격 측은 복제 액터가 지연 생성 없이 스폰되어(엔진 `PackageMapClient.cpp:768`) 초기 프로퍼티·서브오브젝트 복제가 이 함수 뒤에 적용되고, 레벨 배치 액터도 로드 시점에 이 함수를 지나므로 태그는 항상 바로 위에서 등록한 콜백으로 들어온다. 결국 두 분기는 실행되지 않는 코드이고, 실제 안전망은 등록 콜백과 `AWxEnemyCharacter::BeginPlay`의 재확인(`WxEnemyCharacter.cpp:50`–`:55`)이다.
- **제안**: 두 즉시 확인을 지우고, 주석은 콜백 등록 위치의 이유만 남긴다.
- **확신도**: 중간 (Iris 미사용 기준의 엔진 스폰 순서에 근거한다)

### 6. 🟢 적 ASC의 GE 복제 모드 설정은 엔진 기본값을 다시 쓰는 무동작 호출이다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:45`
- **범주**: 중복/복잡도
- **문제**: `BeginPlay`에서 `SetReplicationMode(EGameplayEffectReplicationMode::Full)`을 부르지만 `UAbilitySystemComponent` 생성자가 이미 `Full`로 초기화하고(엔진 `AbilitySystemComponent.cpp:78`) `UWxAbilitySystemComponent`도 이를 바꾸지 않는다. 짝인 플레이어 설정(`WxPlayerCharacter.cpp:56`)은 생성자에 있어 같은 성격의 설정이 파생마다 다른 시점에 놓였고, 적은 자동 빙의로 어빌리티셋 부여가 `BeginPlay`보다 먼저 끝나므로 이 줄이 의미를 가진다 해도 부여 뒤에 도착한다.
- **제안**: 줄을 지운다. 의도를 남기려면 플레이어와 같이 생성자로 올린다.
- **확신도**: 높음

### 7. 🟢 InteractionList의 도착 신호 대기 경로는 타지 않고, 즉시 연결 경로는 `ObservedController`를 곧바로 비운다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:17`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:22`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:26`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:31`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:105`
- **범주**: 중복/복잡도
- **문제**: `StartObserving`은 `ObservedController = PC`(`:17`)를 세운 뒤 스캐너가 있으면 `Initialize(Scanner)`(`:22`)를 부르는데, `Initialize` 첫 줄의 `Deinitialize()`(`:31`)가 `StopObserving()`을 거쳐 그 필드를 다시 비운다. 한편 스캐너가 없을 때의 `OnAnyScannerReady` 대기(`:26`)는 같은 함수의 주석(`:19`)대로 스캐너가 `AWxPlayerController` 생성자 컴포넌트라 위젯보다 늦게 붙는 경우가 없어 실제로 타지 않으며, 생성자에 스캐너가 없는 PC라면 신호도 영영 오지 않는다. `HandleScannerReady`의 `StopObserving()`(`:105`)도 곧이어 `Initialize`가 같은 일을 반복한다. 현재 증상은 없지만 세운 값이 다음 호출로 지워지는 흐름과 타지 않는 대기 경로가 함께 있어, 재연결 경로를 넣으려는 사람이 이 필드를 믿으면 그대로 함정에 빠진다.
- **제안**: 주석의 전제를 계약으로 삼아 `StartObserving`을 "스캐너를 찾아 `Initialize`" 하나로 줄이고 `ObservedController`·`ScannerReadyHandle`·`HandleScannerReady`를 걷어낸다(이 경우 `UWxInteractionScannerComponent::OnAnyScannerReady`는 구독자가 사라진다). 대기 경로를 유지할 것이라면 `Initialize` 뒤에 `ObservedController`를 다시 세운다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp` 및 대응 헤더. 발견 검증을 위해 `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`와 엔진 `AIInterfaces.cpp`·`GenericTeamAgentInterface.h`·`AIPerceptionTypes.h/.cpp`·`AISense_Sight.cpp`·`Pawn.cpp`·`PackageMapClient.cpp`·`ActorReplication.cpp`·`MVVMViewClass.cpp`·`AbilitySystemComponent.cpp`도 읽었다.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.h`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/Tests/WxNameplateViewModelTest.cpp` 및 대응 헤더.
- **일괄 검사**: 소스 첫 줄 Copyright(58파일 + `WxGame.Build.cs` 전부 통과), `FORCEINLINE`·`inline`·헤더 본문 정의(0건), 람다(0건), 자체 타입의 `Wx` 접두사(전부 준수). `WxGame`은 게임 모듈이라 플러그인 참조 규칙의 대상이 아니다.
- **직전 리뷰 대비 정리**: 프론트엔드 위젯 하드코딩 지적은 `WxFrontEndWidget`이 제거되고 `FWxFrontEndOption`·`UWxFrontEndLibrary`로 바뀌어 해소됐다. Quest·Dialogue 주석 불일치는 주석이 "재주입 경로 없음"(`WxViewModel_Quest.cpp:82`)·"생성자 컴포넌트라 늦지 않음"(`WxViewModel_Dialogue.cpp:71`)으로 고쳐져 해소됐다. 플레이어 입력 콜백의 `Handle` 접두사 지적은 현행 `CLAUDE.md` 코딩 규칙에 해당 조항이 없어 뺐다. 처형 프롬프트 지적 중 "0번 플레이어 추측"은 로컬 플레이어 1명 전제로, "첫 처형 어빌리티 채택"은 앞잡·뒤잡을 한 어빌리티(`FinisherVariant`/`BackstabVariant`)가 처리하는 구조로 해소되어 발견 3의 불일치만 남겼다. 즉시 태그 확인 지적(발견 5)은 "이 시점엔 `OnDeath` 구독자가 없다"는 직전 근거가 틀려(권위 측 `AWxAIController`는 `Super` 안의 빙의에서 이미 구독한다) 근거를 엔진 스폰 순서로 바꿨다.
- **재검증 결과 제외한 항목**: 적이 스포너에 부착되어 이동 복제가 `AttachmentReplication`을 타는 건은 속도·스무딩 손실을 엔진 소스로 확인했으나 `WxSpawner.cpp:153`–`:155`가 정찰 경로 조회를 위해 알고 받아들인 대가로 명시해 제외했다. `UWxAbility_UseItem`의 조기 종료·중복 `EndAbility`가 `EndUseItem`을 부르는 건은 `WxItemUseComponent.cpp:29`의 일치 가드로 무해하다. `UWxRespawnLibrary`의 생성 실패 복귀가 `Possess(DeadPawn)`로 어빌리티셋을 다시 부여하는 건은 `WxAbilitySystemComponent.cpp:49`의 `bAbilitySetsGranted` 가드로 막힌다. `LastAcquiredItem` 교체 시 이전 인스턴스를 `Deinitialize`하지 않는 건은 아직 떠 있는 토스트의 표시가 비는 것을 막는 의도(`WxViewModel_Inventory.h:74`)로 보아 제외했다. `UWxViewModelResolver_InteractionList`·`UWxViewModelResolver_Ability`가 `ExpectedType`을 검사하지 않는 건은 엔진이 타입 불일치 인스턴스를 오류와 함께 거부하고(`MVVMViewClass.cpp:156`) 현 C++ 구조에 서브클래스 기대가 없어 제외했다. `UWxCheatManager`의 "존재 시점은 곧 권위" 전제는 클라가 `EnableCheats`로 만들 수 있지만 권위 없는 GE 적용을 ASC가 거부해 무동작이므로 제외했다. `AWxEnemyCharacter::HandleOwnerDeath`의 0번 플레이어 보상은 코드 주석이 밝힌 확정 정책이라 다루지 않았다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE·네트워크 실행은 하지 않았다. WBP/BP 내부와 BP 디폴트 값(플레이어 폰별 AbilitySet 구성, `UWxAbility_Interact::ScanRadius`와 스캐너 반경의 일치, `Team` 설정 등)은 범위 밖이라 발견 1·3의 실제 발현 여부는 콘텐츠 확인이 필요하다. `WxMetaHumanComponent`의 LOD 매핑 산식과 엔진 `UMetaHumanComponentUE` 내부 동작은 에디터 실행 없이는 검증하지 못했다. 멀티플레이 경로는 게임 진입·부활이 스탠드얼론 전용인 현재 구조에 맞춰 권위·복제 지적을 단정하지 않았다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 58파일 — `/module-review`로 갱신*
