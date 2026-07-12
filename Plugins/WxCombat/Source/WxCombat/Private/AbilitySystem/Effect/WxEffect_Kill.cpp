// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Kill.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_Kill::UWxEffect_Kill()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Target의 현재 HP를 그대로 IncomingDamage로 넣어 확정 처치한다.
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetIncomingDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FAttributeBasedFloat AttributeBased;
	AttributeBased.Coefficient = FScalableFloat(1.f);
	AttributeBased.BackingAttribute.AttributeToCapture = UWxCombatAttributeSet::GetHPAttribute();
	AttributeBased.BackingAttribute.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	AttributeBased.BackingAttribute.bSnapshot = false;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);

	Modifiers.Add(Modifier);

	// 대미지 플로터. 표준 대미지(WxExecCalc_Damage)와 동일한 GameplayCue_Damage를 발행한다.
	// MagnitudeAttribute=IncomingDamage로 두면, 위 모디파이어가 IncomingDamage에 넣은 값(= 깎인 HP)이 큐 RawMagnitude로 실린다.
	FGameplayEffectCue DamageCue;
	DamageCue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Damage);
	DamageCue.MagnitudeAttribute = UWxCombatAttributeSet::GetIncomingDamageAttribute();
	GameplayCues.Add(DamageCue);
}
