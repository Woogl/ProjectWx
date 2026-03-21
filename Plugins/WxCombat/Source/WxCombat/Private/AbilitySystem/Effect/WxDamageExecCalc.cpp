// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxDamageExecCalc.h"
#include "AbilitySystem/WxAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WxGameplayTags.h"
#include "Perception/AISense_Damage.h"

struct FWxDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxAttributeSet, IncomingDamage, Target, false);
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

	// FinalDamage = ATK_공격자 * (100 / (100 + DEF_피격자))
	const float DamageReduction = 100.f / (100.f + TargetDEF);
	float FinalDamage = FMath::Max(SourceATK * DamageReduction, 0.f);

	// 가드 중이면 데미지 50% 감소
	if (TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Guard))
	{
		constexpr float GuardDamageReductionRate = 0.5f;
		FinalDamage *= GuardDamageReductionRate;
	}

	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, FinalDamage));

		// 전투 피격 후처리 (환경 데미지와 구분하기 위해 DamageExecCalc에서 처리)
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
}
