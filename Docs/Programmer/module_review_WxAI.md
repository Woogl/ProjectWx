# WxAI — 코드 리뷰

> 기존 노드는 건강하다. 플러그인 의존은 `WxCore` 하나뿐이고(`Build.cs`·`.uplugin`·include), 소스 40개 모두 Copyright 첫 줄과 `Wx` 접두사를 지키며 인라인 정의가 없다. 이번에 들어온 Master 미러 계열 4개 노드는 헤드리스 테스트가 덮지 못한 조건 셋에서 어긋난다: 플레이어 락온, 활성 어빌리티 순서, 원격 클라이언트 Master. 커버리지: README → `Build.cs`/`.uplugin` → 헤더 20개·cpp 20개를 모두 읽었고, 신규·재작성 노드는 UE 5.8 BT·GAS 엔진 소스와 WxCombat 호출 측을 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 MirrorMovement 가 Master 의 모든 활성 어빌리티를 "행동 중"으로 봐서, 락온하는 동안 1초 텔레포트와 종료 복귀가 둘 다 멈춘다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:15`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:145`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:146`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:156`
- **범주**: 버그/정확성
- **문제**: `HasActiveMirrorMovementAbility`(:15~25)는 스펙 종류를 가리지 않고 `IsActive()` 인 스펙이 하나라도 있으면 true 를 돌려준다.
  - 플레이어 락온 어빌리티는 토글형이다. 입력을 다시 누를 때 끝나고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:28~33`), 서버 인스턴스는 `:55` 에서 반환한 뒤에도 활성으로 남는다. 이 어빌리티는 `ABS_Shared_Player` 가 `GA_Shared_LockOn` 으로 부여하는데, `BT_Doppelganger` 매핑에는 없다. 복제 대상이 아닌데도 판정에는 걸린다.
  - 락온하는 동안 `bAbilityActive`(:145)가 계속 true 다. 그래서 `TravelTime` 이 매 틱 0 으로 돌아가고(:156), 벽에 막힌 분신이 락온을 풀 때까지 텔레포트하지 않는다.
  - 공격이 끝날 때 잡힌 종료 복귀 예약(:81)도 `:146` 에서 계속 보류된다. 그러다 한참 뒤 락온을 푸는 순간 분신이 갑자기 순간이동한다.
  - 작업 기록은 가드·질주를 일부러 포함했다고 적었다(`.codex/worklog/2026-09-17-doppelganger-action-guards.md`). 락온은 복제하는 행동이 아니다. 궁극기로 분신을 부르는 전투 상황에서는 락온이 켜져 있는 경우가 흔하다.
- **제안**: 판정할 어빌리티를 복제 대상 행동으로 좁힌다. 예를 들어 `FGameplayTagRequirements` 저작 필드를 두고 `Spec.Ability->GetAssetTags()` 를 거른다(IgnoreTags 에 `Ability.LockOn`). ASC 는 이미 `MasterAbilitySystem`/`FollowerAbilitySystem` 에 캐시돼 있으니 헬퍼가 매 틱 다시 찾지 않게 한다.
- **확신도**: 높음. 락온의 수명과 매핑 누락은 소스와 에셋 문자열로 확인했다. 작업 기록의 테스트 시나리오에는 락온이 없다.

### 2. 🟡 ObserveMasterAbility 의 소환 시 따라잡기는 뒤에 오는 활성 스펙이 `PendingAbility` 를 덮어써, 성패가 어빌리티 부여 순서에 달린다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:95`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:121`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:131`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_MasterAbility.cpp:19`
- **범주**: 버그/정확성
- **문제**: `BindMaster` 는 활성 스펙마다 `HandleAbilityActivated` 를 부른다(:95~101).
  - `HandleAbilityActivated` 는 반응 분기가 있는지 보기 전에 `PendingAbility` 를 먼저 쓴다(:121). 요청을 낸 뒤(:131)에는 자기만 반환하므로, 호출한 루프는 다음 스펙으로 계속 간다.
  - 재선택 탐색은 다음 BT 틱에 돈다(엔진 `BehaviorTreeComponent.cpp:1144~1150`). 이때 데코레이터(:19)가 보는 값은 목록에서 마지막으로 활성인 스펙의 태그다. 이 프로젝트 어빌리티는 모두 생성자에서 `Ability.*` 에셋 태그를 단다(예: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:22`). 그래서 다른 활성 스펙이 하나만 뒤에 있어도 덮어쓰기가 일어난다.
  - 예: 락온한 플레이어가 Skill 1 을 쓰는 도중 미니언이 소환된다. `GA_Shared_LockOn` 이 목록에서 Skill 1 뒤에 있으면 최종값은 `Ability.LockOn` 이 되고, 요구된 "진행 중 스킬 따라잡기"(`.codex/worklog/2026-09-16-minion-master-bt-reaction.md`)가 조용히 빠진다. 목록 순서는 `AbilitySets` 배열의 부여 순서다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:56~61`).
  - 실시간 경로의 "최신 발동이 이긴다"는 설계된 정책이다. 따라잡기 루프의 목록 순서는 시간 순서가 아니므로 이 정책의 근거가 되지 못한다.
- **제안**: 따라잡기 루프는 요청이 한 번 나가면 멈춘다. 예를 들어 `HandleAbilityActivated` 가 요청 여부를 bool 로 돌려주게 하고, 호출 루프는 그 값이 true 면 빠진다.
- **확신도**: 중간. 코드 경로는 확실하다. 다만 `BP_HGTest` 의 `AbilitySets` 순서(`ABS_HGTest` 대 `ABS_Shared_Player`)는 바이너리라 확인하지 못해, 지금 에셋에서 실제로 실패하는지는 모른다.

### 3. 🟡 MirrorAbility 가 서버에서 Master 의 `GetLastMovementInputVector` 를 읽는데, 원격 클라이언트 Master 면 이 값이 0 이라 분신 회피가 항상 뒤로 나간다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:174`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:41`
- **범주**: 버그/정확성
- **문제**: 태스크는 서버에서만 돈다(:41). 회피를 복제할 때는 Master 의 마지막 입력 벡터를 분신 입력으로 옮긴다(:173~175).
  - 이 값은 `AddMovementInput` 을 부른 머신에서만 채워진다. 그래서 서버의 원격 폰은 항상 0 이다. 프로젝트도 같은 이유로 가속도를 쓰도록 이미 정해 두었다(`Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingSorterTask_InputDirection.cpp:23`, `:30`).
  - 서버 AI 는 로컬 제어라 분신의 회피는 자기 `GetLastMovementInputVector` 를 읽는다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:46`, `:51`). 받은 값이 0 벡터면 `Back` 으로 해석된다(`:110`).
  - Master 자신의 서버 인스턴스는 복제된 방향 데이터를 받아 회피하므로(`:77`) 옆으로 구르는데, 분신만 뒤로 구르는 불일치가 생긴다. 리슨 서버 호스트와 스탠드얼론에서만 맞는다.
  - 작업 기록도 멀티플레이 연결은 검증하지 않았다고 적었다(`.codex/worklog/2026-09-16-doppelganger-bt.md`).
- **제안**: 방향 원천을 `Cast<UCharacterMovementComponent>(MasterPawn->GetMovementComponent())->GetCurrentAcceleration()` 로 바꾼다. 서버가 원격 클라이언트 입력으로 채워 두는 엔진 값이라 플러그인 경계와도 무관하다.
- **확신도**: 높음. 엔진 동작은 프로젝트 주석과 회피 코드로 확인했다. 실제 네트워크 실행은 하지 않았다.

### 4. 🟡 RandomChoice 가 가중치를 보기 전에 막힌 자식에 활성화 실패를 알려, 조건이 풀리면 추첨 없이 그 자식이 끼어든다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:63`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:67`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:89`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h:29`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`
- **범주**: 버그/정확성
- **문제**: 후보 수집 루프는 조건 평가(:63)에 걸린 자식마다 `NotifyDecoratorsOnFailedActivation`(:67)을 부른다. 가중치 0 제외(:89)는 그 뒤에야 본다.
  - 엔진은 이 알림에서 LowerPriority·Both 데코레이터를 관찰자 Add 로 쌓는다(엔진 `BTCompositeNode.cpp:307~311`). 적용할 때는 새 태스크보다 실행 인덱스가 큰 Add 를 건너뛴다(`BehaviorTreeComponent.cpp:1498~1499`). 결국 뽑힌 자식보다 앞 인덱스의 막힌 자식만 관찰자가 되고, RandomChoice 가 실패하면 막힌 자식 전부가 관찰자가 된다.
  - 그 조건이 참이 되면 `UBTCompositeNode::GetNextChild` 는 `GetNextChildHandler` 를 거치지 않는다. `GetMatchingChildIndex` 로 그 자식을 바로 고른다(`BTCompositeNode.cpp:598~602`). 이때 가중치·`bAvoidRepeat` 가 무시되고 `LastChosenChild` 도 갱신되지 않는다. 가중치 0 자식도 이렇게 실행되므로 "0 이면 추첨에서 제외된다"(`Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomWeight.h:31`)는 약속이 깨진다.
  - 예: [0] 근접(거리 조건 aborts Lower Priority, Weight 0), [1] 원거리 순서라면, 원거리 공격 중 대상이 붙을 때 근접이 끼어든다. 같은 자식을 [2] 에 두면 끼어들지 않으므로, 자식 순서가 숨은 우선순위가 된다.
  - 헤더(:29~30)에는 "인덱스와 무관하게 관찰자로 등록되고 뒤집히면 재추첨된다"고 적혀 있어 실제 동작과 다르다.
- **제안**: 가중치를 조건 평가보다 먼저 조회하고, 가중치 0 자식은 알림 없이 건너뛴다. 인덱스에 따른 비대칭이 의도가 아니면 방식도 바꾼다. 후보를 뽑았으면 알리지 않고, 후보가 없어 실패할 때만 막힌 자식 전부에 알린다. 어느 쪽이든 헤더 :29~30 을 실제 동작에 맞춘다.
- **확신도**: 높음. 엔진 경로는 소스로 확인했고, `BT_Soldier`·`BT_Template` 가 두 노드를 쓴다. 자식 데코레이터의 aborts 설정과 가중치 값은 보지 못했다.

### 5. 🟢 데코레이터 4종의 `GetStaticDescription` 이 `Super` 를 부르지 않아 그래프에서 aborts·inversed 표기가 사라진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:46`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:18`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp:13`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_MasterAbility.cpp:31`
- **범주**: 버그/정확성
- **문제**: 엔진의 `UBTDecorator::GetStaticDescription` 이 "( aborts lower priority, inversed )" 줄과 타입명을 만든다(엔진 `BTDecorator.cpp:122~145`). 네 데코레이터는 이 함수를 덮어쓰면서 `Super` 를 부르지 않는다.
  - BeyondLeash 는 aborts 가 None 이면 폴링이 꺼지는데(`Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:24`), 그래프에서는 그 여부가 보이지 않는다.
  - 반전된 RandomWeight·MasterAbility 는 자식을 막거나 뜻이 뒤집히는데도, 그래프에는 "Weight = 1.00"·"Master가 … 발동 시" 로만 보인다.
  - 서비스는 같은 이유로 `GetStaticServiceDescription` 만 덮어쓴다고 명시했다(`Plugins/WxAI/Source/WxAI/Public/WxBTService_LockOn.h:44`). 데코레이터에는 이 원칙이 적용되지 않았다.
- **제안**: 엔진 데코레이터처럼 `Super::GetStaticDescription()` 결과 뒤에 자체 설명을 붙인다.
- **확신도**: 높음

### 6. 🟢 ObserveMasterAbility 는 루트 직계 자식의 데코레이터만 살펴서, 한 단계 아래에 둔 MasterAbility 분기는 재선택이 요청되지 않고 나중에 늦게 반응한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:123`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:156`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_MasterAbility.h:9`
- **범주**: 설계/구조
- **문제**: 요청 판정은 `Parent->Children` 의 데코레이터만 본다(:123~134). 반면 `Find` 는 깊이와 상관없이 루트까지 올라가 서비스를 찾는다(:156~159). 그래서 하위 Composite 안에 둔 MasterAbility 도 평가 자체는 정상으로 통과한다.
  - 그런 분기에는 재선택 요청이 나가지 않는다. `PendingAbility` 도 소비되지 않고 다음 발동(:121)까지 남는다. 결과적으로 현재 태스크(대기 등)가 끝나 자연 재탐색이 돌 때, 이미 끝난 스킬에 뒤늦게 반응한다.
  - 두 헤더 모두 "루트 직계 자식에만 둔다"는 제약을 적지 않았다. 현재 `BT_Minion` 은 평평한 구조라 드러나지 않는다(`.codex/worklog/2026-09-16-minion-bt-simplify.md`).
- **제안**: 데코레이터·서비스 헤더에 배치 제약을 적는다. 중첩 배치를 허용하려면 요청 판정을 `ChildComposite` 까지 재귀로 넓힌다.
- **확신도**: 높음(코드 동작). 현재 에셋에는 영향이 없다.

### 7. 🟢 UpdateTargetActor 의 "세 센스 모두 적대만 등록" 전제가 청각에서 성립하지 않는다 — 시야·촉각은 폰 규칙, 청각은 기본 팀 솔버를 쓴다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTService_UpdateTargetActor.h:29`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:60`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp:23`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp:161`
- **범주**: 버그/정확성
- **문제**: `FindPerceivedTarget` 은 피아를 다시 가르지 않고, 조건에 맞는 첫 감지 액터를 타겟으로 문다(:60). 근거는 헤더(:29)의 전제다.
  - 시야는 `AWxAIController::GetTeamAttitudeTowards` 가 폰에 맡기고(`Source/WxGame/Controller/WxAIController.cpp:49~56`), 촉각은 이 모듈의 필터(`Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp:161`)가 거른다. 둘 다 폰 규칙을 따르고, 폰 규칙은 한쪽이 Neutral 이면 Neutral 이다(`Source/WxGame/Character/WxCharacterBase.cpp:144~147`).
  - 청각은 팀 ID 를 기본 솔버로 비교한다(엔진 `AISense_Hearing.cpp:157`, `AIPerceptionTypes.h:231~235`). 기본 솔버는 ID 가 다르기만 하면 Hostile 이다(`AIInterfaces.cpp:30~33`). 프로젝트에 `SetAttitudeSolver` 등록은 없다.
  - `EWxTeam::Neutral` 은 255 로 `NoTeam` 과 같다(`Source/WxGame/Character/WxTeamTypes.h:13`). 팀 인터페이스가 없는 소음원도 `NoTeam` 이 된다(`AISense_Hearing.cpp:29`, `AIInterfaces.cpp:14~23`). 그래서 Neutral 캐릭터나 `AWxNpc` 가 소음 노티파이가 박힌 공용 조깅 애니 8종으로 지나가면 적대 자극이 되고, 이 서비스가 그 액터를 타겟으로 확정한다.
  - 지금은 Neutral 팀 에셋이 없고 `AWxNpc` 는 제자리에만 있어서 드러나지 않는다.
- **제안**: 피아 판정 출처를 하나로 모은다. WxGame 에서 `FGenericTeamId::SetAttitudeSolver` 로 폰 규칙과 같은 솔버를 등록하면 청각까지 맞는다. 모듈 안에서 막으려면 `FindPerceivedTarget` 에 `FGenericTeamId::GetAttitude(SelfActor, PerceivedActor) == ETeamAttitude::Hostile` 검사를 넣는다. 어느 쪽이든 헤더 :29 를 고친다.
- **확신도**: 중간. 엔진 경로는 확인했다. Neutral 팀이나 비팀 소음원을 실제로 쓸지는 기획에 달렸고, 컨트롤러 헤더(`Source/WxGame/Controller/WxAIController.h:27`)가 청각의 솔버 경로를 이미 언급하고 있어 알고 둔 불일치일 수도 있다.

## 검토 범위
- **깊게 본 파일**
  - WxAI: `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_MasterAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`와 각 헤더
  - 계약 확인용 외부 코드: `Source/WxGame/Controller/WxAIController.cpp`·`.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Passive.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingSorterTask_InputDirection.cpp`
  - 대조한 UE 5.8 엔진 소스: `BehaviorTreeComponent.cpp`, `BehaviorTreeManager.cpp`, `BTNode.cpp`, `BTAuxiliaryNode.cpp`, `BTCompositeNode.cpp`, `BTDecorator.cpp`, `BTTaskNode.h`, `AIController.cpp`, `AISense_Hearing.cpp`, `AIInterfaces.cpp`, `AIPerceptionTypes.h`, `GameplayAbility.cpp`, `AbilitySystemComponent_Abilities.cpp`, `GameplayAbilityTypes.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`와 대응 헤더, `.codex/worklog/` 의 도플갱어·미니언 작업 기록 7건
- **미검토 / 한계**
  - 실행 확인: 빌드·PIE·네트워크 실행 없이 코드만 읽었다.
  - 에셋 확인 범위: 노드 클래스명과 에셋명만 문자열로 확인했다.
    - `ObserveMasterAbility`·`MasterAbility` 는 `BT_Minion` 이 쓰고, `MirrorAbility`·`MirrorMovement` 는 `BT_Doppelganger` 가 쓴다. `BT_Doppelganger` 의 원본 매핑에는 `GA_Shared_LockOn` 이 없다.
    - `AttributeRatio` 는 어느 에셋에도 없다.
    - 노드 속성값(aborts·가중치)과 `BP_HGTest` 의 `AbilitySets` 순서는 확인하지 못했다.
  - 직전 리뷰(`7d1d0374`) 이후 달라진 점
    - 직전 2번은 해소돼 뺐다. `Trail` 을 BT 노드 메모리에 두던 문제인데, 재작성된 `UWxBTService_MirrorMovement` 는 `bCreateNodeInstance = true`(`Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:31`)이고 상태를 인스턴스 멤버(`Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h:37~50`)에 둔다.
    - 직전 1·3·4번은 코드가 그대로라 유지하고 라인을 갱신했다. 3번에는 신규 `MasterAbility` 를 추가했다.
    - 1·2·3·6번은 이번에 새로 세운 발견이다.
  - 새로 세웠다가 기각한 가설
    - 서비스가 자기 실행 인덱스로 재선택을 요청하면 `UnregisterAuxNodesUpTo` 가 루트 서비스 자신을 제거한다(엔진 `BehaviorTreeComponent.cpp:2059~2061`) → 기각. Looped 실행에서는 루트 서비스의 Remove 를 건너뛰고(`:1517~1528`), `RunBehaviorTree` 가 Looped 로 시작한다(`AIController.cpp:1039`).
    - `RunBehaviorTree` 뒤에 Master 를 채워 첫 따라잡기를 놓친다 → 기각. 첫 탐색은 다음 틱으로 미뤄지므로(`BehaviorTreeComponent.cpp:1144~1150`), 서비스가 활성화될 때는 이미 Master 가 채워져 있다.
    - `MirrorAbility` 의 따라잡기가 `GetPrimaryInstance` 를 써서 실행마다 새로 만드는 인스턴스를 놓친다 → 기각. 프로젝트 기본값이 `InstancedPerActor` 다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:18`).
    - `HandleEnded` 의 `GrantedHandles[Index]` 가 범위를 벗어난다 → 기각. 구독은 매핑마다 부여를 마친 뒤에만 걸리고(`Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp:44~47`), `CleanUp` 이 구독을 먼저 끊은 뒤 배열을 비운다(:206, :215).
    - 이벤트 문맥이 활성 항목보다 먼저 도착한다 → 기각. 트리거로 발동한 어빌리티가 먼저 실행되고 그 뒤에 `GenericGameplayEventCallbacks` 가 방송된다(엔진 `AbilitySystemComponent_Abilities.cpp:2577`, `:2587`).
  - 판정을 유지한 기각 항목: 코드가 그대로라 직전 판정을 유지한다.
    - 스티키 타겟, 가중치 전원 0 승계, Patrol 인스턴스 재빙의, BeyondLeash 메모리 초기화, ReturnHome 실패 후 BeyondLeash 시드, LockOn 적용 기록 롤백, 완주 후 Patrol abort, `ForgetActor(nullptr)`, 재빙의 `ApplySenseSettings`
    - 옛 Mirror 노드에 대한 기각 항목은 코드가 재작성돼 폐기했다.
  - 발견으로 올리지 않은 것
    - 익명 namespace 헬퍼 두 곳(`Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp:13`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:12`): 코드 작성 선호와 어긋나지만 CLAUDE.md 규칙이 아니다.
    - 베이스보다 넓힌 접근 지정자 세 곳(`Plugins/WxAI/Source/WxAI/Public/WxBTTask_MirrorAbility.h:58` `OnTaskFinished`, `Plugins/WxAI/Source/WxAI/Public/WxBTService_MirrorMovement.h:22`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_MasterAbility.h:16`): 이 중 마지막은 서비스가 직접 호출하려고 넓힌 것이다.
    - `MirrorMovement` 의 매 틱 `MaxWalkSpeed` 기록(:134): 분신이 Master 속도를 따르게 하려는 의도다. 스냅숏 복원(:64)은 BT 가 멈출 때만 돌아 영향이 없다.
    - 실시간 발동에서 반응 없는 발동이 대기 태그를 덮어쓰는 것: 작업 기록이 정한 "최신 한 건" 정책이다.
    - README 핵심 타입 표의 설명: `UWxBTService_ObserveMasterAbility` 를 "분신(미러) 계열 진입점"으로 적었지만, 실제 사용처는 `BT_Minion` 이다. 분신은 `UWxBTTask_MirrorAbility` 가 직접 구독한다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 40파일 — `/module-review`로 갱신*
