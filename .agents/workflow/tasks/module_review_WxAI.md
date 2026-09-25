# WxAI — 코드 리뷰

상태: 확인 대기 · 코드 리뷰 결과 판단 대기
다음 행동: 기존 수용 조건을 유지하며, 관찰 대상에 InstancedPerExecution을 도입할 때 1번을 재검토한다.

> 인지·타겟 선정, 락온 수명, 어빌리티 발동의 동기 종료·중단·재발동, 도플갱어 미러링과 이동 태스크의 정리 경로에서 새 결함은 확인하지 못했다. 사용자가 수용한 `InstancedPerExecution` 관찰 누락 1건을 재확인했으며, 현재 C++ 38파일 중 핵심 실행 경로를 중심으로 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 `UWxBTDecorator_ObserveAbility`가 InstancedPerExecution 어빌리티의 첫 발동을 놓친다

- **위치**: `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp:52`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp:109`
- **범주**: 버그/정확성
- **문제**: 발동 통지에서 `ConditionalFlowAbort`가 조건을 즉시 평가한다. UE 5.8의 `GameplayAbility.cpp:997`은 `NotifyAbilityActivated`를 `:1012`의 `Spec->ActiveCount++`보다 먼저 호출한다. 이 Decorator는 PerActor 외 정책을 `Spec.IsActive()`로 판정하므로, 기존 실행이 없는 PerExecution 스펙은 발동 통지 시점에 거짓이다. 이후 ActiveCount 증가에 대한 재평가 통지가 없어 해당 발동을 계기로 한 분기 전환을 놓친다.
- **제안**: 현재는 조치하지 않는다. 관찰 대상에 PerExecution 어빌리티를 도입하면 인스턴스들의 `IsActive()`를 검사하도록 정책 분기를 보완하고, 첫 발동과 동시 실행·종료에 대한 BT 관찰자 중단을 검증한다.
- **확신도**: 높음 — 엔진 호출 순서와 `FGameplayAbilitySpec::IsActive()`의 ActiveCount 판정을 정적으로 확인했다.
- **판단**: 수용(2026-09-23 사용자) — "InstancedPerExecution인 어빌리티를 안 쓰기 때문". [당시 판단 원문](../../../.wiki/raw/notes/2026-09-23-wxai-review-followups.md)을 보존한다.
- **재개 조건**: 관찰 대상 어빌리티의 인스턴싱 정책을 InstancedPerExecution으로 바꾸거나 해당 정책의 신규 어빌리티를 도입할 때 재검토한다. 이번 리뷰는 기존 수용 범위를 변경하지 않는다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ReturnHome.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivateAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_ObserveAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_MirrorMovement.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Patrol.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTTask_Wander.cpp` 및 각 공개 헤더.
- **훑은 파일**: 나머지 WxAI 소스(`WxBlackboardKeys`, `WxBTService_UpdateTargetDistance`, `WxBTDecorator_RandomWeight`, `WxBTDecorator_AttributeRatio`, `WxPatrolComponent`, `WxAnimNotify_ReportNoise`, `WxAIModule`), `Plugins/WxAI/Source/WxAI/WxAI.Build.cs`, `Plugins/WxAI/WxAI.uplugin`. 엔진은 UE 5.8의 `GameplayAbility.cpp`, `GameplayAbilityTypes.cpp`, `BTDecorator.cpp`에서 발동 통지·활성 판정·즉시 조건 평가를 대조했다.
- **미검토 / 한계**: 기준은 작업 시작 커밋 `ad0db6de0`과 현재 WxAI 작업 트리다. 다른 모듈의 구현은 이번 리뷰 범위에 포함하지 않았다. BT/BP 바이너리 내부·서비스 배치·Observer aborts·GA/GE/캐릭터 구성 지정값은 직접 검증하지 않았다. 빌드·자동화 테스트·PIE·멀티플레이 실행 검증은 하지 않았다. 과거 인게임 수용과 테스트 기록을 이번 버전의 실행 검증으로 간주하지 않는다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 기존 지적 재대조 | ObserveAbility와 엔진 발동 통지·조건 평가 순서 확인 | AI | 통과 | PerExecution의 Spec.IsActive 판정과 ActiveCount 증가 전 통지 확인 |
| 현재 실행 검증 | 빌드·PIE·멀티플레이 실행 | AI | 미실행 | 정적 코드 리뷰 범위 |
| 리뷰 결과 판단 | 기존 수용 및 PerExecution 도입 시 재검토 조건 확인 | 사람 | 대기 | 2026-09-23 판단을 변경하지 않음 |

---
*문서 기준 커밋 `ad0db6de0` · 리뷰일 2026-09-26 · 소스 38파일 — `/module-review`로 갱신*
