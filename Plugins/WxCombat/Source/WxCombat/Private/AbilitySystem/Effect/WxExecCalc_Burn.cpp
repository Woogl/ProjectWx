// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Burn.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

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
	
	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
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
	
	FGameplayCueParameters CueParams;
	CueParams.RawMagnitude = TickDamage;
	if (const AActor* AvatarActor = TargetASC->GetAvatarActor())
	{
		CueParams.Location = AvatarActor->GetActorLocation();
	}
	CueParams.EffectContext = ExecutionParams.GetOwningSpec().GetEffectContext();
	TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
}
