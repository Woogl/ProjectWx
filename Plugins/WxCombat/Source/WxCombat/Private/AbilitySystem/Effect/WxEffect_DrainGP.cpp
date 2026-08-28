// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_DrainGP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "WxGameplayTags.h"

UWxEffect_DrainGP::UWxEffect_DrainGP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = WxGameplayTags::SetByCaller_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	Period = FScalableFloat(DrainPeriod);
	bExecutePeriodicEffectOnApplication = true;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UWxMMC_DrainGP::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(1.0f);

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxCombatAttributeSet::GetGPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);
	Modifiers.Add(Modifier);
}

UWxMMC_DrainGP::UWxMMC_DrainGP()
{
	MaxGPCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxGPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	RelevantAttributesToCapture.Add(MaxGPCaptureDef);
}

float UWxMMC_DrainGP::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const float Duration = Spec.GetDuration();
	if (Duration <= 0.f)
	{
		return 0.f;
	}

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxGP = 0.f;
	GetCapturedAttributeMagnitude(MaxGPCaptureDef, Spec, EvalParams, MaxGP);

	return -(MaxGP / Duration) * Spec.GetPeriod();
}
