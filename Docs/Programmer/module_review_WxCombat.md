# WxCombat — 코드 리뷰

> GAS 전투 경로와 모듈 경계는 전반적으로 잘 정리되어 있고, 확인한 범위에서는 `AGENTS.md`의 Copyright·접두사·콜백 명명·`BlueprintCallable`·플러그인 의존성 규칙 위반이 없었다. 이번 리뷰는 README를 출발점으로 Build 설정, 공개 계약, 대미지·어빌리티·무기·락온·시간 조작의 위험 경로를 중심으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🔴 락온 대상 Server RPC가 클라이언트 값을 무검증으로 권위 상태에 반영한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:38`
- **범주**: 설계/구조
- **문제**: `ServerSetLockOnTarget_Implementation`은 호출자가 넘긴 `USceneComponent`를 사거리·적대 팀·`UWxLockOnPointComponent`·생존/락온 가능 상태를 검증하지 않고 그대로 복제 상태에 저장한다. 악의적 또는 변조된 소유 클라이언트는 자신이 네트워크 참조를 가진 임의 컴포넌트를 보내 서버의 락온 대상으로 만들 수 있으며, 이 값은 투사체 호밍과 모션 워핑에서 서버 권위 소비처가 사용한다. 정상 경로의 `UWxAbility_LockOn` 로컬 후보 검증만으로는 RPC 직접 호출을 막지 못한다.
- **제안**: 서버에서 null 해제만 예외로 허용하고, 대상 컴포넌트의 타입·소유 액터·적대성·거리·`CanBeLockedOn()`을 재검증한 뒤 반영한다. 검증 규칙을 컴포넌트 또는 공용 서버 측 선택 함수로 모아 락온 어빌리티와 동일한 기준을 사용한다.
- **확신도**: 높음

### 2. 🟡 상태 GE 종료가 적용 인스턴스를 식별하지 못한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:31`
- **범주**: 버그/정확성
- **문제**: 노티파이는 시작 시 적용된 `FActiveGameplayEffectHandle`을 보관하지 않고, 종료 때 `EffectClass`와 null source object만으로 한 건을 제거한다. 같은 효과 정의가 겹쳐 적용되면(예: `UWxEffect_Invincible`을 회피 노티파이와 컷신 태스크가 동시에 적용) 종료한 노티파이의 효과가 아닌 다른 인스턴스가 먼저 제거될 수 있다. 이후 종료 순서에 따라 무적이 조기 해제되거나 반대로 남는다.
- **제안**: 적용 API가 핸들을 반환하게 하고, 노티파이 실행별 `(MeshComp, NotifyEvent)` 상태 또는 별도 런타임 객체에 보관한 정확한 핸들만 제거한다. 같은 패턴의 `UWxAbilityTask_PlaySkillCutscene`도 이 식별 방식을 함께 사용한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Public/` 헤더 전반과 나머지 `AbilitySystem/Ability/`, `AbilitySystem/Effect/`, `AbilitySystem/Cue/`, `AnimNotify/`, `Targeting/`, `Damage/`의 대응 cpp
- **미검토 / 한계**: BP/WBP·몬타주·DataTable 자산 내부와 실제 멀티플레이 런타임은 검토하지 않았다. 대미지 수치식과 타게팅 프리셋의 밸런스 타당성은 기획 범위로 보류했다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 152파일 — `/module-review`로 갱신*
