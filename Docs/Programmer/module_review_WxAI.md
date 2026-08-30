# WxAI — 코드 리뷰

> 지각·Behavior Tree 노드·정찰 경로의 책임 분리와 `WxCore`만 참조하는 모듈 경계는 건강하다. Public 헤더 15개와 Private cpp 14개를 모두 훑고, 타겟 수명·리시 복귀·어빌리티 Task·무작위 Composite·정찰/배회 상태 관리를 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 타겟 억제 중 성공 자극이 Blackboard와 포커스를 다시 채운다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:99`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:153`
- **범주**: 버그/정확성
- **문제**: `SetTargetingSuppressed(true)`는 현재 타겟을 비워 `AppliedTarget`도 비운다. 그러나 이후 `HandleTargetPerceptionUpdated`의 획득 조건에는 `bTargetingSuppressed` 검사가 없어, 복귀 중 대상이 시야에 재진입하거나 새 Hearing/Damage 자극이 오면 `SetTargetActor(Actor)`가 다시 실행된다. `UpdateRecognition`은 `State.InCombat`만 꺼 두므로 Blackboard `TargetActor`, AI 포커스, strafe 회전 모드는 억제 상태와 모순되게 복구된다.
- **제안**: 성공 자극의 타겟 획득 조건에 `!bTargetingSuppressed`를 추가한다. Perception 내부 자극은 그대로 유지해 억제 해제 시 현재 Sight를 스캔하는 기존 재획득 경로가 사용하게 한다.
- **확신도**: 높음

### 2. 🟡 빙의 전환 시 이전 폰의 `State.InCombat` 태그가 남는다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:124`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:251`
- **범주**: 버그/정확성
- **문제**: `HandlePossessedPawnChanged`는 전달받은 `OldPawn`을 사용하지 않고 `SetTargetActor(nullptr)` 뒤 `UpdateRecognition()`을 호출한다. 이 델리게이트 시점에는 컨트롤러의 Pawn이 이미 `NewPawn` 또는 null이므로, `SetRecognized(false)`가 조회하는 ASC도 이전 폰이 아니다. 따라서 이전 폰에 `AddMinimalReplicationGameplayTag`로 발행했던 `State.InCombat`은 언빙의·컨트롤러 재사용 후에도 남아 부활·풀링 시 잘못된 전투 상태로 이어질 수 있다.
- **제안**: `OldPawn`의 ASC에서 `State.InCombat`을 명시적으로 제거하는 헬퍼를 두고, 현재 타겟·포커스 정리와 새 폰 바인딩 전에 호출한다.
- **확신도**: 높음

### 3. 🟡 `AttributeRatio`는 실행 중 어트리뷰트 변화로 트리를 재평가할 수 없다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp:11`
- **범주**: 설계/구조
- **문제**: 생성자가 `bAllowAbortLowerPri`와 `bAllowAbortChildNodes`를 모두 끄고, Blackboard 관찰·Tick 폴링·ASC 변화 구독도 제공하지 않는다. 따라서 비율 조건은 트리 탐색 시점에만 평가되며, HP 임계치 기반 도주·광폭화 같은 브랜치는 현재 실행 중인 Task가 끝날 때까지 전환될 수 없다.
- **제안**: 즉시 반응이 필요한 노드라면 abort 모드를 허용하고 ASC 어트리뷰트 변화 구독 또는 제한 주기 폴링으로 `RequestExecution`을 호출한다. 스냅샷 판정이 의도라면 그 제약을 헤더와 노드 설명에 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 Patrol과 Wander가 감속 GameplayEffect 수명 코드를 중복한다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp:58`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp:53`
- **범주**: 중복/복잡도
- **문제**: 두 Task가 `MakeOutgoingSpec` → `SetSetByCallerMagnitude` → `ApplyGameplayEffectSpecToSelf` 적용과 `OnTaskFinished`의 제거·핸들 초기화를 같은 형태로 반복한다. SetByCaller 태그나 제거 정책이 바뀌면 두 구현이 어긋날 수 있다.
- **제안**: Private 범위의 작은 헬퍼로 감속 GE 적용과 제거를 모아 두 Task가 같은 수명 규약을 공유하게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h`, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivateAbility.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxPatrolComponent.cpp`
- **훑은 파일**: `Plugins/WxAI/README.md`, `Plugins/WxAI/WxAI.uplugin`, `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/Source/WxAI/Public/`의 나머지 헤더, `Plugins/WxAI/Source/WxAI/Private/WxAIModule.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAnimNotify_ReportNoise.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomWeight.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_TargetDistance.cpp`, 연계 확인용 `Source/WxGame/Controller/WxEnemyController.cpp`
- **미검토 / 한계**: Behavior Tree·Blackboard 에셋과 BP/WBP 내부 구조는 범위 밖이다. 로컬 UE 5.8 소스로 델리게이트·BT·GAS 수명 의미를 대조했지만 PIE 런타임 재현과 다중 클라이언트 검증은 수행하지 않았다. 접두사·Copyright 첫 줄·Handle 콜백·`BlueprintCallable`·람다·인라인 정의와 Wx 플러그인 의존 규칙에서는 위반을 찾지 못했다.

---
*문서 기준 커밋 `66c0f6fd` · 리뷰일 2026-08-30 · 소스 29파일 — `/module-review`로 갱신*
