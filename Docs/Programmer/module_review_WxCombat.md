# WxCombat — 코드 리뷰

> 대미지의 단일 진입점, `UWxEffect_Damage` 실행 계산, 메타 어트리뷰트의 HP/사망 전이, GameplayCue와 근접·투사체·처형 호출 경계를 추적했다. 권위 적용은 투사체와 처형 경로에서 명시적으로 분리되어 있고, `IncomingDamage`→HP→사망 이벤트 순서도 일관되지만, 치명타로 사망한 대상에 부가 GE가 새는 경로가 남아 있다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 치명타로 사망한 대상에도 `AdditionalEffects`가 적용된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:137`
- **범주**: 버그/정확성
- **문제**: `ApplyDamage`는 `UWxEffect_Damage`를 먼저 동기 적용한 뒤, 성공 여부와 퍼펙트 가드만 보고 같은 행의 `AdditionalEffects`를 순회한다. 그 사이 `UWxCombatAttributeSet::PostGameplayEffectExecute`가 `IncomingDamage`를 HP에 반영하고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:102`), HP가 0 이하이면 `Event.Death`를 즉시 발행한다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:114`). 그러나 부가 GE 적용 전에는 사망 태그/HP를 다시 검사하지 않으며, `UWxEffect_Damage`에만 `Ability.Death` 대상 제외 조건이 있다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:24`). 따라서 피해 행에 지속 디버프·상태이상 등 `AdditionalEffects`가 있고 해당 한 방이 대상을 죽이면, 사망 전이 후에도 그 효과가 시체 ASC에 적용된다. 효과 자체가 사망 제외 조건을 갖지 않으면 태그·주기 처리·큐가 남을 수 있다.
- **재현 시나리오**: HP가 작은 적에게 `AdditionalEffects`로 지속 GE를 가진 대미지 행을 적중시킨다. 첫 `UWxEffect_Damage`가 HP를 0으로 만들고 사망 어빌리티를 발동한 다음, 같은 `ApplyDamage` 호출이 그 지속 GE를 대상에게 적용한다.
- **제안**: `UWxEffect_Damage` 적용 직후 대상이 사망 태그를 얻었거나 HP가 0 이하이면 부가 GE 루프를 건너뛴다. 사망 뒤에도 허용해야 하는 효과가 있다면 행 수준의 명시적 플래그로 예외를 선언하고, 각 지속 GE에는 `Ability.Death` 대상 제외 조건을 기본으로 둔다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxCombatEffectContext.h`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Backstab.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_PerfectGuard.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/`, `AbilitySystem/Effect/`, `AbilitySystem/Cue/`, `AnimNotify/`, `Weapon/`의 나머지 대미지 연관 구현과 대응 공개 헤더
- **미검토 / 한계**: BP/WBP·몬타주·DataTable 자산의 실제 행 구성과 멀티플레이 런타임은 검토하지 않았다. `Source/WxGame`은 치트·캐릭터 맥락만 읽었으며 대미지 파이프라인의 직접 호출자는 찾지 못했다.

---
*문서 기준 커밋 `973c9c07` · 리뷰일 2026-08-29 · 소스 152파일 — `/module-review`로 갱신*
