// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxDamageExecCalc.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WxGameplayTags.h"
#include "Perception/AISense_Damage.h"

struct FWxDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDMG);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
	}
};

static const FWxDamageStatics& GetDamageStatics()
{
	static FWxDamageStatics DamageStatics;
	return DamageStatics;
}

UWxDamageExecCalc::UWxDamageExecCalc()
{
	const FWxDamageStatics& Statics = GetDamageStatics();
	RelevantAttributesToCapture.Add(Statics.ATKDef);
	RelevantAttributesToCapture.Add(Statics.DEFDef);
	RelevantAttributesToCapture.Add(Statics.CritRateDef);
	RelevantAttributesToCapture.Add(Statics.CritDMGDef);
}

void UWxDamageExecCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// 타겟이 무적 상태이면 대미지를 적용하지 않음
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Invincible))
	{
		return;
	}

	const FWxDamageStatics& Statics = GetDamageStatics();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = ExecutionParams.GetOwningSpec().CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = ExecutionParams.GetOwningSpec().CapturedTargetTags.GetAggregatedTags();

	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	float SourceCritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritRateDef, EvalParams, SourceCritRate);

	float SourceCritDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDMGDef, EvalParams, SourceCritDMG);

	// FinalDamage = ATK_공격자 * (100 / (100 + DEF_피격자))
	const float DamageReduction = 100.f / (100.f + TargetDEF);
	float FinalDamage = FMath::Max(SourceATK * DamageReduction, 0.f);

	// 치명타 판정: CritRate 1당 1% 확률, 치명타 시 (1 + CritDMG * 0.01) 배율 적용
	const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
	if (FMath::FRand() < CritChance)
	{
		FinalDamage *= (1.f + SourceCritDMG * 0.01f);
	}

	// 가드 중이면 데미지 50% 감소
	if (TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Guard))
	{
		constexpr float GuardDamageReductionRate = 0.5f;
		FinalDamage *= GuardDamageReductionRate;
	}

	if (FinalDamage <= 0.f)
	{
		return;
	}
	
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, FinalDamage));
    
	// 전투 피격 후처리
	AActor* TargetActor = TargetASC->GetOwnerActor();
	AActor* SourceActor = ExecutionParams.GetOwningSpec().GetEffectContext().GetInstigator();
	if (TargetActor)
	{
		// AI 데미지 감지
		if (SourceActor)
		{
			UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, FinalDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
		}
    
		// HitReact 이벤트 발송
		FGameplayEventData EventData;
		EventData.Instigator = ExecutionParams.GetOwningSpec().GetEffectContext().GetOriginalInstigator();
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_HitReact, EventData);
	}
}
