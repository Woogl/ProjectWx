---
title: "DamageResult를 직접 읽는 bool 필드로 평탄화"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "방어/거부 enum을 제거하고 적용·회피·일반 가드·퍼펙트 가드 여부를 직접 제공한다."
---

# DamageResult 평탄화

사용자가 EWxDamageDefense/EWxDamageRejection의 복잡성을 지적하며 평탄화를 요청했고 bool 필드 전환에 동의했다.

- 두 enum과 Defense/Rejection 필드를 제거했다. FWxDamageResult는 bApplied, bEvaded, bGuarded, bPerfectGuard, DamageMagnitude, ReflectMagnitude, bHasReflect, bCritical, bGuardBreak를 제공한다.
- bApplied는 여전히 자식 Damage GE의 적용 성공이며 양수 피해 여부가 아니다. 회피는 bApplied=false와 bEvaded=true다. 일반적인 요청 거부는 bApplied=false이며 상세 거부 원인을 결과에 담지 않는다.
- bGuarded는 일반 가드만 의미한다. bPerfectGuard와 동시에 true가 되지 않으며, 회피 시 두 플래그 모두 false다. 방어 판정 이후 자식 GE가 거부되면 판정 플래그는 유지되고 bApplied=false다.
- 투사체는 bEvaded로 통과/충돌 연출을, bApplied && bPerfectGuard와 기존 투사체 설정으로 반사를 결정한다. 0 반사 기록은 기존 bHasReflect가 유지한다.
- C++ 테스트의 enum 검사는 직접 플래그와 판정 배타성 검사로 바꿨다. 요청 검증과 피해 계산 순서는 그대로다.
- Content uasset의 enum/API 이름 문자열 검색에서는 참조를 찾지 못했다. 이 검색이 모든 Blueprint 실행 검증을 대신하지는 않는다. 외부 사용처는 제거된 Defense/Rejection 핀을 새 bool 필드로 바꿔야 한다.

근거: [Result](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageResult.h), [Hit](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp), [투사체](../../../Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp), [진입점](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp). 빌드와 테스트는 Workflow Task에서 기록한다.
