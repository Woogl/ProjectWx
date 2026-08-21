# WxAI — 코드 리뷰

> 규칙 준수와 모듈 경계는 흠잡을 데가 없다 — 플러그인 의존은 `WxCore`뿐이고, 29파일 전체에서 Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix 누락이나 `BlueprintCallable`/`FORCEINLINE`/불필요한 람다 사용이 한 건도 없다. 위험은 대부분 "엔진이 대신 해 주는 줄 알았던 것"에 몰려 있다: 퍼셉션의 엣지 트리거 통지, BT Composite의 abort 관찰자 등록, MoveTo 파생 태스크의 키 이원화가 그것이다. 이번 리뷰는 소스 29개를 모두 읽고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite의 cpp를 UE 5.8 엔진 소스(`AIPerceptionComponent.cpp`·`AISense_Sight.cpp`·`AISenseConfig_Damage.h`·`BTCompositeNode.cpp`·`BTDecorator.h`·`BlackboardKeyType_Vector.cpp`·`AbilitySystemComponent_Abilities.cpp`)와 대조해 각 발견의 메커니즘을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 8 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 억제를 풀어도 "계속 보이고 있던" 대상은 다시 잡히지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:71-74`, `:124-139`
- **범주**: 버그/정확성
- **문제**: 타겟 획득 경로가 `OnTargetPerceptionUpdated` 브로드캐스트 하나뿐인데, 억제 중에는 `:71-74`가 즉시 반환해 그 구간의 통지를 전부 버린다. 문제는 Sight 센스가 `NotifyType = EAISenseNotifyType::OnPerceptionChange`(`AISense_Sight.cpp:159`)라 **보임↔안 보임이 뒤집힐 때만** 통지가 나간다는 점이다. 엔진은 `bActorInfoUpdated = WantsToNotifyOnlyOnPerceptionChange() == false || WasSuccessfullySensed() != StimulusStore.WasSuccessfullySensed()` 로 갱신 여부를 정하고, 성공 자극 경로의 `ConditionallyStoreSuccessfulStimulus` 는 **항상 false** 를 돌려주므로(`AIPerceptionComponent.cpp`), 계속 보이는 동안에는 브로드캐스트 자체가 없다. 따라서 억제가 켜졌다 꺼지는 동안 대상이 내내 시야에 있었다면 `SetTargetingSuppressed(false)` 이후에도 새 통지가 오지 않아 `TargetActor`가 빈 채로 남는다 — AI가 눈앞의 플레이어를 무시하고 서 있는 형태다. 리시 경계에서 `UWxBTTask_ReturnHome`이 몇 프레임만 실행됐다 중단되는 경우가 가장 열리기 쉽다.
- **제안**: `SetTargetingSuppressed(false)` 시 `GetCurrentlyPerceivedActors(...)`로 현재 감지 목록을 한 번 훑어 살아 있는 적대 대상을 재획득한다. 엣지 이벤트 옆에 레벨 기반 복구 경로를 하나 두는 것이 핵심이다.
- **확신도**: 중간(엔진 통지 규약은 확인했고, 발현 빈도는 시야각·지형에 달렸다)

### 2. 🟡 복귀 이동이 실패해도 타겟은 이미 비워진 뒤다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:29-35`, `:38-48`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask`가 `Super::ExecuteTask`(MoveTo)의 성패를 알기 **전에** `SetTargetingSuppressed(true)`를 부른다. HomeLocation이 내비메시 밖이거나 경로가 없어 MoveTo가 `Failed`를 반환하면, 그 헛발질 한 번이 이미 `SetTargetActor(nullptr)` → 포커스 해제 → CMC 회전 모드 원복 → `State.InCombat` 제거까지 끝낸 뒤다(`WxAIPerceptionComponent.cpp:124-139`, `:199-243`). `OnTaskFinished`는 억제만 풀고 타겟을 되돌리지 않으며 `UpdateRecognition`조차 부르지 않으므로, 리시 데코가 참인 동안 재탐색마다 이 손실이 반복된다. 발견 1 때문에 자동 재획득도 즉시 되지 않아 두 결함이 겹친다.
- **제안**: `Super::ExecuteTask` 결과가 `InProgress`일 때만 억제를 켠다(다른 결과에서는 억제를 아예 건드리지 않는다).
- **확신도**: 중간

### 3. 🟡 ActivateAbility: 재발동 경로에서 태스크가 어빌리티보다 먼저 끝나고 종료 통지도 잃는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:49-63`, `:66-69`, 콜백 `:110-137`
- **범주**: 버그/정확성
- **문제**: `ActivatedHandle`을 `TryActivateAbility` **호출 전에**(`:54`) 세워 두고, `HandleAbilityEnded`는 스펙 핸들만으로 "내 실행의 종료"를 판별한다. 그런데 엔진은 `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 이미 활성인 채로 재발동되면 **기존 인스턴스를 `EndAbility(bWasCancelled=false)`로 먼저 끝낸 뒤** 재활성화한다(`AbilitySystemComponent_Abilities.cpp:1834-1843`). 이때 같은 핸들로 콜백이 들어와 `CleanUp()`이 구독을 끊고 `ActivationResult = Succeeded`가 채워진다. 이어 재활성화가 성공하면 `:66-69`가 그 낡은 결과를 그대로 반환해 태스크를 즉시 종료한다 — 어빌리티는 계속 도는데 BT는 다음 행동으로 넘어가고(공격 모션 위에 이동·배회가 겹침), `ActivatedHandle`도 비어 있어 `AbortTask`가 취소할 수도 없다. 더 나쁜 변종: 첫 후보가 이 경로를 타고 `TryActivateAbility`가 false를 반환하면 루프는 다음 후보로 넘어가는데, 구독은 이미 `CleanUp()`으로 끊긴 뒤라 그 후보의 종료 통지가 영영 오지 않고 BT가 `InProgress`로 영구 정지한다.
- **제안**: `bIsActivating` 구간의 콜백은 결과만 기록하고 `CleanUp()`(구독 해제)은 `ExecuteTask` 반환 직전으로 미룬다. 또는 `:66-69`의 `ActivationResult` 신뢰를 `:79-84`의 "핸들 유효 + 스펙 활성" 확인 뒤로 미뤄, 실제로 도는 어빌리티가 있으면 InProgress를 유지하게 한다.
- **확신도**: 중간(엔진 경로는 확인했고, `bRetriggerInstancedAbility`는 기본 false라 디자이너가 켠 어빌리티에서만 발현한다)

### 4. 🟡 Patrol의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:46-49`, `:63`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask`는 하드코딩된 `WxBlackboardKeys::PatrolTargetLocation`에 목표를 쓰지만, 실제 이동은 `Super::ExecuteTask`가 `UBTTask_BlackboardBase::BlackboardKey`(엔진에서 `EditAnywhere`)를 읽어 수행한다. 생성자 `:19`는 기본값을 맞춰 둔 것뿐이라, 디자이너가 BT 에디터에서 키를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation`에 쓰고 엉뚱한 키(대개 미설정 = `FAISystem::InvalidLocation`)로 이동을 시도한다. 경고 하나 없이 정찰이 실패하거나 폰이 엉뚱한 곳으로 걸어가는 형태로만 드러난다.
- **제안**: 쓰기도 `BlackboardKey`를 경유해 읽는 키와 통일하거나, 반대로 `BlackboardKey` 편집을 잠근다. `UWxBTTask_ReturnHome`은 읽기만 하므로 동일 위험이 없다.
- **확신도**: 높음(메커니즘) / 중간(실제 에셋 설정에 달림)

### 5. 🟡 RandomChoice의 사전 필터가 형제 데코레이터의 abort 관찰자 등록을 통째로 건너뛴다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp:57-84`
- **범주**: 설계/구조
- **문제**: 후보 수집 루프가 `DoDecoratorsAllowExecution`으로 조건 실패 자식을 **직접** 걸러내고 통과한 자식 인덱스를 바로 반환한다. 그런데 엔진에서 "조건 실패 자식"의 처리는 `UBTCompositeNode::FindChildToExecute`의 `NotifyDecoratorsOnFailedActivation` 경로가 담당하며, 바로 그 함수가 `FlowAbortMode == LowerPriority | Both`인 데코레이터를 aux 노드로 등록한다(`BTCompositeNode.cpp`). 사전 필터로 걸러진 자식은 이 경로를 밟지 않으므로, RandomChoice **안에 있는** 형제의 LowerPriority 데코레이터(`UWxBTDecorator_BeyondLeash`나 엔진 Blackboard 관찰자 등)는 관찰자로 등록되지 않고 실행 중인 자식을 선점하지 못한다. 같은 이유로 조건 데코레이터는 선택된 자식에 대해 두 번 평가되고(사전 필터 + 엔진 재검사), 걸러진 자식의 `WrappedOnNodeProcessed`는 호출되지 않는다.
- **제안**: RandomChoice 하위에 관찰자 abort 데코레이터를 두지 않는다는 제약을 클래스 주석과 `GetStaticDescription`에 명시하거나, 필터에서 걸러낸 자식마다 `NotifyDecoratorsOnFailedActivation`을 직접 호출해 엔진 규약을 복원한다.
- **확신도**: 중간(공격 패턴 추첨 용도로만 쓴다면 무해하지만, 규약 이탈이라 확장 시 조용히 깨진다)

### 6. 🟡 Damage 센스에는 진영 필터가 없는데 타겟 채택에도 적대 판정이 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:34`, `:77-79`
- **범주**: 버그/정확성
- **문제**: Sight(`:19-21`)와 Hearing(`:29-31`)은 `DetectionByAffiliation`으로 적만 감지하도록 막아 두었지만, `UAISenseConfig_Damage`에는 애초에 `DetectionByAffiliation` 필드가 없어 **가해자가 누구든** 자극이 들어온다. 그리고 `HandleTargetPerceptionUpdated`는 성공 자극이면 사망 여부만 보고 무조건 `SetTargetActor(Actor)`를 부른다. `Source/WxGame/Character/WxEnemyCharacter.cpp:73`이 `Context.GetInstigator()`를 그대로 실어 `UAISense_Damage::ReportDamageEvent`를 호출하므로, 아군 광역기·환경 데미지·도트의 Instigator가 그대로 적의 타겟이 된다(같은 진영 NPC를 공격하는 형태).
- **제안**: `SetTargetActor` 진입 전에 `FGenericTeamId::GetAttitude(GetOwner(), Actor) == ETeamAttitude::Hostile` 게이트를 둔다. 엔진도 `FActorPerceptionInfo::bIsHostile`을 같은 방식으로 계산하므로 규약이 어긋나지 않는다.
- **확신도**: 중간(아군 오사가 실제로 발생하는 설계인지에 달렸다)

### 7. 🟡 AttributeRatio에는 재평가 트리거가 없어 FlowAbortMode 설정이 무효다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:9-13`, `:35-79`
- **범주**: 설계/구조
- **문제**: 이 데코레이터는 `CalculateRawConditionValue`만 구현하고, 값 변화를 관찰할 장치(어트리뷰트 변경 델리게이트 구독이나 `TickNode` 폴링)가 없다. 엔진에서 관찰자 abort는 노드가 스스로 `RequestExecution`을 부를 때만 일어나므로, 디자이너가 디테일 패널에서 FlowAbortMode를 지정해도 **아무 일도 일어나지 않는다** — "HP가 30% 아래로 떨어지면 즉시 광폭화 브랜치로 전환" 같은 의도가 조용히 무시되고, 다음 재탐색까지 미뤄진다. 같은 모듈의 `UWxBTDecorator_BeyondLeash`는 정확히 이 문제 때문에 `TickNode` 폴링을 두었다(`WxBTDecorator_BeyondLeash.cpp:49-61`).
- **제안**: BeyondLeash와 같은 폴링을 넣거나(`INIT_DECORATOR_NODE_NOTIFY_FLAGS` + `TickNode`에서 값 전이 시 `RequestExecution`), ASC의 `GetGameplayAttributeValueChangeDelegate`를 구독한다. 지원하지 않기로 한다면 생성자에서 `bAllowAbortLowerPri = false; bAllowAbortChildNodes = false;`로 드롭다운을 잠가 오해를 없앤다.
- **확신도**: 중간

### 8. 🟡 Once 정찰 완료 후 매 BT 틱 즉시 Succeeded를 반환해 전체 재탐색이 상시화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:41-44`
- **범주**: 성능/안전
- **문제**: `bPatrolFinished`가 서면 `ExecuteTask`가 이동도 지연도 없이 동기 `Succeeded`를 반환한다. 엔진은 즉시 끝난 태스크에 대해 실행 갱신을 예약하고 다음 틱에 재탐색을 돌리므로, 이 폰은 살아 있는 내내 **매 프레임 트리 검색**을 반복한다(형제 데코레이터 평가 비용이 전부 따라붙는다). 프레임 내 무한 루프는 아니지만 Once 정찰 적이 많은 맵에서는 그대로 누적된다.
- **제안**: 완료 상태에서는 `InProgress`를 반환해 실제로 브랜치를 점유하게 한다. 주석이 말하는 "그 자리에 머문다"의 정확한 표현이며, 상위 우선순위 abort는 그대로 동작한다.
- **확신도**: 중간

### 9. 🟢 BeyondLeash의 "Self/Both 금지"가 주석으로만 있고 에디터에서 막히지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:11-19`, 경고 주석 `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:21-22`
- **범주**: 설계/구조
- **문제**: 헤더는 Self/Both를 고르면 "복귀가 경계에서 끊기고 경계 왕복이 난다"고 명시하는데, 생성자는 `FlowAbortMode`에 기본값만 넣을 뿐 잘못된 선택지를 막지 않는다. 엔진은 바로 이 목적의 플래그(`bAllowAbortNone`·`bAllowAbortLowerPri`·`bAllowAbortChildNodes`, `BTDecorator.h`)로 디테일 패널 드롭다운을 제한할 수 있는데 쓰지 않았다. 잘못 고르면 재현·진단이 어려운 왕복 버그로만 드러난다.
- **제안**: 생성자에서 `bAllowAbortNone = false; bAllowAbortChildNodes = false;`를 세워 LowerPriority 외 선택 자체를 없앤다.
- **확신도**: 중간(의도적으로 여지를 남긴 것일 수 있음)

### 10. 🟢 BeyondLeash가 HomeLocation 미설정을 "이탈"로 오판한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:36-37`
- **범주**: 버그/정확성
- **문제**: Blackboard Vector 키의 미설정 값은 `FAISystem::InvalidLocation`(`FLT_MAX` 성분, `BlackboardKeyType_Vector.cpp`)이라, `HomeLocation`이 비어 있으면 `DistSquared`가 천문학적 값이 되어 항상 `true`(이탈)로 판정된다. 그 결과 AI는 경고 하나 없이 영구 복귀 모드에 갇힌다. 현재는 `Source/WxGame/Controller/WxEnemyController.cpp:40`이 `OnPossess`에서 항상 채워 주므로 발현하지 않지만, 다른 컨트롤러나 스폰 경로가 생기면 진단이 어려운 형태로 터진다.
- **제안**: `FAISystem::IsValidLocation(Home)`으로 가드하고, 무효면 `false`를 반환하며 `LogWxAI` 경고를 남긴다(`WxBlackboardKeys.cpp`의 키 검증 진단과 같은 결).
- **확신도**: 중간

### 11. 🟢 `EWxTeam`이 WxAI에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:8-14`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:11`·`:121`, `WxCharacterBase.cpp:162`·`:174`, `WxEnemyCharacter.cpp:21`, `WxPlayerCharacter.cpp:22`뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다(퍼셉션의 진영 판정은 엔진 `IGenericTeamAgentInterface`를 그대로 쓴다). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이라, 다른 도메인 플러그인(예: WxCombat의 피아 필터)이 필요해지는 순간 "WxCore 외 플러그인 참조 금지" 규칙을 어기지 않고는 쓸 수 없다.
- **제안**: `WxTeamTypes.h`를 `WxCore`로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 높음

### 12. 🟢 Patrol과 Wander의 감속 GE 부여·제거 코드가 통째로 중복이다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52-61`·`:81-85`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55-64`·`:107-111`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf` → `OnTaskFinished`에서 `RemoveActiveGameplayEffect` 까지 두 파일이 한 줄도 다르지 않게 반복된다(헤더의 `MoveSpeedMultiplier`/`MoveSpeedEffect`/`MoveSpeedEffectHandle` 3필드 세트도 동일). SetByCaller 태그나 핸들 수명 규약을 바꾸면 두 곳을 함께 고쳐야 한다.
- **제안**: 두 태스크의 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode`라 공통 베이스를 만들 수 없으므로, "부여/해제" 두 함수만 `WxCore`(또는 WxAI 내부)의 작은 유틸로 뽑아 양쪽이 호출한다.
- **확신도**: 높음

### 13. 🟢 타겟 교체 정책이 "가장 최근 자극이 무조건 승리"다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:77-79`
- **범주**: 설계/구조
- **문제**: 이미 타겟을 추적 중이어도 다른 적대 액터의 성공 자극이 들어오면 그대로 갈아탄다. 거리·최근 피격·현재 타겟 유지 같은 우선순위가 전혀 없어, 적대 대상이 둘 이상인 상황(코옵, 또는 `UWxAnimNotify_ReportNoise` 소음이 멀리서 발생)에서 타겟이 프레임마다 오갈 수 있다. README에 명시된 의도이므로 버그는 아니지만, 다중 플레이어를 붙이면 가장 먼저 드러날 지점이다.
- **제안**: 지금 고칠 필요는 없다. 다만 발견 6의 적대 게이트를 넣을 때 "현재 타겟이 유효하면 유지" 같은 유지 규칙을 함께 두는 것이 자연스럽다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, 대응 `Public/` 헤더 15개, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`
- **미검토 / 한계**:
  - BT/Blackboard 에셋 자체를 열지 않았다. 발견 4·5·7·9는 "디자이너가 이 필드를 이렇게 설정하면"이 전제이므로, 실제 에셋에서 해당 설정이 쓰이는지는 확인하지 못했다.
  - 발견 1·3은 엔진 코드 경로를 대조해 메커니즘은 확정했으나 런타임 재현은 하지 않았다. 발현 조건(시야 유지 여부, `bRetriggerInstancedAbility` 설정)에 달려 있다.
  - `UWxAIPerceptionComponent`가 생성자에서 `OnTargetPerceptionUpdated.AddDynamic`을 하고 `PostInitProperties`에서 `ConfigureSense`를 호출하는 순서는 UE의 서브오브젝트 인스턴싱과 얽혀 있어 정적 판단이 어려웠다. 현재 동작에 문제가 관측되지 않아 발견으로 올리지 않았다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 29파일 — `/module-review`로 갱신*
