# WxAI — 코드 리뷰

> 여전히 건강한 모듈이다. 플러그인 의존은 `WxCore`뿐이고, 프로젝트 코딩 규칙(Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix·`BlueprintCallable`/`FORCEINLINE`/람다 금지) 위반은 29파일 전체에서 한 건도 없다. 다만 "감지 → 타겟 → 인식"이 전부 **엣지 트리거 이벤트**에 매달려 있어, 이벤트가 한 번 유실되는 경로에서 상태가 조용히 굳는다. 이번 리뷰는 29개 소스를 전부 훑고 퍼셉션 컴포넌트·BT Task/Decorator/Composite의 cpp를 UE 5.8 엔진 소스(`BTDecorator.cpp`·`BehaviorTreeComponent.cpp`·`AISense_Sight.cpp`·`AIPerceptionComponent.cpp`·`AbilitySystemComponent_Abilities.cpp`)와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `AttributeRatio` 데코레이터는 실시간 재평가가 구조적으로 불가능한데 헤더는 가능하다고 안내한다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h:14`, 구현 `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:35`
- **범주**: 버그/정확성
- **문제**: 헤더는 "실시간 재평가가 필요한 경우 FlowAbortMode 를 LowerPriority/Self/Both 로 설정한다"고 적었지만, 엔진에서 `FlowAbortMode`는 **무엇을 중단할 수 있는지만 선언**할 뿐 재평가를 촉발하지 않는다. 실제 중단은 누군가 `UBTDecorator::ConditionalFlowAbort` 또는 `UBehaviorTreeComponent::RequestExecution`을 호출해야 일어나며, 엔진 전체에서 그 호출을 만드는 곳은 블랙보드 키 옵저버(`BTDecorator_Blackboard.cpp:73`)와 자체 폴링(`BTDecorator_ConeCheck.cpp:107-115`)뿐이다. `UWxBTDecorator_AttributeRatio`는 둘 중 아무것도 하지 않으므로 **브랜치가 재탐색될 때만** 평가된다. 즉 "HP가 30% 아래로 내려가면 즉시 광폭화 브랜치로 전환" 같은 기대는 성립하지 않고, 대신 관계없는 다른 태스크가 끝나 재탐색이 돌 때 뒤늦게 전환된다. 같은 모듈의 `UWxBTDecorator_BeyondLeash`가 왜 굳이 `TickNode` 폴링을 두었는지(`WxBTDecorator_BeyondLeash.cpp:50-61`)와 정확히 대비된다.
- **제안**: BeyondLeash와 같은 패턴으로 인스턴스 메모리에 직전 결과를 두고 `TickNode`에서 폴링해 `RequestExecution`을 걸거나, `OnBecomeRelevant`에서 `ASC->GetGameplayAttributeValueChangeDelegate(Attribute)`를 구독해 변화 시점에만 재평가를 요청한다. 실시간 전환을 안 쓸 거라면 헤더 주석을 실제 동작("브랜치 재탐색 시에만 평가")으로 정정한다.
- **확신도**: 높음

### 2. 🟡 Patrol의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:47-50`, `:64`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask`는 하드코딩된 `WxBlackboardKeys::PatrolTargetLocation`에 목표를 쓰지만, 실제 이동은 `Super::ExecuteTask`가 `UBTTask_BlackboardBase::BlackboardKey`(엔진에서 `EditAnywhere`)를 읽어 수행한다. 생성자에서 기본값을 맞춰 둔 것뿐이라, 디자이너가 BT 에디터에서 `BlackboardKey`를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation`에 쓰고 엉뚱한 키(대개 0,0,0 또는 stale 값)로 이동한다. 경고 하나 없이 폰이 월드 원점 쪽으로 걸어가는 형태로 드러난다.
- **제안**: 쓰기도 `BlackboardKey.SelectedKeyName`을 경유해 같은 키를 쓰고 읽게 통일하거나, 반대로 `BlackboardKey`를 편집 불가로 잠근다. `UWxBTTask_ReturnHome`은 읽기만 하므로 동일 위험이 없다.
- **확신도**: 높음

### 3. 🟡 ActivateAbility: 재발동 어빌리티에서 태스크가 어빌리티보다 먼저 끝나고, 이후 종료 통지도 못 받는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:41-70`, 콜백 `:111-138`
- **범주**: 버그/정확성
- **문제**: 엔진 `InternalTryActivateAbility`는 `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 이미 활성인 채로 재발동되면 기존 인스턴스를 `EndAbility(bWasCancelled=false)`로 끝낸 뒤 그대로 재활성화한다(`AbilitySystemComponent_Abilities.cpp:1832-1852`). 이때 `HandleAbilityEnded`가 **같은 핸들**로 들어와 `CleanUp()`으로 구독을 끊고 `ActivationResult = Succeeded`를 채운다. 이후 재활성화가 성공해 `TryActivateAbility`가 true를 돌려주면, `ExecuteTask`는 `:67-70`에서 그 낡은 결과를 그대로 반환해 태스크를 즉시 종료한다. 결과적으로 어빌리티는 계속 도는데 BT는 다음 행동으로 넘어가고(공격 모션 중에 이동/배회가 겹침), 그 어빌리티의 진짜 종료 통지를 받을 구독도 남아 있지 않다.
  전 리뷰에서 "재발동 어빌리티를 안 쓰면 무해"로 낮게 잡았으나, 실제로는 `WxAbility_Attack`·`WxAbility_Skill`·`WxAbility_HitReact`가 모두 `bRetriggerInstancedAbility = true`이고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:21` 등) 베이스가 `InstancedPerActor`(`WxAbilityBase.cpp:21`)라, AI가 BT로 부르는 어빌리티가 전부 이 경로에 해당한다. 발현 조건은 "발동 시점에 그 어빌리티가 이미 활성"이라 흔하진 않지만(정상 경로에서는 `AbortTask`가 취소하고 끝난다), 이벤트 기반으로 걸린 HitReact나 병렬 브랜치와 겹치면 열린다.
- **제안**: `ActivationResult`를 신뢰하기 전에 `ActivatedHandle`이 유효하고 스펙이 여전히 활성인지 먼저 확인하도록 `:67-70`과 `:72-85`의 순서를 뒤집거나, `HandleAbilityEnded`가 `bIsActivating` 구간에서는 `CleanUp()`(구독 해제)을 미루고 결과만 기록하게 한다.
- **확신도**: 중간(엔진 경로는 확인했고, 발현은 어빌리티가 이미 활성인 상황에 달렸다)

### 4. 🟡 타겟 억제를 풀어도 "이미 보이고 있는" 상대는 다시 잡지 못한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:125-140`, `:69-84`
- **범주**: 설계/구조
- **문제**: 이 컴포넌트는 타겟을 오직 `OnTargetPerceptionUpdated` 브로드캐스트로만 획득하는데, 억제 중에는 `HandleTargetPerceptionUpdated`가 `:71-74`에서 즉시 반환해 그 사이의 자극을 전부 버린다. 문제는 Sight 센스가 `NotifyType = EAISenseNotifyType::OnPerceptionChange`(`AISense_Sight.cpp:159`)라 **보임↔안 보임 상태가 바뀔 때만** 브로드캐스트한다는 점이다(계속 보이는 동안에는 `AIPerceptionComponent.cpp:546-547`의 `bActorInfoUpdated`가 false라 통지가 나가지 않는다). 따라서 `SetTargetingSuppressed(false)`로 억제를 풀어도 상대가 그동안 계속 시야에 있었다면 새 통지가 없어 `TargetActor`가 빈 채로 남고, AI는 눈앞의 플레이어를 무시한 채 서 있게 된다. 시야를 한 번 끊었다 잇거나(플레이어가 시야각 밖으로 나갔다 들어옴) 청각·피격 자극(둘 다 `OnEveryPerception`)이 와야 복구된다. 리시 복귀 후, 그리고 발견 5의 실패 경로에서 그대로 노출된다.
- **제안**: `SetTargetingSuppressed(false)` 시 `GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), ...)`로 현재 감지 중인 액터를 다시 훑어 살아 있는 적대 대상을 재획득한다(엣지 이벤트에 더해 레벨 기반 복구 경로를 하나 둔다).
- **확신도**: 중간(엔진 통지 규약은 확인했고, 실제 발현 빈도는 시야각·지형에 달렸다)

### 5. 🟡 복귀 이동이 실패하면 그때마다 타겟이 조용히 비워진다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:29-35`, `:38-49`
- **범주**: 설계/구조
- **문제**: `ExecuteTask`는 `Super::ExecuteTask`(MoveTo)의 성공 여부를 알기 **전에** `SetTargetingSuppressed(true)`를 부른다. HomeLocation이 내비메시 밖이거나 경로가 없어 MoveTo가 `Failed`를 반환하면, 그 한 번의 헛발질이 이미 `SetTargetActor(nullptr)` → 포커스 해제 + CMC 회전 모드 원복 + `State.InCombat` 태그 제거(`WxAIPerceptionComponent.cpp:125-140`, `:200-244`)를 끝낸 뒤다. `OnTaskFinished`가 억제만 풀 뿐 타겟은 복구하지 않으므로(`SetTargetingSuppressed(false)`는 `UpdateRecognition`조차 부르지 않는다), 리시 데코가 계속 참인 동안 재탐색마다 이 일이 반복되며 그때마다 추적이 끊긴다. 발견 4 때문에 재획득도 즉시 되지 않아 두 결함이 겹친다.
- **제안**: `Super::ExecuteTask` 결과가 `InProgress`일 때만 억제를 켜고, 그 외 결과에서는 억제를 아예 건드리지 않도록 순서를 뒤집는다.
- **확신도**: 중간

### 6. 🟡 Once 정찰 완료 후 태스크가 매 BT 틱 즉시 Succeeded를 반환해 트리 전체 재탐색이 상시화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:42-45`
- **범주**: 성능/안전
- **문제**: `bPatrolFinished`가 서면 `ExecuteTask`가 이동 없이 동기 `Succeeded`를 반환한다. 엔진은 즉시 끝난 태스크에 대해 `ScheduleExecutionUpdate()`를 걸고 다음 틱 `ProcessExecutionRequest()`로 루트부터 재탐색하므로(`BehaviorTreeComponent.cpp:1804-1812`), 이 폰은 살아 있는 내내 **매 프레임 전체 트리 검색**을 돈다. 지연을 만드는 노드가 하나도 없는 브랜치를 영구 점유하는 구조라, 재탐색마다 모든 형제 데코레이터 평가 비용이 따라붙는다. 프레임 내 무한 루프는 아니지만(엔진 tick 당 1회 처리), Once 정찰 적이 많은 맵에서는 그대로 누적된다.
- **제안**: 완료 상태에서는 `InProgress`를 반환해 실제로 브랜치를 점유하게 한다(주석이 말하는 "그 자리에 머문다"의 정확한 표현이며, 상위 우선순위 abort는 그대로 동작한다).
- **확신도**: 중간

### 7. 🟢 `EWxTeam`이 WxAI에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:10`
- **범주**: 설계/구조
- **문제**: 저장소 전체에서 소비자는 `Source/WxGame/Character/WxCharacterBase.{h,cpp}`·`WxEnemyCharacter.cpp`·`WxPlayerCharacter.cpp`뿐이고, WxAI 코드는 이 타입을 한 번도 참조하지 않는다(퍼셉션의 진영 판정은 엔진 `IGenericTeamAgentInterface`를 그대로 쓴다). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념인데 AI 플러그인이 소유하고 있어, 다른 도메인(예: WxCombat의 타겟 필터)이 필요해지는 순간 의존 규칙 위반 없이는 참조할 수 없다.
- **제안**: `WxTeamTypes.h`를 `WxCore`(공용 정의)로 옮긴다. 소비자가 3파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 중간

### 8. 🟢 Patrol과 Wander가 감속 GameplayEffect 적용·해제 로직을 통째로 중복한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52-62`·`:82-86` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:56-65`·`:108-112` (선언부도 `WxBTTask_Patrol.h:33-44`·`:57-58` ↔ `WxBTTask_Wander.h:56-67`·`:76-77`)
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`/`MoveSpeedEffect` UPROPERTY, `MoveSpeedEffectHandle` 멤버, spec 생성 + `SetByCaller_MoveSpeedScale` 주입 + 적용 블록, `OnTaskFinished`의 제거 블록이 두 파일에 문자 단위로 동일하다. GE 규약이 바뀔 때(예: SetByCaller 태그 변경, 스택 정책 추가) 한쪽만 고치면 정찰과 배회의 속도 거동이 조용히 갈린다.
- **제안**: 프로젝트는 공통 부모·컴포넌트 도입보다 인플레이스 반복 용인을 선호하므로(그리고 두 태스크의 베이스가 `UBTTask_MoveTo`/`UBTTaskNode`로 갈려 상속 통합도 부자연스럽다) **당장 구조를 바꾸라는 뜻은 아니다**. 다만 두 곳이 짝이라는 사실만 기억해 두고, GE 규약을 건드릴 때 반드시 함께 고친다.
- **확신도**: 낮음(프로젝트의 중복 허용 방침과 상충할 수 있음)

### 9. 🟢 Blackboard accessor의 키 검증이 매 프레임 폴링 경로에서 반복 실행된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:15-33`, 호출부 `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:37`
- **범주**: 성능/안전
- **문제**: `VerifyBlackboardKey`는 호출마다 `GetKeyID`(에셋 키 배열 선형 탐색) + `GetKeyType`을 돈다. `UWxBTDecorator_BeyondLeash::TickNode`는 관찰 중 매 프레임 `GetHomeLocation`을 부르므로, Development/Editor 빌드에서 AI 수 × 프레임만큼 같은 검증이 반복된다(Shipping에서는 비어 있음). 키가 5개뿐이라 절대 비용은 작지만, "에셋 설정 오류를 드러낸다"는 목적에 매 호출 반복은 불필요하다.
- **제안**: 검증을 Blackboard 에셋별 1회만 수행하도록 캐시하거나, 엔진 관례대로 `FBlackboardKeySelector`로 KeyID를 캐시해 이름 조회 자체를 없앤다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Public/` 헤더 15개 전부, 소비자 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **검증 과정에서 기각한 가설**: (a) `FWxBeyondLeashMemory::bWasBeyond` 미초기화 — 노드 메모리는 엔진이 0 초기화하고 `INIT_DECORATOR_NODE_NOTIFY_FLAGS()`가 `bNotifyBecomeRelevant`를 세워 `OnBecomeRelevant`가 항상 먼저 시드하므로 안전. (b) `UWxBTComposite_RandomChoice`가 `UBTComposite_Selector`를 상속해 Selector 시멘틱이 새어 나올 가능성 — UE 5.8 Selector는 `GetNextChildHandler`만 있는 껍데기(`BTComposite_Selector.cpp` 전체 38줄)라 무해. `FWxBTRandomChoiceMemory`도 베이스 placement-new가 앞 2바이트만 덮어 `LastChosenChild`를 건드리지 않음. (c) `EWxWanderDirection`에 `meta=(Bitflags)` 누락 — 디테일 패널은 프로퍼티 쪽 `Bitmask`/`BitmaskEnum`만 보고 값을 비트 인덱스로 해석하므로 문제 없음(`SPropertyEditorNumeric.h:70-120`). (d) `UWxPatrolComponent::ConfigureSpline`의 닫힘 상태 처리 — `GetNumberOfSplinePoints()`가 closed loop에서 가짜 끝점을 세지 않으므로 인덱스 계산 안전. (e) `UWxBTTask_Wander`의 `AddMovementInput` — AI 폰도 `IsLocallyControlled()`가 참이라 CMC가 입력을 정상 소비.
- **미검토 / 한계**: BT/Blackboard 에셋의 실제 노드 배치(`UWxBTDecorator_BeyondLeash`의 FlowAbortMode가 실제로 Lower Priority인지, Patrol의 `BlackboardKey`가 기본값 그대로인지, `AttributeRatio`에 FlowAbortMode가 걸려 있는지)는 에셋 영역이라 확인하지 않았다 — 발견 1·2·5의 실제 발현 여부는 에셋 설정에 달려 있다. 멀티플레이 실환경의 `State.InCombat` MinimalReplication 타이밍과 다수 AI 동시 구동 프로파일링도 하지 않았다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 29파일 — `/module-review`로 갱신*
