// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_DrainDP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_DrainDP::UWxEffect_DrainDP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	Period = FScalableFloat(DrainPeriod);
	bExecutePeriodicEffectOnApplication = true;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UWxMMC_DrainDP::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(1.0f);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetDPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);
	Modifiers.Add(Modifier);
}

UWxMMC_DrainDP::UWxMMC_DrainDP()
{
	MaxDPCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxDPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	RelevantAttributesToCapture.Add(MaxDPCaptureDef);
}

float UWxMMC_DrainDP::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const float Duration = Spec.GetDuration();
	if (Duration <= 0.f)
	{
		return 0.f;
	}

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxDP = 0.f;
	GetCapturedAttributeMagnitude(MaxDPCaptureDef, Spec, EvalParams, MaxDP);

	return -(MaxDP / Duration) * Spec.GetPeriod();
}
