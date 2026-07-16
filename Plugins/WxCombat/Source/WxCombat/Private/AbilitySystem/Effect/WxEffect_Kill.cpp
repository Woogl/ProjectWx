// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Kill.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_Kill::UWxEffect_Kill()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Target의 현재 HP를 그대로 IncomingDamage로 넣어 확정 처치한다.
	// 처형(즉사) 연출이므로 대미지 수치 플로터(GameplayCue_Damage)는 발행하지 않는다.
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
}
