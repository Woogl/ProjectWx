# WxAI — 코드 리뷰

> 핵심 경로(타겟 선정 서비스, 리시 판정 Decorator와 복귀 Task 분리, 락온 서비스의 수명 짝, 어빌리티 발동 Task의 동기 종료·중단·재발동 처리, RandomChoice)는 엔진 수명주기까지 방어돼 있어 건강하다. 남은 발견은 사용자가 수용한 잠복 결함 1건뿐이다. 커버리지: WxAI 소스 46파일 전부를 읽었고, WxGame 컨트롤러·캐릭터 SPD 콜백·WxCombat 피격 이벤트 발신부와 UE 5.8 엔진 소스(GAS 발동 순서, BT 데코레이터 abort, 퍼셉션 망각)를 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 `UWxBTDecorator_ObserveAbility`가 InstancedPerExecution 어빌리티의 발동을 놓친다
- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp:50`
- **범주**: 버그/정확성
- **문제**: 발동 통지(`HandleAbilityActivated` → `ConditionalFlowAbort`, `:109`)는 조건을 즉시 평가하는데, 엔진은 `NotifyAbilityActivated`를 `Spec->ActiveCount++`보다 먼저 부른다(UE 5.8 `GameplayAbility.cpp:997`, `:1012`). 이 Decorator는 PerActor 외 정책을 `Spec.IsActive()`(ActiveCount 기반)로 보므로 PerExecution 어빌리티는 발동 순간 조건이 거짓이다.
- **제안**: 조치하지 않는다. 인스턴싱 정책을 PerExecution으로 바꾸는 어빌리티가 생기면 정책 분기(`:51`~`54`)를 `Spec.GetAbilityInstances()` 중 하나라도 `IsActive()`인지로 바꾼다.
- **확신도**: 중간
- **판단**: 수용(2026-09-23 사용자) — 프로젝트는 InstancedPerExecution 어빌리티를 쓰지 않는다.

## 검토 범위
- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp` 및 각 헤더
- **훑은 파일**: WxAI의 나머지 소스(`WxBTService_UpdateTargetDistance`, `WxBTDecorator_RandomWeight`, `WxBTDecorator_AttributeRatio`, `WxAnimNotify_ReportNoise`, `WxPatrolComponent`, `WxAIModule`, `Private/Tests/*`, `WxAI.Build.cs`), `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageReaction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_MoveSpeedScale.cpp`, UE 5.8 엔진 소스(`AIPerceptionComponent.cpp`, `BTDecorator.cpp`, `BehaviorTreeComponent.h`, `GameplayAbility.cpp`, `GameplayAbilityTypes.cpp`, `Controller.cpp`, `Pawn.cpp`)
- **미검토 / 한계**: BT/BP 에셋 내부는 `BT_Doppelganger`의 Mirror 노드 값 외에는 해석하지 않았다. UpdateTargetActor 서비스의 배치 위치, Observer aborts 값처럼 에셋 설정에 달린 동작은 단정하지 않았다. PIE·멀티플레이 실행 검증은 하지 않았다.

---
*문서 기준 커밋 `e72c9179f` · 리뷰일 2026-09-23 · 소스 46파일 — `/module-review`로 갱신*
