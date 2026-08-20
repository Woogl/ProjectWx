// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Burn.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Burn::UWxEffect_Burn()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(UWxExecCalc_Burn::BurnDuration));

	Period = FScalableFloat(UWxExecCalc_Burn::BurnPeriod);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxExecCalc_Burn::StaticClass();
	Executions.Add(ExecDef);

	// TODO: StackingType 직접 대입은 5.7에서 deprecated이나 SetStackingType이 WITH_EDITOR 전용이라 런타임에서 링크가 깨진다.
	// 런타임 setter가 생기면 교체할 것.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 0;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;

	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Burn);
	GameplayCues.Add(Cue);

	// MagnitudeAttribute를 걸면 그 틱에 실제로 적용된 양이 RawMagnitude로 실려 온다.
	// bRequireModifierSuccessToTriggerCues(기본값)가 참이라, ExecCalc이 팀 판정·무적으로 조기 반환한 틱에는 아예 나가지 않는다.
	FGameplayEffectCue FloaterCue;
	FloaterCue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_DamageFloater);
	FloaterCue.MagnitudeAttribute = UWxCombatAttributeSet::GetIncomingDamageAttribute();
	GameplayCues.Add(FloaterCue);
}

struct FWxBurnStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

	FWxBurnStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
	}
};

static const FWxBurnStatics& GetBurnStatics()
{
	static FWxBurnStatics BurnStatics;
	return BurnStatics;
}

UWxExecCalc_Burn::UWxExecCalc_Burn()
{
	const FWxBurnStatics& Statics = GetBurnStatics();
	RelevantAttributesToCapture.Add(Statics.ATKDef);
	RelevantAttributesToCapture.Add(Statics.DEFDef);
}

void UWxExecCalc_Burn::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	// 화상은 대미지 GE의 AdditionalEffects로 붙으므로, 대미지 쪽만 막으면 아군이 지속 피해를 계속 받는다.
	// 시전자가 사라진 뒤의 틱은 아바타가 없어 통과하므로 화상이 시전자 사망으로 멈추지는 않는다.
	if (UWxExecCalc_Damage::CheckDamage(ExecutionParams.GetSourceAbilitySystemComponent(), TargetASC) != EWxDamageResult::Damaged)
	{
		return;
	}

	const FWxBurnStatics& Statics = GetBurnStatics();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = ExecutionParams.GetOwningSpec().CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = ExecutionParams.GetOwningSpec().CapturedTargetTags.GetAggregatedTags();

	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	const float TotalDamage = FMath::Max(SourceATK * (100.f / (100.f + TargetDEF)), 0.f);
	const float TickDamage = TotalDamage / static_cast<float>(NumTicks);

	if (TickDamage <= 0.f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, TickDamage));
}
