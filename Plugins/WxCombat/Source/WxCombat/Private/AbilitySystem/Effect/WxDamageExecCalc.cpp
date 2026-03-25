// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxDamageExecCalc.h"
#include "AbilitySystem/Effect/WxEffect_MPRecovery.h"
#include "AbilitySystem/Effect/WxEffect_Reflect.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
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
	DECLARE_ATTRIBUTE_CAPTUREDEF(DP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxDP);

	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DP, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, MaxDP, Target, false);
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
	RelevantAttributesToCapture.Add(Statics.DPDef);
	RelevantAttributesToCapture.Add(Statics.MaxDPDef);
}

void UWxDamageExecCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// 타겟이 무적 상태이면 대미지를 적용하지 않음
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
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

	// 퍼펙트 가드: 대미지 무효화 + 공격자에게 DP 반사
	if (TargetASC && TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_PerfectGuard))
	{
		UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
		if (SourceASC)
		{
			const float Reflect = FMath::Max(SourceATK * (100.f / (100.f + TargetDEF)), 0.f);

			// 공격자에게 DP 반사 적용
			const UGameplayEffect* ReflectEffect = UWxEffect_Reflect::StaticClass()->GetDefaultObject<UGameplayEffect>();
			FGameplayEffectSpec Spec(ReflectEffect, SourceASC->MakeEffectContext(), 1.f);
			Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_ReflectDP, Reflect);
			SourceASC->ApplyGameplayEffectSpecToSelf(Spec);

			// 공격자 그로기 판정
			if (!SourceASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
			{
				const UWxCombatAttributeSet* SourceAttrSet = SourceASC->GetSet<UWxCombatAttributeSet>();
				if (SourceAttrSet && SourceAttrSet->GetMaxDP() > 0.f && SourceAttrSet->GetDP() >= SourceAttrSet->GetMaxDP())
				{
					SourceASC->AddLooseGameplayTag(WxGameplayTags::State_Groggy);
				}
			}
		}

		return;
	}

	float SourceCritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritRateDef, EvalParams, SourceCritRate);

	float SourceCritDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDMGDef, EvalParams, SourceCritDMG);

	// FinalDamage = ATK_공격자 * (100 / (100 + DEF_피격자))
	const float DamageReduction = 100.f / (100.f + TargetDEF);
	float FinalDamage = FMath::Max(SourceATK * DamageReduction, 0.f);

	// 치명타 판정: CritRate 1당 1% 확률, 치명타 시 (1 + CritDMG * 0.01) 배율 적용
	const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
	const bool bIsCritical = FMath::FRand() < CritChance;
	if (bIsCritical)
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

	// 그로기 상태가 아닐 때만 DP 증가 및 그로기 판정
	if (TargetASC && !TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, FinalDamage));

		float TargetDP = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DPDef, EvalParams, TargetDP);

		float TargetMaxDP = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.MaxDPDef, EvalParams, TargetMaxDP);

		if (TargetMaxDP > 0.f && TargetDP + FinalDamage >= TargetMaxDP)
		{
			TargetASC->AddLooseGameplayTag(WxGameplayTags::State_Groggy);
		}
	}

	// 데미지 플로터 GameplayCue 실행
	{
		AActor* TargetActor = TargetASC->GetOwnerActor();
		FGameplayCueParameters CueParams;
		CueParams.RawMagnitude = FinalDamage;
		CueParams.Location = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
		CueParams.EffectContext = ExecutionParams.GetOwningSpec().GetEffectContext();

		if (bIsCritical)
		{
			FGameplayTagContainer DamageInfoTags;
			DamageInfoTags.AddTag(WxGameplayTags::Damage_Critical);
			CueParams.AggregatedSourceTags = DamageInfoTags;
		}

		TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
	}

	// 전투 피격 후처리
	AActor* TargetActor = TargetASC->GetOwnerActor();
	AActor* SourceActor = ExecutionParams.GetSourceAbilitySystemComponent()->GetOwnerActor();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	if (SourceASC)
	{
		// 공격자 MP 5 회복
		const UGameplayEffect* MPRecoveryEffect = UWxEffect_MPRecovery::StaticClass()->GetDefaultObject<UGameplayEffect>();
		SourceASC->ApplyGameplayEffectToSelf(MPRecoveryEffect, 1.f, SourceASC->MakeEffectContext());
	}
	if (TargetActor)
	{
		// AI 데미지 감지
		if (SourceActor)
		{
			UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, FinalDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
		}
    
		// HitReact 이벤트 발송
		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_HitReact, EventData);
	}
}
