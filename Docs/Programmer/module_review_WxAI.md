# WxAI — 코드 리뷰

> 규칙 준수와 경계는 흠잡을 데가 없다 — 플러그인 의존은 `WxCore`뿐이고, 29파일 전체에서 Copyright 첫 줄·`Wx` prefix·`Handle` 콜백 prefix·`BlueprintCallable`/`FORCEINLINE`/람다 금지 위반이 한 건도 없다. 남은 위험은 전부 한 곳에 몰려 있다: "감지 → 타겟 → 인식"이 **엣지 트리거 브로드캐스트**에만 의존해, 그 이벤트가 나오지 않는 구간(억제·복귀)에서 상태가 조용히 굳는다. 이번 리뷰는 소스 29개를 전부 읽고 퍼셉션 컴포넌트와 BT Task/Decorator/Composite의 cpp를 UE 5.8 엔진 소스(`AIPerceptionComponent.cpp`·`AISense_Sight.cpp`·`BTCompositeNode.cpp`·`BTTask_MoveTo.cpp`·`BTDecorator.h`·`AbilitySystemComponent_Abilities.cpp`)와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 억제를 풀어도 "계속 보이고 있던" 대상은 다시 잡히지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:71-74`, `:125-140`
- **범주**: 설계/구조
- **문제**: 타겟 획득 경로가 `OnTargetPerceptionUpdated` 브로드캐스트 하나뿐인데(`HandleTargetPerceptionUpdated`), 억제 중에는 `:71-74`가 즉시 반환해 그 구간의 통지를 전부 버린다. 문제는 Sight 센스가 `NotifyType = EAISenseNotifyType::OnPerceptionChange`(`AISense_Sight.cpp:158`)라 **보임↔안 보임이 뒤집힐 때만** 통지가 나간다는 점이다. 계속 보이는 동안에는 `AIPerceptionComponent.cpp:545-546`의 `bActorInfoUpdated`가 false이고 `ConditionallyStoreSuccessfulStimulus`도 항상 `false`를 돌려주므로(`AIPerceptionComponent.cpp:632-643`) `bRequiresUpdate`가 서지 않아 브로드캐스트 자체가 없다. 따라서 억제가 켜졌다 꺼지는 동안 대상이 내내 시야에 있었다면, `SetTargetingSuppressed(false)` 이후에도 새 통지가 영영 오지 않아 `TargetActor`가 빈 채로 남는다 — AI가 눈앞의 플레이어를 무시하고 서 있는 형태다. 리시 경계에서 짧게 왕복해 `UWxBTTask_ReturnHome`이 몇 프레임만 실행됐다 중단되는 경우가 가장 열리기 쉽다.
- **제안**: `SetTargetingSuppressed(false)` 시 `GetCurrentlyPerceivedActors(...)`로 현재 감지 목록을 한 번 훑어 살아 있는 적대 대상을 재획득한다. 엣지 이벤트 옆에 레벨 기반 복구 경로를 하나 두는 것이 핵심이다.
- **확신도**: 중간(엔진 통지 규약은 확인했고, 발현 빈도는 시야각·지형에 달렸다)

### 2. 🟡 복귀 이동이 실패해도 타겟은 이미 비워진 뒤다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:29-35`, `:38-48`
- **범주**: 설계/구조
- **문제**: `ExecuteTask`가 `Super::ExecuteTask`(MoveTo)의 성패를 알기 **전에** `SetTargetingSuppressed(true)`를 부른다. HomeLocation이 내비메시 밖이거나 경로가 없어 MoveTo가 `Failed`를 반환하면, 그 헛발질 한 번이 이미 `SetTargetActor(nullptr)` → 포커스 해제 → CMC 회전 모드 원복 → `State.InCombat` 제거까지 끝낸 뒤다(`WxAIPerceptionComponent.cpp:125-140`, `:200-244`). `OnTaskFinished`는 억제만 풀고 타겟을 되돌리지 않으며 `UpdateRecognition`조차 부르지 않으므로, 리시 데코가 참인 동안 재탐색마다 이 손실이 반복된다. 발견 1 때문에 자동 재획득도 즉시 되지 않아 두 결함이 겹친다.
- **제안**: `Super::ExecuteTask` 결과가 `InProgress`일 때만 억제를 켠다(다른 결과에서는 억제를 아예 건드리지 않는다).
- **확신도**: 중간

### 3. 🟡 ActivateAbility: 재발동 어빌리티에서 태스크가 어빌리티보다 먼저 끝나고 종료 통지도 잃는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:53-59`, `:65-69`, 콜백 `:110-137`
- **범주**: 버그/정확성
- **문제**: `ActivatedHandle`을 `TryActivateAbility` **호출 전에** 세워 두고, `HandleAbilityEnded`는 스펙 핸들만으로 "내 실행의 종료"를 판별한다. 그런데 엔진은 `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 이미 활성인 채로 재발동되면 **기존 인스턴스를 `EndAbility(bWasCancelled=false)`로 먼저 끝낸 뒤** 재활성화한다(`AbilitySystemComponent_Abilities.cpp:1830-1851`). 이때 같은 핸들로 콜백이 들어와 `CleanUp()`이 구독을 끊고 `ActivationResult = Succeeded`가 채워진다. 이어 재활성화가 성공하면 `:65-69`가 그 낡은 결과를 그대로 반환해 태스크를 즉시 종료한다 — 어빌리티는 계속 도는데 BT는 다음 행동으로 넘어가고(공격 모션 위에 이동·배회가 겹침), `ActivatedHandle`도 비어 있어 `AbortTask`가 취소할 수도 없다. 같은 이유로, 첫 후보가 이 경로로 실패한 뒤 루프가 다음 후보로 넘어가면 구독이 이미 끊긴 상태로 발동하게 된다.
- **제안**: `bIsActivating` 구간의 콜백은 결과만 기록하고 `CleanUp()`(구독 해제)은 미룬다. 또는 `:65-69`의 `ActivationResult` 신뢰를 `:71-84`의 "핸들 유효 + 스펙 활성" 확인 뒤로 미뤄, 실제로 도는 어빌리티가 있으면 InProgress를 유지하게 한다.
- **확신도**: 중간(엔진 경로는 확인했고, 발현은 "발동 시점에 이미 활성"인 상황에 달렸다)

### 4. 🟡 Patrol의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:46-49`, `:63`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask`는 하드코딩된 `WxBlackboardKeys::PatrolTargetLocation`에 목표를 쓰지만, 실제 이동은 `Super::ExecuteTask`가 `UBTTask_BlackboardBase::BlackboardKey`(엔진에서 `EditAnywhere`, Vector/Object 필터만 걸림)를 읽어 수행한다. 생성자 `:19`는 기본값을 맞춰 둔 것뿐이라, 디자이너가 BT 에디터에서 키를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation`에 쓰고 엉뚱한 키(대개 `0,0,0`이나 stale 값)로 이동한다. 경고 하나 없이 폰이 월드 원점으로 걸어가는 형태로만 드러난다.
- **제안**: 쓰기도 `BlackboardKey`를 경유해 읽는 키와 통일하거나, 반대로 `BlackboardKey` 편집을 잠근다. `UWxBTTask_ReturnHome`은 읽기만 하므로 동일 위험이 없다.
- **확신도**: 높음(메커니즘) / 중간(실제 에셋 설정에 달림)

### 5. 🟡 Once 정찰 완료 후 매 BT 틱 즉시 Succeeded를 반환해 전체 재탐색이 상시화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:41-44`
- **범주**: 성능/안전
- **문제**: `bPatrolFinished`가 서면 `ExecuteTask`가 이동도 지연도 없이 동기 `Succeeded`를 반환한다. 엔진은 즉시 끝난 태스크에 대해 실행 갱신을 예약하고 다음 틱에 루트부터 재탐색하므로, 이 폰은 살아 있는 내내 **매 프레임 트리 전체 검색**을 돈다(형제 데코레이터 평가 비용이 전부 따라붙는다). 프레임 내 무한 루프는 아니지만 Once 정찰 적이 많은 맵에서는 그대로 누적된다.
- **제안**: 완료 상태에서는 `InProgress`를 반환해 실제로 브랜치를 점유하게 한다. 주석이 말하는 "그 자리에 머문다"의 정확한 표현이며, 상위 우선순위 abort는 그대로 동작한다.
- **확신도**: 중간

### 6. 🟢 BeyondLeash의 "Self/Both 금지"가 주석으로만 있고 에디터에서 막히지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:11-19`, 경고 주석 `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h:21-22`
- **범주**: 설계/구조
- **문제**: 헤더는 Self/Both를 고르면 "복귀가 경계에서 끊기고 경계 왕복이 난다"고 명시하는데, 생성자는 `FlowAbortMode`에 기본값만 넣을 뿐 잘못된 선택지를 막지 않는다. 엔진은 바로 이 목적의 플래그(`bAllowAbortNone`·`bAllowAbortLowerPri`·`bAllowAbortChildNodes`, `BTDecorator.h:70-77`)로 디테일 패널 드롭다운을 제한할 수 있는데 쓰지 않았다. 잘못 고르면 재현·진단이 어려운 왕복 버그로만 드러난다.
- **제안**: 생성자에서 `bAllowAbortNone = false; bAllowAbortChildNodes = false;`를 세워 LowerPriority 외 선택 자체를 없앤다.
- **확신도**: 중간(의도적으로 여지를 남긴 것일 수 있음)

### 7. 🟢 `EWxTeam`이 WxAI에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:9`
- **범주**: 설계/구조
- **문제**: 저장소 전체 소비자는 `Source/WxGame/Character/WxCharacterBase.h:11`·`:120`, `WxCharacterBase.cpp:168`·`:180`, `WxEnemyCharacter.cpp:21`, `WxPlayerCharacter.cpp:22`뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다(퍼셉션의 진영 판정은 엔진 `IGenericTeamAgentInterface`를 그대로 쓴다). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이라, 다른 도메인(예: WxCombat의 피아 필터)이 필요해지는 순간 "WxCore 외 플러그인 참조 금지" 규칙을 어기지 않고는 쓸 수 없다.
- **제안**: `WxTeamTypes.h`를 `WxCore`로 옮긴다. 소비자가 4파일뿐인 지금이 비용이 가장 낮다.
- **확신도**: 중간

### 8. 🟢 Patrol과 Wander가 감속 GameplayEffect 적용·해제 로직을 통째로 중복한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:51-61`·`:80-85` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:55-65`·`:107-112` (선언부도 `WxBTTask_Patrol.h:31-42`·`:55-56` ↔ `WxBTTask_Wander.h:54-65`·`:74-75`)
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`/`MoveSpeedEffect` UPROPERTY, `MoveSpeedEffectHandle` 멤버, spec 생성 + `SetByCaller_MoveSpeedScale` 주입 + 적용 블록, `OnTaskFinished`의 제거 블록이 두 파일에 사실상 문자 단위로 동일하다. GE 규약이 바뀔 때(SetByCaller 태그 변경, 스택 정책 추가 등) 한쪽만 고치면 정찰과 배회의 속도 거동이 조용히 갈린다.
- **제안**: 베이스가 `UBTTask_MoveTo`/`UBTTaskNode`로 갈려 상속 통합은 부자연스럽고, 프로젝트가 얕은 중복을 용인하는 편이므로 **당장 구조를 바꾸자는 뜻은 아니다**. 다만 두 곳이 짝이라는 사실을 기억해 GE 규약을 건드릴 때 반드시 함께 고친다.
- **확신도**: 낮음(의도된 중복 허용일 수 있음)

### 9. 🟢 Blackboard accessor의 키 검증이 매 프레임 폴링 경로에서 반복된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:15-33`, 호출부 `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:36`
- **범주**: 성능/안전
- **문제**: `VerifyBlackboardKey`는 호출마다 `GetKeyID`(에셋 키 배열 선형 탐색)와 `GetKeyType`을 돈다. `UWxBTDecorator_BeyondLeash::TickNode`는 관찰 중 매 프레임 `CalculateRawConditionValue` → `GetHomeLocation`을 부르므로 Development/Editor 빌드에서 `AI 수 × 프레임`만큼 같은 검증이 반복된다(Shipping에서는 통째로 비어 있다). 키가 5개뿐이라 절대 비용은 작지만, "에셋 설정 오류를 드러낸다"는 목적에 매 호출 반복은 불필요하고 Development 프로파일을 왜곡한다.
- **제안**: 검증 결과를 Blackboard 에셋 단위로 1회만 수행하도록 캐시하거나, 엔진 관례대로 `FBlackboardKeySelector`로 KeyID를 캐시해 이름 조회 자체를 없앤다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Public/` 헤더 15개 전부, 소비자 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **검증 과정에서 기각한 가설**: (a) `FWxBTRandomChoiceMemory`가 `FBTCompositeMemory`와 메모리를 겹칠 가능성 — 엔진에서 `FBTCompositeMemory`는 특수 메모리가 아니라 인스턴스 메모리이고(`BTCompositeNode.cpp:702-705`), 베이스의 placement-new가 앞 2바이트만 덮으므로 `LastChosenChild`는 안전. `GetNextChildHandler`의 `GetActiveInstanceIdx()` 사용도 엔진 `FindChildToExecute`(`BTCompositeNode.cpp:46`)와 동일. (b) `FWxBeyondLeashMemory::bWasBeyond` 미초기화 — 노드 메모리는 0 초기화되고 `INIT_DECORATOR_NODE_NOTIFY_FLAGS()`가 `bNotifyBecomeRelevant`를 세워 `OnBecomeRelevant`가 항상 먼저 시드한다. (c) `PostInitProperties`의 `ConfigureSense` 중복 등록 — 엔진이 같은 클래스면 교체하므로(`AIPerceptionComponent.cpp:122-139`) 중복되지 않는다. (d) `UWxBTComposite_RandomChoice`가 `UBTComposite_Selector`를 상속해 Selector 시멘틱이 새어 나올 가능성 — UE 5.8 Selector는 `GetNextChildHandler`와 에디터 아이콘만 있는 껍데기라 무해. (e) `UWxBTTask_ActivateAbility`의 range-for 중 스펙 배열 재할당 — `FScopedAbilityListLock`이 `GiveAbility`/`ClearAbility`를 지연시키므로 순회는 안전하고, 락 해제 후 재조회하는 현재 구조가 옳다.
- **미검토 / 한계**: BT/Blackboard 에셋의 실제 노드 배치(`UWxBTDecorator_BeyondLeash`의 FlowAbortMode 실제 값, Patrol의 `BlackboardKey` 변경 여부)는 에셋 영역이라 확인하지 않았다 — 발견 4·6의 발현은 여기에 달려 있다. 다수 AI 동시 구동 프로파일링과 실제 멀티플레이 환경에서의 `State.InCombat` MinimalReplication 타이밍도 측정하지 않았다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 29파일 — `/module-review`로 갱신*
