# WxAI — 코드 리뷰

> 규칙 준수와 모듈 경계는 여전히 흠잡을 데가 없다 — 플러그인 의존은 `WxCore`뿐이고, 29파일 전체에서 Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix 누락이나 `BlueprintCallable`/`FORCEINLINE`/불필요한 람다가 한 건도 없다. 직전 리뷰의 최상위 발견(억제 해제 후 재획득 불가)은 `SetTargetingSuppressed` 에 레벨 기반 재획득 루프가 들어가 해소됐고, 남은 위험은 "엔진이 대신 해 주는 줄 알았던 것"에 몰려 있다: Damage 센스의 무필터 자극, GAS 재발동 경로, BT Composite 의 abort 관찰자 등록, MoveTo 파생 태스크의 키 이원화가 그것이다. 이번 리뷰는 소스 29개를 모두 읽고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite 의 cpp 를 UE 5.8 엔진 소스(`AbilitySystemComponent_Abilities.cpp`·`BTCompositeNode.cpp`·`BehaviorTreeComponent.cpp`·`AIPerceptionComponent.cpp/.h`·`AISenseConfig_Damage.h`·`BTTask_BlackboardBase.h`·`BlackboardKeyType_Vector.cpp`)와 직접 대조해 각 발견의 메커니즘을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 Damage 센스에는 진영 필터가 없는데 타겟 채택에도 적대 판정이 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:34`, `:77-79`, `:140-148`
- **범주**: 버그/정확성
- **문제**: Sight(`:19-21`)와 Hearing(`:29-31`)은 `DetectionByAffiliation` 으로 적만 감지하도록 막아 두었지만, `UAISenseConfig_Damage` 에는 애초에 `DetectionByAffiliation` 필드 자체가 없어(엔진 `AISenseConfig_Damage.h` 는 `Implementation` 하나뿐) **가해자가 누구든** 자극이 들어온다. 그리고 `HandleTargetPerceptionUpdated` 는 성공 자극이면 사망 여부만 보고 무조건 `SetTargetActor(Actor)` 를 부른다. `Source/WxGame/Character/WxEnemyCharacter.cpp:71` 이 `Context.GetInstigator()` 를 그대로 실어 `UAISense_Damage::ReportDamageEvent` 를 호출하므로, 아군 광역기·환경 데미지·도트의 Instigator 가 그대로 적의 타겟이 된다(같은 진영 NPC 를 공격하는 형태). 억제 해제 시의 재획득 루프(`:140-148`)도 같은 구멍을 공유한다 — `HasAnyCurrentStimulus()` 만 보고 적대 여부를 묻지 않으며, 게다가 `FActorPerceptionContainer`(TMap) 순회 순서로 "첫 항목"을 집으므로 어느 액터가 뽑힐지 결정적이지 않다.
- **제안**: 엔진이 자극 등록 때마다 `PerceptualInfo->bIsHostile = (FGenericTeamId::GetAttitude(GetOwner(), SourceActor) == ETeamAttitude::Hostile)` 을 이미 계산해 둔다(`AIPerceptionComponent.cpp:532`). 채택 경로에서 그 값을 게이트로 쓰면 된다 — 자극 콜백은 `GetActorInfo(*Actor)->bIsHostile`, 재획득 루프는 `It->Value.bIsHostile` 한 줄이면 규약을 벗어나지 않고 막힌다.
- **확신도**: 중간(아군 오사가 실제로 발생하는 설계인지에 달렸다)

### 2. 🟡 ActivateAbility: 재발동 경로에서 태스크가 어빌리티보다 먼저 끝난다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:49-63`, `:66-69`, 콜백 `:110-137`
- **범주**: 버그/정확성
- **문제**: `ActivatedHandle` 을 `TryActivateAbility` **호출 전에**(`:54`) 세워 두고, `HandleAbilityEnded` 는 스펙 핸들만으로 "내 실행의 종료"를 판별한다. 그런데 엔진은 `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 이미 활성인 채로 재발동되면 **기존 인스턴스를 `EndAbility(bWasCancelled=false)` 로 먼저 끝낸 뒤** 재활성화한다(`AbilitySystemComponent_Abilities.cpp:1831-1852`, 5.8.1 확인). 이때 같은 핸들로 콜백이 들어와 `CleanUp()` 이 구독을 끊고 `ActivatedHandle` 을 비우며 `ActivationResult = Succeeded` 가 채워진다. 이어 재활성화가 성공하면 `:66-69` 가 그 낡은 결과를 그대로 반환해 태스크를 즉시 종료한다 — 어빌리티는 계속 도는데 BT 는 다음 행동으로 넘어가고(공격 모션 위에 이동·배회가 겹침), `ActivatedHandle` 도 비어 있어 `AbortTask` 가 취소할 수도 없다.
- **제안**: `bIsActivating` 구간의 콜백은 결과만 기록하고 `CleanUp()`(구독 해제)은 `ExecuteTask` 반환 직전으로 미룬다. 또는 `:66-69` 의 `ActivationResult` 신뢰를 `:79-84` 의 "핸들 유효 + 스펙 활성" 확인 뒤로 미뤄, 실제로 도는 어빌리티가 있으면 InProgress 를 유지하게 한다.
- **확신도**: 중간(엔진 경로는 확인했고, `bRetriggerInstancedAbility` 는 기본 false 라 디자이너가 켠 어빌리티에서만 발현한다. 참고로 직전 리뷰가 함께 적었던 "구독 유실로 BT 영구 정지" 변종은 엔진 코드 대조 결과 도달 불가라 이번에 뺐다 — 콜백이 뜬 시점엔 `ActivationResult` 가 이미 채워져 `:66` 에서 조기 반환된다)

### 3. 🟡 복귀 이동이 실패해도 타겟은 이미 비워진 뒤다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:29-35`, `:38-48`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask` 가 `Super::ExecuteTask`(MoveTo)의 성패를 알기 **전에** `SetTargetingSuppressed(true)` 를 부른다. HomeLocation 이 내비메시 밖이거나 경로가 없어 MoveTo 가 `Failed` 를 반환하면, 그 헛발질 한 번이 이미 `SetTargetActor(nullptr)` → 포커스 해제 → CMC 회전 모드 원복 → `State.InCombat` 제거까지 끝낸 뒤다. 엔진은 동기 결과에도 `OnTaskFinished` 를 부르므로(`BehaviorTreeComponent.cpp:2506-2514`) 곧바로 억제가 풀리고 재획득 루프가 돌지만, 그 루프는 **지금 감지 중인** 액터만 집는다. 이 모듈의 설계 전제는 "한 번 확보한 타겟은 시야를 잠시 잃어도 유지한다"(`WxAIPerceptionComponent.h:27`)이므로, 벽 뒤·등 뒤의 타겟은 이 한 번의 실패로 영구히 사라진다. 감지 중이어서 되살아나는 경우에도 재탐색마다 포커스·회전 모드·복제 태그(`State.InCombat`)가 껐다 켜져 네임플레이트가 깜빡이고 불필요한 복제 트래픽이 난다.
- **제안**: `Super::ExecuteTask` 결과가 `InProgress` 일 때만 억제를 켠다(다른 결과에서는 억제를 아예 건드리지 않는다).
- **확신도**: 높음(메커니즘) / 중간(HomeLocation 경로 실패 빈도에 달림)

### 4. 🟡 Patrol 의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:46-49`, `:63`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask` 는 하드코딩된 `WxBlackboardKeys::PatrolTargetLocation` 에 목표를 쓰지만, 실제 이동은 `Super::ExecuteTask` 가 `UBTTask_BlackboardBase::BlackboardKey`(엔진에서 `UPROPERTY(EditAnywhere)`, `BTTask_BlackboardBase.h:30`)를 읽어 수행한다. 생성자 `:19` 는 기본값을 맞춰 둔 것뿐이라, 디자이너가 BT 에디터에서 키를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation` 에 쓰고 엉뚱한 키(대개 미설정 = `FAISystem::InvalidLocation`)로 이동을 시도한다. 경고 하나 없이 정찰이 실패하거나 폰이 엉뚱한 곳으로 걸어가는 형태로만 드러난다.
- **제안**: 쓰기도 `BlackboardKey` 를 경유해 읽는 키와 통일하거나, 반대로 `BlackboardKey` 편집을 잠근다. `UWxBTTask_ReturnHome` 은 읽기만 하므로 동일 위험이 없다.
- **확신도**: 높음(메커니즘) / 중간(실제 에셋 설정에 달림)

### 5. 🟡 AttributeRatio 에는 재평가 트리거가 없어 FlowAbortMode 설정이 무효다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-13`, `:35-79`
- **범주**: 설계/구조
- **문제**: 이 데코레이터는 `CalculateRawConditionValue` 만 구현하고, 값 변화를 관찰할 장치(어트리뷰트 변경 델리게이트 구독이나 `TickNode` 폴링)가 없다. 엔진에서 관찰자 abort 는 노드가 스스로 `RequestExecution` 을 부를 때만 일어나므로, 디자이너가 디테일 패널에서 FlowAbortMode 를 지정해도 **아무 일도 일어나지 않는다** — "HP 가 30% 아래로 떨어지면 즉시 광폭화 브랜치로 전환" 같은 의도가 조용히 무시되고, 다음 재탐색까지 미뤄진다. 같은 모듈의 `UWxBTDecorator_BeyondLeash` 는 정확히 이 문제 때문에 `TickNode` 폴링을 두었다(`WxBTDecorator_BeyondLeash.cpp:49-61`).
- **제안**: BeyondLeash 와 같은 폴링을 넣거나(`INIT_DECORATOR_NODE_NOTIFY_FLAGS` + `TickNode` 에서 값 전이 시 `RequestExecution`), ASC 의 `GetGameplayAttributeValueChangeDelegate` 를 구독한다. 지원하지 않기로 한다면 생성자에서 `bAllowAbortLowerPri = false; bAllowAbortChildNodes = false;` 로 드롭다운을 잠가 오해를 없앤다.
- **확신도**: 중간

### 6. 🟡 RandomChoice 의 사전 필터가 형제 데코레이터의 abort 관찰자 등록을 통째로 건너뛴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:57-84`
- **범주**: 설계/구조
- **문제**: 후보 수집 루프가 `DoDecoratorsAllowExecution` 으로 조건 실패 자식을 **직접** 걸러내고 통과한 자식 인덱스를 바로 반환한다. 그런데 엔진에서 "조건 실패 자식"의 처리는 `UBTCompositeNode::FindChildToExecute` 의 else 분기가 담당하며(`BTCompositeNode.cpp:35-68`), 거기서 부르는 `NotifyDecoratorsOnFailedActivation` 이 `FlowAbortMode == LowerPriority | Both` 인 데코레이터를 aux 노드로 등록한다(`:297-315`). 사전 필터로 걸러진 자식은 이 경로를 밟지 않으므로, RandomChoice **안에 있는** 형제의 LowerPriority 데코레이터(`UWxBTDecorator_BeyondLeash` 나 엔진 Blackboard 관찰자 등)는 관찰자로 등록되지 않고 실행 중인 자식을 선점하지 못한다. 같은 이유로 조건 데코레이터는 선택된 자식에 대해 두 번 평가되고, 걸러진 자식의 `WrappedOnNodeProcessed` 는 호출되지 않는다.
- **제안**: RandomChoice 하위에 관찰자 abort 데코레이터를 두지 않는다는 제약을 클래스 주석과 `GetStaticDescription` 에 명시하거나, 필터에서 걸러낸 자식마다 `NotifyDecoratorsOnFailedActivation` 을 직접 호출해 엔진 규약을 복원한다.
- **확신도**: 중간(공격 패턴 추첨 용도로만 쓴다면 무해하지만, 규약 이탈이라 확장 시 조용히 깨진다)

### 7. 🟡 Once 정찰 완료 후 매 BT 틱 즉시 Succeeded 를 반환해 전체 재탐색이 상시화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:41-44`
- **범주**: 성능/안전
- **문제**: `bPatrolFinished` 가 서면 `ExecuteTask` 가 이동도 지연도 없이 동기 `Succeeded` 를 반환한다. 엔진은 즉시 끝난 태스크에 대해 곧바로 `OnTaskFinished` → 실행 갱신을 예약하므로(`BehaviorTreeComponent.cpp:2506-2514`), 이 폰은 살아 있는 내내 **BT 틱마다 트리 검색**을 반복한다(형제 데코레이터 평가 비용이 전부 따라붙는다). 프레임 내 무한 루프는 아니지만 Once 정찰 적이 많은 맵에서는 그대로 누적된다.
- **제안**: 완료 상태에서는 `InProgress` 를 반환해 실제로 브랜치를 점유하게 한다. 주석이 말하는 "그 자리에 머문다"의 정확한 표현이며, 상위 우선순위 abort 는 그대로 동작한다.
- **확신도**: 중간

### 8. 🟢 BeyondLeash 의 "Self/Both 금지"가 주석으로만 있고 에디터에서 막히지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:11-19`, 경고 주석 `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:21-22`
- **범주**: 설계/구조
- **문제**: 헤더는 Self/Both 를 고르면 "복귀가 경계에서 끊기고 경계 왕복이 난다"고 명시하는데, 생성자는 `FlowAbortMode` 에 기본값만 넣을 뿐 잘못된 선택지를 막지 않는다. 엔진은 바로 이 목적의 플래그(`bAllowAbortNone`·`bAllowAbortLowerPri`·`bAllowAbortChildNodes`)로 디테일 패널 드롭다운을 제한할 수 있는데 쓰지 않았다. 잘못 고르면 재현·진단이 어려운 왕복 버그로만 드러난다.
- **제안**: 생성자에서 `bAllowAbortNone = false; bAllowAbortChildNodes = false;` 를 세워 LowerPriority 외 선택 자체를 없앤다.
- **확신도**: 중간(의도적으로 여지를 남긴 것일 수 있음)

### 9. 🟢 BeyondLeash 가 HomeLocation 미설정을 "이탈"로 오판한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:36-37`
- **범주**: 버그/정확성
- **문제**: Blackboard Vector 키의 미설정 값은 `FAISystem::InvalidLocation`(`BlackboardKeyType_Vector.cpp:8`, `:37`)이라, `HomeLocation` 이 비어 있으면 `DistSquared` 가 천문학적 값이 되어 항상 `true`(이탈)로 판정된다. 그 결과 AI 는 경고 하나 없이 영구 복귀 모드에 갇힌다. 현재는 `Source/WxGame/Controller/WxEnemyController.cpp:40` 이 `OnPossess` 에서 항상 채워 주므로 발현하지 않지만, 다른 컨트롤러나 스폰 경로가 생기면 진단이 어려운 형태로 터진다.
- **제안**: `FAISystem::IsValidLocation(Home)` 으로 가드하고, 무효면 `false` 를 반환하며 `LogWxAI` 경고를 남긴다(`WxBlackboardKeys.cpp` 의 키 검증 진단과 같은 결).
- **확신도**: 중간

### 10. 🟢 `EWxTeam` 이 WxAI 에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:8-14`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:11`·`:121`, `WxCharacterBase.cpp:170`·`:182`, `WxEnemyCharacter.cpp:21`, `WxPlayerCharacter.cpp:22` 뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다(퍼셉션의 진영 판정은 엔진 `IGenericTeamAgentInterface` 를 그대로 쓴다). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이라, 다른 도메인 플러그인(예: WxCombat 의 피아 필터)이 필요해지는 순간 "WxCore 외 플러그인 참조 금지" 규칙을 어기지 않고는 쓸 수 없다.
- **제안**: `WxTeamTypes.h` 를 `WxCore` 로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 높음

### 11. 🟢 Patrol 과 Wander 의 감속 GE 부여·제거 코드가 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52-61`·`:81-85`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55-64`·`:107-111`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf` → `OnTaskFinished` 에서 `RemoveActiveGameplayEffect` 까지 두 파일이 한 줄도 다르지 않게 반복된다(헤더의 `MoveSpeedMultiplier`/`MoveSpeedEffect`/`MoveSpeedEffectHandle` 3필드 세트도 동일). SetByCaller 태그나 핸들 수명 규약을 바꾸면 두 곳을 함께 고쳐야 한다.
- **제안**: 두 태스크의 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode` 라 공통 베이스를 만들 수 없으므로, "부여/해제" 두 함수만 작은 유틸로 뽑아 양쪽이 호출한다.
- **확신도**: 높음

### 12. 🟢 타겟 교체 정책이 "가장 최근 자극이 무조건 승리"다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:77-79`
- **범주**: 설계/구조
- **문제**: 이미 타겟을 추적 중이어도 다른 적대 액터의 성공 자극이 들어오면 그대로 갈아탄다. 거리·최근 피격·현재 타겟 유지 같은 우선순위가 전혀 없어, 적대 대상이 둘 이상인 상황(코옵, 또는 `UWxAnimNotify_ReportNoise` 소음이 멀리서 발생)에서 타겟이 자극마다 오갈 수 있다. README 에 명시된 의도이므로 버그는 아니지만, 다중 플레이어를 붙이면 가장 먼저 드러날 지점이다.
- **제안**: 지금 고칠 필요는 없다. 다만 발견 1 의 적대 게이트를 넣을 때 "현재 타겟이 유효하면 유지" 같은 유지 규칙을 함께 두는 것이 자연스럽다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 15개, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, 경계 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`·`Source/WxGame/Character/WxEnemyCharacter.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**:
  - BT/Blackboard 에셋 자체를 열지 않았다. 발견 4·5·6·8 은 "디자이너가 이 필드를 이렇게 설정하면"이 전제이므로, 실제 에셋에서 해당 설정이 쓰이는지는 확인하지 못했다.
  - 발견 2 는 엔진 코드 경로를 대조해 메커니즘은 확정했으나 런타임 재현은 하지 않았다. 어빌리티 BP 에셋의 `bRetriggerInstancedAbility` 실제 설정값은 텍스트로 확인할 수 없어 미확인이다.
  - `UWxAIPerceptionComponent` 가 생성자에서 `OnTargetPerceptionUpdated.AddDynamic` 을 하는 것이 `FObjectInitializer::InitProperties` 의 CDO→인스턴스 프로퍼티 복사와 어떻게 맞물리는지는 이번에도 정적으로 단정하지 못했다. 현재 동작에 문제가 관측되지 않아(타겟팅이 실제로 동작) 발견으로 올리지 않았다.
  - `UWxBTService_TargetDistance` 의 `RandomDeviation = 0.0f`(엔진 기본 0.1 을 덮어 서비스 틱 분산을 끔), `WxBTTask_ActivateAbility.h:6` 이 Public 헤더에서 `AbilitySystemComponent.h` 전체를 끌어오는 점은 영향이 미미해 발견으로 세우지 않았다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 29파일 — `/module-review`로 갱신*
