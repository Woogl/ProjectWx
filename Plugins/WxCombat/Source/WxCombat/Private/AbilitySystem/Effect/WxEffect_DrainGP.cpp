// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_DrainGP.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"

UWxEffect_DrainGP::UWxEffect_DrainGP()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	Period = FScalableFloat(DrainPeriod);

	// 즉시 실행은 주기 눈금 밖에 놓여 총 실행 횟수를 하나 늘린다.
	bExecutePeriodicEffectOnApplication = false;

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
	const float Period = Spec.GetPeriod();
	if (Duration <= 0.f || Period <= 0.f)
	{
		return 0.f;
	}

	// 틱은 주기 배수에만 놓이므로, 자투리 시간까지 시간 비례로 나누면 마지막 틱에서도 GP가 남는다.
	const int32 TickCount = FMath::Max(1, FMath::FloorToInt(Duration / Period + UE_KINDA_SMALL_NUMBER));

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxGP = 0.f;
	GetCapturedAttributeMagnitude(MaxGPCaptureDef, Spec, EvalParams, MaxGP);

	// 마지막 틱이 정확히 0에 떨어지길 기대하면 누적 오차로 GP가 미세하게 남아 그로기가 풀리지 않는다.
	constexpr float TickOvershoot = 1.f + UE_KINDA_SMALL_NUMBER;

	return -MaxGP * TickOvershoot / TickCount;
}
