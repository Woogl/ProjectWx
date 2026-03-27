// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
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

	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DP, Target, false);
	}
};

static const FWxDamageStatics& GetDamageStatics()
{
	static FWxDamageStatics DamageStatics;
	return DamageStatics;
}

UWxExecCalc_Damage::UWxExecCalc_Damage()
{
	const FWxDamageStatics& Statics = GetDamageStatics();
	RelevantAttributesToCapture.Add(Statics.ATKDef);
	RelevantAttributesToCapture.Add(Statics.DEFDef);
	RelevantAttributesToCapture.Add(Statics.CritRateDef);
	RelevantAttributesToCapture.Add(Statics.CritDMGDef);
}

void UWxExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Invincible))
	{
		return;
	}

	AActor* TargetActor = TargetASC->GetOwnerActor();
	AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;

	const FWxDamageStatics& Statics = GetDamageStatics();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = ExecutionParams.GetOwningSpec().CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = ExecutionParams.GetOwningSpec().CapturedTargetTags.GetAggregatedTags();

	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	const float DefenseMultiplier = 100.f / (100.f + TargetDEF);

	// 퍼펙트 가드: 대미지 무효화 + 공격자에게 DP 반사
	if (HandlePerfectGuard(SourceASC, TargetASC, SourceActor, TargetActor, SourceATK, DefenseMultiplier))
	{
		return;
	}

	// 데미지 계산
	FWxDamageResult DamageResult = CalcDamage(ExecutionParams, EvalParams, SourceATK, DefenseMultiplier, TargetASC);
	if (DamageResult.FinalDamage <= 0.f)
	{
		return;
	}

	// IncomingDamage 어트리뷰트 전달
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));

	// 그로기 상태가 아닐 때만 DP 증가
	if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
	}

	// 피격 후처리
	ApplyPostDamageEffects(SourceASC, TargetASC, SourceActor, TargetActor, ExecutionParams.GetOwningSpec(), DamageResult);
}

bool UWxExecCalc_Damage::HandlePerfectGuard(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, AActor* SourceActor, AActor* TargetActor, float SourceATK, float DefenseMultiplier) const
{
	if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_PerfectGuard))
	{
		return false;
	}

	// 공격자에게 DP 반사 적용. 그로기 판정은 PostGameplayEffectExecute에서 수행
	if (SourceASC)
	{
		const float Reflect = FMath::Max(SourceATK * DefenseMultiplier, 0.f);

		const UGameplayEffect* ReflectEffect = UWxEffect_Reflect::StaticClass()->GetDefaultObject<UGameplayEffect>();
		FGameplayEffectSpec Spec(ReflectEffect, SourceASC->MakeEffectContext(), 1.f);
		Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_ReflectDP, Reflect);
		SourceASC->ApplyGameplayEffectSpecToSelf(Spec);
	}

	// 퍼펙트 가드 성공 시에도 HitReact 발동
	if (TargetActor)
	{
		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_HitReact, EventData);
	}

	return true;
}

FWxDamageResult UWxExecCalc_Damage::CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, float SourceATK, float DefenseMultiplier, UAbilitySystemComponent* TargetASC) const
{
	const FWxDamageStatics& Statics = GetDamageStatics();

	float SourceCritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritRateDef, EvalParams, SourceCritRate);

	float SourceCritDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDMGDef, EvalParams, SourceCritDMG);

	// FinalDamage = ATK_공격자 * (100 / (100 + DEF_피격자))
	FWxDamageResult Result;
	Result.FinalDamage = FMath::Max(SourceATK * DefenseMultiplier, 0.f);

	// 치명타 판정: CritRate 1당 1% 확률, 치명타 시 (1 + CritDMG * 0.01) 배율 적용
	const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
	Result.bIsCritical = FMath::FRand() < CritChance;
	if (Result.bIsCritical)
	{
		Result.FinalDamage *= (1.f + SourceCritDMG * 0.01f);
	}

	// 가드 중이면 데미지 50% 감소
	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Guard))
	{
		constexpr float GuardDamageReductionRate = 0.5f;
		Result.FinalDamage *= GuardDamageReductionRate;
	}

	return Result;
}

void UWxExecCalc_Damage::ApplyPostDamageEffects(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, AActor* SourceActor, AActor* TargetActor, const FGameplayEffectSpec& OwningSpec, const FWxDamageResult& DamageResult) const
{
	// 데미지 플로터 GameplayCue 실행
	{
		FGameplayCueParameters CueParams;
		CueParams.RawMagnitude = DamageResult.FinalDamage;
		CueParams.Location = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
		CueParams.EffectContext = OwningSpec.GetEffectContext();

		if (DamageResult.bIsCritical)
		{
			FGameplayTagContainer DamageInfoTags;
			DamageInfoTags.AddTag(WxGameplayTags::Damage_Critical);
			CueParams.AggregatedSourceTags = DamageInfoTags;
		}

		TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
	}

	// 공격자 MP 회복
	if (SourceASC)
	{
		const UGameplayEffect* MPRecoveryEffect = UWxEffect_MPRecovery::StaticClass()->GetDefaultObject<UGameplayEffect>();
		SourceASC->ApplyGameplayEffectToSelf(MPRecoveryEffect, 1.f, SourceASC->MakeEffectContext());
	}

	if (TargetActor)
	{
		// AI 데미지 감지
		if (SourceActor)
		{
			UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, DamageResult.FinalDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
		}

		// HitReact 이벤트 발송
		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_HitReact, EventData);
	}
}
