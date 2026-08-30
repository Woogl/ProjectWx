# WxAI — 코드 리뷰

> 지각·BT 노드·정찰 경로의 책임 분리가 명확하고 의존성도 `WxCore` 하나에 머무는, 전반적으로 건강한 모듈이다. 규칙 위반(prefix·Handle 콜백·람다·인라인 정의·`BlueprintCallable`·저작권 헤더)은 한 건도 없다. 이번 리뷰는 Public 헤더 15개·Private cpp 14개를 훑고 퍼셉션 수명주기, 어빌리티 발동 Task, 리시 복귀 분업, 가중 무작위 Composite, 정찰/배회 이동을 깊게 봤으며 소비자인 `AWxEnemyController`까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 4 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 빙의 전환 때 이전 폰의 전투 상태와 타겟이 정리되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:255`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged`는 `BindPawnHit(NewPawn)` 한 줄뿐이다. `SetRecognized`가 `GetOwnerPawn()`(현재 폰)의 ASC에만 태그를 쓰므로(`:128`), 언빙의/재빙의 시 이전 폰 ASC에 발행한 `State.InCombat`은 제거되지 않고 남는다. Blackboard `TargetActor`·`AppliedTarget`·컨트롤러 포커스·`bUseControllerDesiredRotation` 변경도 그대로 유지돼, 새 폰은 이전 타겟을 향한 상태로 시작한다. 소비자인 `AWxEnemyController::OnUnPossess`도 `SelfActor`만 비우므로(`Source/WxGame/Controller/WxEnemyController.cpp:56`) 이 정리를 대신하지 않는다.
- **제안**: `OldPawn`이 유효하면 그 ASC에서 `State.InCombat`을 명시적으로 제거하고, `SetTargetActor(nullptr)`로 타겟·포커스·회전 모드·소실 구독을 함께 되돌린 뒤 새 폰을 바인드한다.
- **확신도**: 높음

### 2. 🟡 어빌리티 취소가 거부돼도 BT 중단이 즉시 완료된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:96`
- **범주**: 버그/정확성
- **문제**: `AbortTask`가 `CleanUp()`으로 종료 델리게이트를 먼저 해제한 뒤 `CancelAbilityHandle`을 부르고 곧바로 `Super::AbortTask`로 중단을 확정한다(`:102`~`:109`). `CanBeCanceled()`가 거짓이거나 취소가 scope lock으로 지연된 어빌리티는 실제로는 계속 실행되는데 BT만 다음 브랜치로 넘어간다. 이때 남은 몽타주·히트박스·GameplayEffect 위에 새 행동이 겹치고, 구독을 이미 끊었으므로 실제 종료 시점도 추적하지 못한다.
- **제안**: 취소 직후 `FindAbilitySpecFromHandle`로 spec이 여전히 `IsActive()`인지 확인한다. 활성이면 구독을 유지하고 `EBTNodeResult::InProgress`(지연 중단)를 반환해 종료 콜백에서 `FinishLatentAbort`를 호출한다. 취소 불가 어빌리티를 BT에 매달지 않는 것이 저작 규약이라면 최소한 런타임 경고를 남긴다.
- **확신도**: 중간

### 3. 🟡 감지 자극이 올 때마다 현재 타겟을 무조건 교체한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:103`
- **범주**: 설계/구조
- **문제**: `HandleTargetPerceptionUpdated`는 성공 자극이면 살아있는지만 확인하고 `SetTargetActor(Actor)`로 덮어쓴다. 우선순위 판단이 없어, A와 교전 중에 B가 시야 가장자리(최대 1500cm)에 들어오거나 멀리서 소음(`HearingRange` 1000cm)을 내기만 해도 타겟·포커스가 B로 넘어가 근접전을 중단하고 이탈한다. 같은 클래스가 억제 해제 경로에서는 "가장 가까운 적"을 고르며 그 이유를 주석으로 남겨 두었으므로(`:181`), 두 경로의 선택 정책이 서로 어긋나 있다.
- **제안**: 교체 정책을 한 곳에 모은다. 예를 들어 현재 타겟이 유효한 동안에는 Damage 자극이거나 거리가 뚜렷하게 가까울 때만 교체하고, 그 외에는 인식 갱신만 하도록 게이트를 둔다. "최신 자극 우선"이 의도라면 억제 해제 경로도 같은 규칙으로 맞추고 헤더 doc에 명시한다.
- **확신도**: 중간

### 4. 🟡 AttributeRatio 데코레이터는 실행 중 조건 변화를 관찰할 수 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:11`
- **범주**: 설계/구조
- **문제**: 생성자가 `bAllowAbortLowerPri`·`bAllowAbortChildNodes`를 모두 끄므로 에디터에서 `FlowAbortMode`를 `None` 외의 값으로 지정할 수 없다. 이 데코레이터는 Blackboard 키를 관찰하지도, `UWxBTDecorator_BeyondLeash`처럼 `TickNode` 폴링을 하지도 않아 조건은 트리 탐색 시점에만 평가된다. 결과적으로 저체력 전환(도주·광폭화) 브랜치는 현재 공격 태스크가 끝날 때까지 시작되지 않는다.
- **제안**: 즉시 전환이 필요한 용도면 abort 허용 플래그를 열고 `BeyondLeash`와 같은 폴링 + `RequestExecution` 패턴을 쓴다. 태스크 경계 평가가 의도라면 그 제약을 헤더 doc과 `GetStaticDescription`에 드러내 BT 저작자가 오해하지 않게 한다.
- **확신도**: 중간

### 5. 🟢 Patrol과 Wander의 감속 GameplayEffect 수명 관리가 그대로 중복된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:54`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec` → `SetSetByCallerMagnitude(SetByCaller.MoveSpeedScale, ...)` → `ApplyGameplayEffectSpecToSelf` 부여와 `OnTaskFinished`의 `RemoveActiveGameplayEffect` + 핸들 리셋이 두 Task에 문자 그대로 같은 형태로 존재한다. `MoveSpeedMultiplier`/`MoveSpeedEffect` 프로퍼티 선언과 `GetStaticDescription`의 "감속 GE 미지정" 분기까지 중복이라, SetByCaller 태그나 제거 규약을 바꿀 때 한쪽만 고칠 위험이 있다.
- **제안**: 상속 계층을 넓히지 말고, 부여/해제를 담당하는 작은 내부 헬퍼(예: Private의 자유 함수 한 쌍)로 추출해 두 Task가 같은 코드를 공유하게 한다.
- **확신도**: 높음

### 6. 🟢 Wander는 내비게이션을 거치지 않고 원시 이동 입력만 넣는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:98`
- **범주**: 버그/정확성
- **문제**: 실행 시점에 정한 고정 방향으로 `AddMovementInput`만 호출할 뿐 내비메시 투영도, 목적지 유효성 검사도 없다. 오픈월드 지형에서는 배회가 캐릭터를 낭떠러지 아래나 내비메시 밖으로 밀어낼 수 있고, 그 상태에서는 후속 `UWxBTTask_ReturnHome`의 `UBTTask_MoveTo`가 경로를 못 찾아 동기 실패하며(`WxBTTask_ReturnHome.cpp:24`) 억제도 켜지지 않아 복귀가 성립하지 않는다.
- **제안**: 이동 전에 `UNavigationSystemV1::ProjectPointToNavigation`(또는 `GetRandomReachablePointInRadius`)으로 목표 지점을 검증하고, 도달 불가 방향이면 다른 방향을 고르거나 `Failed`로 마감한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h`, 대응 Public 헤더 전체, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`, 소비자 대조용 `Source/WxGame/Controller/WxEnemyController.cpp`
- **미검토 / 한계**: Behavior Tree·Blackboard 에셋과 런타임 재현은 보지 않았다(샌드박스에 엔진 없음). 발견 2·4·6의 실제 체감은 각각 어빌리티의 취소 설정, BT 에셋의 분기 배치, 레벨 지형에 따라 달라진다. `UWxBTComposite_RandomChoice`가 사전 필터에서 `DoDecoratorsAllowExecution`을 부르고 엔진이 선택 직후 같은 검사를 다시 수행하는 이중 평가는, 부작용 있는 데코레이터를 붙이지 않는 한 문제가 되지 않아 발견으로 올리지 않았다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 29파일 — `/module-review`로 갱신*
