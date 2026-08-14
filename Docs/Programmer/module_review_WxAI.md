# WxAI — 코드 리뷰

> 전반적으로 건강한 모듈이다. 플러그인 의존은 `WxCore`만이고, 프로젝트 코딩 규칙(Copyright 헤더·`Wx` prefix·`Handle` 콜백 prefix·`BlueprintCallable`/`FORCEINLINE`/람다 금지) 위반이 한 건도 없으며, 수명 관리가 까다로운 퍼셉션·GAS 연동 지점은 주석과 방어 코드가 잘 깔려 있다. 이번 리뷰는 29개 소스 전부를 헤더 수준에서 훑고, 퍼셉션 컴포넌트·BT Task/Decorator/Composite의 cpp 구현을 엔진 소스(UE 5.8 `BehaviorTreeComponent`·`AbilitySystemComponent_Abilities`)와 대조해 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 Patrol의 이동 목표 키가 쓰는 쪽·읽는 쪽으로 이원화되어 있다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:19`, `:47-50`, `:64`
- **범주**: 버그/정확성
- **문제**: `ExecuteTask`는 `WxBlackboardKeys::SetPatrolTargetLocation`으로 **하드코딩된** `PatrolTargetLocation` 키에 목표를 쓰지만, 실제 이동은 `Super::ExecuteTask`가 `UBTTask_BlackboardBase::BlackboardKey`(엔진에서 `UPROPERTY(EditAnywhere, Category=Blackboard)`, 확인함)를 읽어 수행한다. 생성자에서 기본값을 맞춰 두었을 뿐이라, 디자이너가 BT 에디터에서 `BlackboardKey`를 다른 Vector 키로 바꾸면 태스크는 `PatrolTargetLocation`에 쓰고 엉뚱한 키(대개 0,0,0 또는 stale 값)로 이동한다. 경고 하나 없이 폰이 월드 원점 쪽으로 걸어가는 형태로 드러난다.
- **제안**: 쓰기도 `BlackboardKey.SelectedKeyName`으로 통일하거나(같은 값을 쓰고 읽게), 반대로 `BlackboardKey`를 `VisibleAnywhere`/`meta=(EditCondition=false)`로 잠가 편집 불가로 만든다. `UWxBTTask_ReturnHome`은 읽기만 하므로 동일 위험이 없다.
- **확신도**: 높음

### 2. 🟡 Once 정찰 완료 후 태스크가 매 BT 틱 즉시 Succeeded를 반환해 트리 전체 재탐색이 상시화된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:42-45`
- **범주**: 성능/안전
- **문제**: `bPatrolFinished`가 서면 `ExecuteTask`가 이동 없이 동기 `Succeeded`를 반환한다. BT는 즉시 종료된 태스크에 대해 `ScheduleExecutionUpdate()`를 걸고 다음 틱에 `ProcessExecutionRequest()`로 루트부터 재탐색하므로(엔진 `BehaviorTreeComponent.cpp:1804-1812` 확인), 이 폰은 **살아 있는 내내 매 프레임 전체 트리 검색**을 돈다. 지연(latency)을 만드는 노드가 하나도 없는 브랜치가 영구 점유되는 구조다. 프레임 내 무한 루프는 아니지만, Once 정찰 적이 많은 맵에서 AI 틱 비용이 그대로 쌓인다. 덧붙여 `bPatrolFinished`는 한 번 서면 절대 풀리지 않아, 전투 후 복귀해도 정찰이 재개되지 않는다(주석상 의도된 동작).
- **제안**: 완료 상태에서는 `InProgress`를 반환하고 대기(또는 엔진 `Wait` 태스크를 브랜치에 두는 전제로 `Failed` 대신 명시적 idle 노드로 분리)하게 해, 완료된 정찰 브랜치가 매 틱 재탐색을 유발하지 않게 한다.
- **확신도**: 중간

### 3. 🟡 복귀 이동이 실패하면 타겟 억제가 매 탐색마다 켜졌다 꺼지며 진동한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp:29-35`, `:38-49`
- **범주**: 설계/구조
- **문제**: `ExecuteTask`는 `Super::ExecuteTask`(MoveTo)의 성공 여부를 알기 **전에** `SetTargetingSuppressed(true)`를 호출한다. HomeLocation이 내비메시 밖이거나 경로가 없어 MoveTo가 `Failed`를 반환하면 곧바로 `OnTaskFinished`가 억제를 풀고, 리시 데코는 여전히 참이므로 다음 탐색에서 같은 일이 반복된다. 억제 진입은 `SetTargetActor(nullptr)` → 포커스 해제 + CMC 회전 모드 원복 + `State.InCombat` 태그 제거(`WxAIPerceptionComponent.cpp:125-140`, `:200-244`)를 수반하므로, 이 진동은 서버 부하만이 아니라 **MinimalReplication 태그가 클라이언트로 add/remove를 반복**해 네임플레이트가 깜빡이는 형태로 관측된다.
- **제안**: `Super::ExecuteTask` 결과가 `InProgress`일 때만 억제를 켜고, 그 외 결과에서는 억제를 건드리지 않도록 순서를 뒤집는다.
- **확신도**: 중간

### 4. 🟡 Patrol과 Wander가 감속 GameplayEffect 적용·해제 로직을 통째로 중복한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:52-62`·`:82-86` ↔ `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:56-65`·`:108-112` (선언부도 `WxBTTask_Patrol.h:33-44`·`:57-58` ↔ `WxBTTask_Wander.h:56-67`·`:76-77`)
- **범주**: 중복/복잡도
- **문제**: `MoveSpeedMultiplier`/`MoveSpeedEffect` UPROPERTY 2개, `MoveSpeedEffectHandle` 멤버, spec 생성 + `SetByCaller_MoveSpeedScale` 주입 + 적용 코드 블록, `OnTaskFinished`의 제거 코드가 두 파일에 문자 단위로 동일하게 복제돼 있다. `GetStaticDescription`의 "감속 GE 미지정" 분기까지 같다. GE 적용 규약(예: SetByCaller 태그 변경, 스택 정책 추가)이 바뀌면 두 곳을 동시에 고쳐야 하고, 한쪽만 고치면 정찰과 배회의 속도 거동이 갈린다.
- **제안**: 두 태스크의 공통 베이스(예: `UWxBTTask_MoveSpeedScaled`)로 프로퍼티·핸들·적용/해제를 올리거나, 최소한 적용/해제 두 함수만 공용 헬퍼로 뽑는다. 다만 베이스가 각각 `UBTTask_MoveTo`/`UBTTaskNode`로 갈리므로 상속 통합보다는 헬퍼 추출이 자연스럽다.
- **확신도**: 높음

### 5. 🟢 ActivateAbility: 발동 구간 중 도착한 종료 통지가 재발동(retrigger) 케이스에서 태스크를 조기 종료시킨다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:41-70`
- **범주**: 버그/정확성
- **문제**: 엔진 `InternalTryActivateAbility`는 `InstancedPerActor` + `bRetriggerInstancedAbility` 어빌리티가 **이미 활성인 채로** 재발동될 때 기존 인스턴스를 `EndAbility`로 끝낸 뒤 그대로 재활성화한다(`AbilitySystemComponent_Abilities.cpp:1832-1852` 확인). 이때 `HandleAbilityEnded`가 같은 핸들로 들어와 `CleanUp()`으로 **구독을 끊고** `ActivationResult`를 채우므로, 재활성화가 성공해도 `ExecuteTask`는 `:67-70`에서 그 결과를 그대로 반환해 태스크를 끝내 버린다. 결과적으로 어빌리티는 계속 도는데 BT는 종료 처리하고, 이후 종료 통지도 못 받는다. `bRetriggerInstancedAbility` 기본값이 false라 디자이너가 명시적으로 켠 어빌리티에서만 발생한다.
- **제안**: 루프 종료 후 `ActivationResult`를 신뢰하기 전에 `ActivatedHandle`이 여전히 유효하고 스펙이 활성인지 먼저 확인하도록 `:67-70`과 `:72-85`의 순서를 바꾸거나, `HandleAbilityEnded`가 `bIsActivating` 구간에서는 구독 해제(`CleanUp`)를 미루게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 재발동 어빌리티를 안 쓰기로 정했다면 무해하다)

### 6. 🟢 `EWxTeam`이 WxAI에 정의되어 있으나 WxAI 안에서 전혀 쓰이지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h:10`
- **범주**: 설계/구조
- **문제**: 저장소 전체에서 `EWxTeam` 소비자는 `Source/WxGame/Character/WxCharacterBase.{h,cpp}`, `WxEnemyCharacter.cpp`, `WxPlayerCharacter.cpp`뿐이고 WxAI 코드는 이 타입을 한 번도 참조하지 않는다(퍼셉션의 진영 판정은 엔진 `IGenericTeamAgentInterface`를 그대로 쓴다). 팀 구분은 AI 전용이 아니라 캐릭터 공통 개념이고, 모듈 표상 `WxCore`가 "공용 정의" 자리인데 AI 플러그인이 소유하고 있다. 지금은 WxGame만 쓰므로 의존 규칙 위반은 아니지만, 다른 도메인(예: WxCombat의 타겟 필터)이 이 enum이 필요해지는 순간 규칙 위반 없이는 참조할 수 없다.
- **제안**: `WxTeamTypes.h`를 `WxCore`로 옮긴다. 지금 옮기면 소비자가 3파일뿐이라 비용이 가장 낮다.
- **확신도**: 중간

### 7. 🟢 Blackboard accessor의 키 검증이 매 프레임 폴링 경로에서 반복 실행된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp:15-33`, 호출부 `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp:37`
- **범주**: 성능/안전
- **문제**: `VerifyBlackboardKey`는 호출마다 `GetKeyID`(에셋 키 배열 선형 탐색) + `GetKeyType`을 돈다. `UWxBTDecorator_BeyondLeash::TickNode`는 관찰 중 **매 프레임** `GetHomeLocation`을 부르므로, Development/Editor 빌드에서 AI 수 × 프레임만큼 같은 검증이 반복된다(Shipping에서는 비어 있음). 키가 5개뿐이라 절대 비용은 작지만, 검증의 목적이 "에셋 설정 오류를 드러내는 것"이라면 매 호출 반복은 필요 없는 형태다.
- **제안**: 검증을 최초 1회(또는 Blackboard 에셋별 1회)만 수행하도록 캐시하거나, 엔진 관례대로 `FBlackboardKeySelector`로 KeyID를 캐시해 이름 조회 자체를 없앤다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Public/` 헤더 15개 전부, 소비자 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`
- **검증 과정에서 기각한 가설**: (a) `FWxBeyondLeashMemory::bWasBeyond` 미초기화 — BT 노드 메모리는 엔진이 `AddZeroed`로 0 초기화(`BehaviorTreeTypes.cpp:215`)하고 `OnBecomeRelevant`가 시드하므로 안전. (b) `UWxBTComposite_RandomChoice`가 `UBTComposite_Selector`를 상속해 핸들러가 무시될 가능성 — 엔진 Selector는 `OnNextChild`를 바인드하지 않고 순수 가상 디스패치라 오버라이드가 정상 동작. (c) `EWxWanderDirection`에 `meta=(Bitflags)` 누락 — UE 5.8 프로퍼티 에디터는 프로퍼티 쪽 `Bitmask`/`BitmaskEnum`만 보므로 문제 없음. (d) Damage 센스에 진영 필터가 없어 아군을 타겟팅할 가능성 — `WxEffect_Damage.cpp:174-185`가 비적대 대미지를 원천 차단해 아군 피격 자체가 성립하지 않음.
- **미검토 / 한계**: BT/Blackboard 에셋의 실제 노드 배치(특히 `UWxBTDecorator_BeyondLeash`의 FlowAbortMode가 실제로 Lower Priority로 저작됐는지, Patrol 태스크의 `BlackboardKey`가 기본값 그대로인지)는 에셋 영역이라 확인하지 않았다 — 1번·3번 발견의 실제 발현 여부는 에셋 설정에 달려 있다. 멀티플레이 실환경에서의 `State.InCombat` MinimalReplication 타이밍, 다수 AI 동시 구동 시의 실측 프로파일링도 하지 않았다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 29파일 — `/module-review`로 갱신*
