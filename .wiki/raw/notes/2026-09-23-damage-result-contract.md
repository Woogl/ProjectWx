---
title: "피해 결과 반환과 투사체 소비 계약"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage, static-review]
summary: "DamageResult의 적용·방어·거부·계산량 계약과 투사체의 서버 재판정 제거를 코드에서 확인했다."
revision: 47b7f8bd7
---

# 피해 결과 반환과 투사체 소비 계약

2026-09-23 HEAD `47b7f8bd7` 및 미커밋 작업 트리의 정적 조사다. 아래 해시는 조사한 코드 버전을 식별한다. 빌드·자동화 실행 근거는 Workflow Task에서 별도로 관리한다. 이 원자료는 실제 투사체 궤적이나 멀티플레이 연출 검증을 뜻하지 않는다.

## 확인한 계약

- `UWxCombatLibrary::ApplyDamageWithResult`가 값 타입 `FWxDamageResult`를 반환한다. 기존 BlueprintCallable `ApplyDamage`는 같은 처리를 호출하고 `bApplied`만 돌려주는 호환 래퍼다.
- `bApplied`는 자식 Damage GE 적용 성공이다. `Defense`는 None/Evaded/Guarded/PerfectGuard이며, 회피는 bApplied=false, Rejection=None이다. 0 피해와 0 반사량도 적용 성공일 수 있다.
- `Rejection`은 잘못된 액터, ASC 부재, 비권위, 피해 정의 부재, Spec 생성 실패, Hit Wrapper 거부, 자식 Damage 거부의 단계를 구분한다. HitRejected는 비적대·사망·적용 요건 거부 등을 세분하지 않는다.
- DamageMagnitude/ReflectMagnitude는 실행 기록이다. 실제 HP/SP/GP 순변화량은 이번 API에 포함하지 않는다. bCritical/bGuardBreak도 계산 결과이며, 후속 Ability 활성화 성공을 뜻하지 않는다.
- Hit 컴포넌트는 방어 결과를 이벤트 전 로컬 값으로 확보한다. 후속 이벤트와 추가 GE 처리가 끝나면 원래 타격의 결과를 Context에 다시 기록한다. FWxHitEffectContext의 Result는 동기 반환 브리지이며 Duplicate/역직렬화 시 초기화된다.
- 투사체 서버는 반환된 Defense로 회피 통과를 결정한다. bApplied와 PerfectGuard, 자체 bCanReflect 및 Pawn 여부로 반사를 결정한다. 서버 경로의 무적·퍼펙트 가드 재조회와 DamageRow 가드 가능 여부 재조회는 제거했다.
- 클라이언트 ImpactFX는 기존 로컬 회피 조회를 유지한다. 서버 Overlap ImpactFX는 확정 결과를 얻은 뒤 재생하므로 피해 처리 뒤로 이동한다. 내부 피해 계산·자원 반영·이벤트·추가 효과 순서는 유지한다.

## 코드 근거

모든 경로는 `Plugins/WxCombat/Source/WxCombat/` 아래다.

| 파일 | 확인 범위 | SHA-256 |
|---|---|---|
| [WxDamageResult.h](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageResult.h) | 결과 타입 전체 | `522caca45d2c586300e50cef065b2b15f87a33165260eba9fe8047203870b87f` |
| [WxCombatLibrary.h](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h) | 공개 API 계약 | `ac958009dd4d154f801720bfa4e34eb195deb85033d0df03830e4cb29e1bfc71` |
| [WxCombatLibrary.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp) | ApplyDamage / ApplyDamageWithResult | `b4c9789ea2abef485b28ed80fef902c1495bd8abd77807d19b7dfb4f37e5b705` |
| [WxHitEffectContext.h](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxHitEffectContext.h) | Result 브리지 | `0cd86105e062b77d5b04753e844797270460311c093b61e11241bc2cb824251d` |
| [WxHitEffectContext.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp) | Duplicate / NetSerialize / ResetDamageResult | `1d7350e8d32f379e0ce307893ea118178c4ec3f3a420ac9e7a3c1006244d86ff` |
| [WxEffectComponent_Hit.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp) | OnGameplayEffectApplied | `ce254b18ddd13953751a5bfe37a2763cf4e9e69f50c51e51aabd45ce42aa7ac1` |
| [WxProjectileBase.cpp](../../../Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp) | HandleHitCollisionOverlap | `c825ce8f78788ff2b267a4cea7639d6c48b948e542c977e67dd6b8408a9eb8b6` |
