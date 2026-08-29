# WxCombat — 코드 리뷰

> `UWxCombatLibrary::ApplyDamage`부터 `UWxExecCalc_Damage`의 최종 산식, `IncomingDamage`의 HP·사망·피격 전이까지 현재 대미지 경로를 재검토했다. 무기·투사체·Finisher·Backstab의 호출 경계와 예측/서버 권위도 대조했으며, 이번 범위에서 즉시 수정할 수 있는 재현 가능한 문제는 확인하지 못했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 0 |

## 결과

현재 검토 범위에서는 기록할 실행 가능한 이슈가 없다.

`CalculateFinalDamage`는 방어 보정·기본 피해·치명타·일반 가드 배율을 순서대로 합성하고, 퍼펙트 가드는 치명타·일반 가드 배율을 제외한 반사량 경로로 분리한다. 이후 `IncomingDamage`를 마지막 출력으로 적용해 GP/가드 브레이크 전이를 먼저 처리하며, HP 감소·`Event.Death`·피격 이벤트의 순서도 일관된다.

기존 리뷰의 “사망 타격 뒤 `AdditionalEffects` 적용”은 사망 후 부가 효과를 허용한다는 결정에 따라 이슈로 재등록하지 않았다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Backstab.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnProjectile.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/`, `Plugins/WxCombat/Source/WxCombat/Public/Weapon/`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Action/`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/`의 나머지 대미지·행동 경계 구현과 대응 공개 헤더
- **미검토 / 한계**: BP/WBP·몬타주·DataTable 자산의 실제 행 구성, 네트워크 지연·패킷 손실 환경의 런타임 재현, 엔진 내부 GAS의 예측 키 처리 구현은 검토하지 않았다.

---
*문서 기준 커밋 `38bb5476` · 리뷰일 2026-08-29 · 소스 160파일 — `/module-review`로 갱신*
