// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxBleedExecCalc.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

struct FWxBleedStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DP);

	FWxBleedStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DP, Target, false);
	}
};

static const FWxBleedStatics& GetBleedStatics()
{
	static FWxBleedStatics BleedStatics;
	return BleedStatics;
}

UWxBleedExecCalc::UWxBleedExecCalc()
{
	const FWxBleedStatics& Statics = GetBleedStatics();
	RelevantAttributesToCapture.Add(Statics.ATKDef);
	RelevantAttributesToCapture.Add(Statics.DEFDef);
}

void UWxBleedExecCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Invincible))
	{
		return;
	}

	const FWxBleedStatics& Statics = GetBleedStatics();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = ExecutionParams.GetOwningSpec().CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = ExecutionParams.GetOwningSpec().CapturedTargetTags.GetAggregatedTags();

	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	// TickDamage = (ATK * (100 / (100 + DEF))) / NumTicks
	const float TotalDamage = FMath::Max(SourceATK * (100.f / (100.f + TargetDEF)), 0.f);
	const float TickDamage = TotalDamage / static_cast<float>(NumTicks);

	if (TickDamage <= 0.f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, TickDamage));

	if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, TickDamage));
	}
}
