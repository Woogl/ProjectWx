---
title: "0 피해 히트스톱 조건과 Hit Cue 발행 주석 정정"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "히트스톱을 Hit Cue와 같은 조건(피해 > 0 또는 퍼펙트 가드)으로 맞추고, Hit Cue가 예측 발행된다는 낡은 주석과 무적·무기 사전 검사 주석을 현재 구조에 맞게 정정했다. 빌드 통과, 플레이 미검증."
---

# 0 피해 히트스톱 조건과 주석 정정

사용자 지시(2026-09-23): "1, 2 적용해주세요". 다른 GAS 프로젝트(Lyra·Action RPG·GASDocumentation·Aura·Ninja Combat 등)와 비교한 개선 제안 중 1(낡은 주석 정정)과 2(0 피해 히트스톱)를 적용했다.

- 히트스톱 조건: `UWxEffectComponent_HitStop`은 Damage GE 실행 기록의 `IncomingDamage`가 0보다 크거나 Spec 동적 태그에 `Damage.PerfectGuarded`가 있을 때만 히트스톱을 건다. `UWxEffectComponent_DamageReaction`의 Hit Cue와 같은 조건이다.
- 이전 동작: ExecCalc가 `FinalDamage <= 0`으로 출력 없이 끝난 타격(반올림 0, 완전 경감 가드)은 플로터·Hit Cue·피격 이벤트·가드 SP 차감이 모두 빠졌는데 히트스톱만 걸렸다.
- 추가 효과: `UWxEffectComponent_AdditionalEffects`는 피해 없는 디버프 행을 위해 0 피해에도 적용을 유지한다(퍼펙트 가드만 생략).
- Hit Cue 발행: `WxCueNotify_Hit`·`WxCueNotify_DamageFloater` 헤더의 "공격자 클라에서 예측 발행" 서술은 사실이 아니었다. Damage GE에는 Cue 정의가 없고 `_DamageReaction`이 서버에서 빈 예측 키로 발행하므로, 공격자 클라도 서버 판정 뒤에 받는다.
- 무적 범위: `UWxEffect_Invincible`의 Immunity는 `UWxEffect_Damage` 클래스만 막고, 그 차단 통지(`OnImmunityBlockGameplayEffectDelegate`)를 Dodge가 극한 회피로 받는다. 추가 효과로 이미 걸린 지속 피해 GE나 치트 `UWxEffect_AddIncomingDamage`는 무적 중에도 들어간다.
- 무기 사전 검사: `AWxWeaponBase::ProcessHit`의 적대 사전 검사는 `ApplyDamage`의 적대 판정과 중복이다. 낡은 이유 주석("AdditionalEffects도 걸리면 안 되므로")만 삭제하고 검사는 유지했다.

근거: [히트스톱 컴포넌트](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_HitStop.cpp), [Hit Cue](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Cue/WxCueNotify_Hit.h), [플로터 Cue](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Cue/WxCueNotify_DamageFloater.h), [무적](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Invincible.cpp), [무기](../../../Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp).

검증: 전체 WxEditor 빌드 성공(경고 0), 로그 `Saved/Logs/BuildDoctor/build_2026-09-23_204153_209_33376.log`. 플레이 미검증. 작업 기록과 남은 기획 확인 항목은 [작업 자료](../../../.agents/workflow/tasks/damage-pipeline-structure-review.md)에 있다.
