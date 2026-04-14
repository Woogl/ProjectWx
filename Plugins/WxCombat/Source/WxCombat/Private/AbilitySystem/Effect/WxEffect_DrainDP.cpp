// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_DrainDP.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_DrainDP::UWxEffect_DrainDP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Period = FScalableFloat(DrainPeriod);
	bExecutePeriodicEffectOnApplication = true;

	// 틱당 차감량 = MaxDP * -DrainPeriod
	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxDPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	AttributeBased.Coefficient = FScalableFloat(-DrainPeriod);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetDPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
	Modifiers.Add(Modifier);
}
