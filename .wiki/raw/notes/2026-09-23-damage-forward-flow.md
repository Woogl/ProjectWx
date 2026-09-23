---
title: "Damage 결과를 앞으로만 흘리는 구조"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, damage]
summary: "Hit Wrapper GE와 전용 EffectContext를 없애고, ApplyDamage 판정 → Damage GE → DamageResponse 반응으로 결과가 앞으로만 흐르게 했다."
---

# Damage 정방향 흐름

사용자가 FWxDamageRequest를 제거하고 `ApplyDamage(Causer, Target, DamageTableRow, HitResult)`로 되돌린 뒤, 결과가 앞으로만 흐르는 재설계안을 승인했다(2026-09-23, "구조가 단순화되고 직관적이 되는 것을 선호").

- 진단: 반응이 결과를 모르는 Hit GE의 OnGameplayEffectApplied에 있어 Damage GE 실행 결과를 Context로 거꾸로 올려야 했다. 이 역방향 통로가 FWxHitEffectContext·Result 수치·결과 태그 사본·Reset/Duplicate/NetSerialize 규칙을 만들었다.
- ApplyDamage: 출처·레벨 추론, 권위·적대·사망 확인(사망 대상엔 회피 이벤트도 없음), 무적이면 DodgeSuccess 후 종료, 방어 판정 1회, 판정을 Damage Spec 동적 태그 `Damage.Guarded`/`Damage.PerfectGuarded`로 부착, 퍼펙트 가드가 아니면 추가 효과 Spec을 피해 전에 생성, Damage GE 적용, 성공 시 추가 효과 적용.
- ExecCalc는 Spec 태그로 가드/퍼펙트 가드를 읽는다. 퍼펙트 가드면 IncomingReflect만 출력한다.
- DamageResponse(OnGameplayEffectExecuted)는 실행 기록을 가진 자리에서 플로터 → Hit Cue(피해>0 또는 퍼펙트 가드) → 가드 취소·피격·가해 → 퍼펙트 가드 이벤트·반사 GP·패리·Cue를 낸다.
- 삭제: UWxEffect_Hit, UWxEffectComponent_Hit, FWxHitEffectContext. Context는 엔진 기본형이다. FWxDamageResult는 bApplied/bEvaded/bGuarded/bPerfectGuard만 남았다. Hit Cue의 AggregatedTargetTags 전달은 소비자가 없어(GC_Hit은 데이터 전용 BP) 제거했다.
- 엔진 근거(UE 5.8): ExecuteActiveEffectsFrom은 모디파이어·AttributeSet 훅 뒤 OnExecuted를 호출한다(GameplayEffect.cpp:3369). ApplyGameplayEffectSpecToSelf는 함수 전체에 활성 GE 잠금을 건다. 조건부 GE(ConditionalGameplayEffects)는 OnExecuted보다 먼저 적용되고 GE 정의 단위라 행별 추가 효과를 대체하지 못한다.
- 동작 변화: 추가 효과와 DodgeSuccess가 대상 활성 GE 잠금 밖에서 실행된다. 반응이 Damage GE의 OnApplied·적용 델리게이트보다 먼저 실행된다(구독자 없음). 사망/그로기 발행은 AttributeSet에 유지한다(치트·AddGP 경로 공유).

근거: [진입점](../../../Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp), [반응](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp), [계산](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp). 빌드·회귀 결과는 Workflow Task에 기록한다.

## 추가: 회피를 엔진 Immunity 통지로 전환 (2026-09-23)

사용자 지시: "OnImmunityBlockGameplayEffectDelegate 를 쓰는 방식으로 재구현합시다." ApplyDamage의 무적 분기와 Event.DodgeSuccess 발행·태그를 제거했다. Dodge 어빌리티가 활성 동안 ASC의 OnImmunityBlockGameplayEffectDelegate를 구독하고, 차단된 Spec에 Damage.Attack이 있으면 극한 회피로 전환한다(EndAbility에서 해제). 방송 위치는 ImmunityGameplayEffectComponent.cpp:67이며 ApplyGameplayEffectSpecToSelf에서 CanApply(사망 TargetTagRequirements)보다 먼저 돈다. FWxDamageResult.bEvaded를 제거하고 투사체가 적용 전에 무적 태그로 통과를 정한다. 무적은 항상 UWxEffect_Invincible(Immunity 포함)로 부여된다(Dodge ANS·처형·스킬 컷신).

## 추가: 결과 반환 제거와 Damage GE 컴포넌트 분할 (2026-09-23)

사용자 판단: ApplyDamage 인자 추가는 원치 않음, Damage Effect 내부 분할 제안에 "B로 진행". DamageResponse를 Damage GE의 컴포넌트 셋(생성자 추가 순서 = 실행 순서)으로 나눴다: `UWxEffectComponent_DamageReaction`(플로터·Hit Cue·가드 취소·피격·가해), `_PerfectGuard`(이벤트·반사 GP·패리·Cue·투사체 되돌림), `_HitStop`(EffectCauser를 무기/투사체로 캐스트해 기존 InstigatorHitStop/VictimHitStop 적용). 별도 내부 GE로 쪼개지 않은 이유: 새 적용마다 Spec 복사·중첩이 생기고 Damage GE 실행 기록을 읽지 못한다. FWxDamageResult를 삭제하고 ApplyDamage는 void를 반환한다. 무기는 호출 한 줄, 투사체는 적용 전 무적 조회(통과)와 적용 후 Owner 변화(되돌림) 확인으로 파괴를 정한다. 히트스톱 값은 모두 C++ 기본값이었다(Content 덮어쓰기 없음: 무기 0.1/0.1, 투사체 0/0.1). 동작 변화: 히트스톱이 추가 효과보다 먼저, 투사체 되돌림이 퍼펙트 가드 반응 안에서 일어난다. 공격별 히트스톱이 필요해지면 피해 행+SetByCaller로 옮기는 안(A)을 검토한다.

## 추가: 적용 여부 반환 (2026-09-23)

사용자 제안("반환값으로 처리하면 되지 않나요?")으로 `ApplyDamage`가 Damage GE 적용 여부(bool)를 반환한다. 판정·수치는 싣지 않는다. 테스트는 적용 관찰 람다 대신 반환값과 자원 변화로 검증하고, 반사량 0 퍼펙트 가드만 이벤트 관찰로 확인한다.

## 추가: 규칙을 Damage GE 안으로 (2026-09-23)

사용자 판단: 사망 확인은 Effect 내부로, 적대는 ApplyDamage 유지(A), 방어 판정 ExecCalc 이관과 추가 효과 Effect 이관(B안 전체) 승인. ApplyDamage의 사망 확인 제거(Death 어빌리티가 Ability.*를 취소해 Dodge 비활성). 적대를 GE CanApply로 옮기는 시도는 Immunity 쿼리가 CanApply보다 먼저 돌아 아군 공격에도 회피 통지가 나가므로 되돌렸다(현재 콘텐츠는 모든 호출 경로가 사전 적대 필터를 가져 실위험은 낮음). 방어 판정은 ExecCalc가 대상 태그로 내리고 결과 태그를 붙인다. 추가 효과는 MakeDamageSpec이 Spec을 미리 만들어 입력 전용 FWxDamageEffectContext(서버 로컬, Duplicate 시 비움)에 싣고, 마지막 컴포넌트 _AdditionalEffects가 퍼펙트 가드가 아니면 적용한다. 엔진 UAdditionalEffectsGameplayEffectComponent는 GE 클래스 단위 정적 목록·출처 태그 조건만 지원하고 OnGameplayEffectApplied에서 Spec을 만들어(반응 뒤 캡처) 행별 목록·퍼펙트 가드 생략을 대체하지 못한다.

## 추가: 추가 효과 Spec은 반응 뒤에 생성 (2026-09-23)

사용자 지시: "반응이 끝난 뒤에 만들어야해요." 추가 효과의 출처 태그 캡처 시점을 피해 전에서 반응 뒤로 바꿨다. Context는 Spec 대신 GE 클래스 목록만 싣고, `_AdditionalEffects`가 실행 시점에 같은 Context로 Spec을 만들어 적용한다(순환 참조 우려와 Context 사본 사용이 사라짐). 테스트의 캡처 시점 검사를 반응 뒤 기준으로 뒤집었다.

## 추가: Damage.Attack 제거와 자동화 테스트 삭제 (2026-09-23)

Damage.Attack은 Damage GE에 항상 붙어 의미가 없어 삭제했다. 반응은 피해량>0으로, Dodge는 막힌 Spec의 Def가 UWxEffect_Damage인지로 판정한다(다른 Immunity 통지 배제). 사용자 지시로 Wx.Combat.Damage.Result 자동화 테스트를 삭제했다. 투사체 회피 조건의 중복 적대 판정을 제거했다.
