# WxAI — 코드 리뷰

> 전투 AI의 지각·BT·정찰 책임이 비교적 명확하게 나뉘어 있고, 플러그인 의존성도 `WxCore`에만 머문다. 이번 리뷰는 README와 Build 설정, Public 헤더 15개·Private cpp 14개를 확인하고 퍼셉션 수명주기와 지연 BT Task, 정찰·랜덤 선택 구현을 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 빙의 전환 때 이전 폰의 전투 상태와 타겟이 정리되지 않는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:255`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged`는 새 폰의 피격 이벤트만 다시 바인드한다. 기존 `TargetActor`·`AppliedTarget`·컨트롤러 포커스와 이전 폰 ASC에 발행한 `State.InCombat`은 유지된다. 이후 인식 갱신은 `GetOwnerPawn()`의 새 폰만 대상으로 태그를 제거하므로, 이전 폰에는 전투 태그가 남고 새 폰은 이전 타겟을 가진 상태로 시작할 수 있다. 실제 `AWxEnemyController`도 언빙의에서 `SelfActor`만 비우므로 이 정리를 대신하지 않는다.
- **제안**: 전환 시 `OldPawn`의 ASC에서 `State.InCombat`을 명시적으로 제거하고, 타겟·포커스·소실 구독을 해제한 뒤 새 폰의 이벤트를 바인드한다. 새 폰이 준비된 뒤에는 Blackboard 상태에 맞춰 인식을 다시 계산한다.
- **확신도**: 높음

### 2. 🟡 어빌리티 취소를 거부해도 BT 중단이 즉시 완료된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp:96`
- **범주**: 버그/정확성
- **문제**: `AbortTask`가 종료 델리게이트를 먼저 해제하고 `CancelAbilityHandle` 호출 뒤 즉시 상위 구현으로 중단을 완료한다. `CanBeCanceled()`이 거짓이거나 취소가 scope lock으로 지연된 어빌리티에서는 실제 어빌리티가 계속 실행되는데 BT만 다음 브랜치로 진행한다. 이때 기존 몽타주·히트박스·GameplayEffect가 남은 채 새 행동이 겹칠 수 있고, 델리게이트가 해제돼 실제 종료도 추적하지 못한다.
- **제안**: 취소 뒤 해당 spec이 여전히 활성인지 확인한다. 활성 상태면 종료 구독을 유지하고 지연 중단으로 전환해 종료 콜백에서 `FinishLatentAbort`를 호출한다. 취소 불가를 허용하지 않는 저작 규약이라면 런타임 경고도 추가한다.
- **확신도**: 중간

### 3. 🟡 AttributeRatio 데코레이터는 실행 중 조건 변화를 관찰할 수 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:11`
- **범주**: 설계/구조
- **문제**: `bAllowAbortLowerPri`와 `bAllowAbortChildNodes`를 모두 끄므로 에디터에서 `FlowAbortMode`를 `None` 외 값으로 설정할 수 없다. 이 데코레이터는 Blackboard 키를 관찰하거나 자체 폴링도 하지 않아, HP 비율이 임계값을 넘어도 현재 태스크가 끝나고 트리가 다시 탐색될 때까지 분기가 바뀌지 않는다. 긴 공격·경직 태스크 아래의 저체력 행동은 기대보다 늦게 시작된다.
- **제안**: 즉시 전환이 요구되는 용도라면 abort 허용과 값 변화 감지 방식을 제공한다. 태스크 경계에서만 평가하는 것이 의도라면 이 제약을 헤더와 노드 설명에 명시해 BT 저작자가 선택할 수 있게 한다.
- **확신도**: 중간

### 4. 🟢 Patrol과 Wander의 감속 GameplayEffect 수명 관리가 중복된다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`
- **범주**: 중복/복잡도
- **문제**: `MakeOutgoingSpec`·SetByCaller 속도 배율 설정·자기 자신에 대한 적용과 `OnTaskFinished`의 활성 Effect 제거가 `UWxBTTask_Wander`에도 같은 형태로 반복된다. 속도 태그나 제거 규약을 변경할 때 두 Task가 어긋날 위험이 있다.
- **제안**: 상속 구조를 넓히지 말고 Effect 부여와 핸들 해제를 담당하는 작은 내부 유틸로 추출해 두 Task가 공유하게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, 대응 Public 헤더, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/README.md`
- **미검토 / 한계**: Behavior Tree·Blackboard 에셋과 런타임 재현은 검토하지 않았다. 발견 2와 3의 실제 영향은 어빌리티 취소 설정 및 BT 에셋의 분기 배치에 따라 달라진다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 29파일 — `/module-review`로 갱신*
