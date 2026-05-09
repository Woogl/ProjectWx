// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxMMC_LinearDrain.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxMMC_LinearDrain::UWxMMC_LinearDrain()
{
	MaxDPCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxDPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	MaxUPCaptureDef = FGameplayEffectAttributeCaptureDefinition(
		UWxCombatAttributeSet::GetMaxUPAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	RelevantAttributesToCapture.Add(MaxDPCaptureDef);
	RelevantAttributesToCapture.Add(MaxUPCaptureDef);
}

float UWxMMC_LinearDrain::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	if (!Spec.Def)
	{
		return 0.f;
	}

	if (!ensureMsgf(Spec.Def->Modifiers.Num() == 1, TEXT("UWxMMC_LinearDrain 은 Modifier 1개를 전제합니다 (현재 %d개)."), Spec.Def->Modifiers.Num()))
	{
		return 0.f;
	}

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	const FGameplayAttribute& TargetAttribute = Spec.Def->Modifiers[0].Attribute;

	float MaxAttributeValue = 0.f;
	if (TargetAttribute == UWxCombatAttributeSet::GetDPAttribute())
	{
		GetCapturedAttributeMagnitude(MaxDPCaptureDef, Spec, EvalParams, MaxAttributeValue);
	}
	else if (TargetAttribute == UWxCombatAttributeSet::GetUPAttribute())
	{
		GetCapturedAttributeMagnitude(MaxUPCaptureDef, Spec, EvalParams, MaxAttributeValue);
	}
	else
	{
		return 0.f;
	}

	const float Duration = Spec.GetDuration();
	if (Duration <= 0.f)
	{
		return 0.f;
	}

	const float Period = Spec.GetPeriod();
	return -(MaxAttributeValue / Duration) * Period;
}
