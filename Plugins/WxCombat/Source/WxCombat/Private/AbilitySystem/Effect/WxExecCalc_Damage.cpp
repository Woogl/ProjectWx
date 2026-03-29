// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/Effect/WxEffect_GainMP.h"
#include "AbilitySystem/Effect/WxEffect_GainUP.h"
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
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}
	
	// --- 1. 무적 판정 (극한 회피) ---
	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Invincible))
	{
		if (const UGameplayAbility* Ability = TargetASC->GetAnimatingAbility())
		{
			if (Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Dodge))
			{
				ApplyResourceRecovery(TargetASC, UWxEffect_RecoveryMP::StaticClass(), 5.f);
			}
		}
		return;
	}

	// --- 2. 어트리뷰트 캡처 ---
	const FWxDamageStatics& Statics = GetDamageStatics();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = ExecutionParams.GetOwningSpec().CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = ExecutionParams.GetOwningSpec().CapturedTargetTags.GetAggregatedTags();

	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	const float DefenseMultiplier = 100.f / (100.f + TargetDEF);

	// --- 3. 대미지 판정 ---
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_PerfectGuard);
	FWxDamageResult DamageResult;

	if (bHasPerfectGuard)
	{
		// 퍼펙트 가드: 대미지 무효화, 공격자에게 DP 반사. 그로기 판정은 PostGameplayEffectExecute에서 수행
		if (SourceASC)
		{
			const float Reflect = FMath::Max(SourceATK * DefenseMultiplier, 0.f);

			const UGameplayEffect* ReflectEffect = UWxEffect_Reflect::StaticClass()->GetDefaultObject<UGameplayEffect>();
			FGameplayEffectSpec Spec(ReflectEffect, SourceASC->MakeEffectContext(), 1.f);
			Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_ReflectDP, Reflect);
			SourceASC->ApplyGameplayEffectSpecToSelf(Spec);
		}
	}
	else
	{
		// 일반/가드 피격: 데미지 계산 및 어트리뷰트 전달
		DamageResult = CalcDamage(ExecutionParams, EvalParams, SourceATK, DefenseMultiplier, TargetASC);
		if (DamageResult.FinalDamage > 0.f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));

			if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
			{
				OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
			}
		}
	}
	
	// --- 4. 자원 회복 ---
	if (bHasPerfectGuard)
	{
		// 패링 성공: 방어자 MP 회복
		ApplyResourceRecovery(TargetASC, UWxEffect_RecoveryMP::StaticClass(), 5.f);
	}
	else if (SourceASC)
	{
		// 공격 적중: 공격자 자원 회복 (평타 → MP, 스킬 → UP)
		const UGameplayAbility* OwningAbility = ExecutionParams.GetOwningSpec().GetContext().GetAbility();
		FGameplayTagContainer AbilityTags = OwningAbility->GetAssetTags();

		if (AbilityTags.HasTag(WxGameplayTags::Ability_Attack))
		{
			ApplyResourceRecovery(SourceASC, UWxEffect_RecoveryMP::StaticClass(), 5.f);
		}
		else if (AbilityTags.HasTag(WxGameplayTags::Ability_Skill))
		{
			ApplyResourceRecovery(SourceASC, UWxEffect_RecoveryUP::StaticClass(), 5.f);
		}
	}

	// --- 5. 피격 통지 (HitReact + AI 감지) ---
	{
		AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		AActor* TargetActor = TargetASC->GetOwnerActor();

		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_HitReact, EventData);

		if (SourceActor)
		{
			UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, DamageResult.FinalDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
		}
	}

	// --- 6. 대미지 GameplayCue ---
	ExecuteGameplayCueDamage(TargetASC, DamageResult.FinalDamage, TargetASC->GetAvatarActor()->GetActorLocation(), ExecutionParams.GetOwningSpec(), DamageResult.bIsCritical, !bHasPerfectGuard);
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

void UWxExecCalc_Damage::ApplyResourceRecovery(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> RecoveryEffect, float Amount) const
{
	if (ASC && RecoveryEffect && Amount > 0.f)
	{
		const UGameplayEffect* Effect = RecoveryEffect->GetDefaultObject<UGameplayEffect>();
		FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.f);
		Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery, Amount);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
	}
}


void UWxExecCalc_Damage::ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical, bool bDisplayDamagefloater) const
{
	if (!TargetASC)
	{
		return;
	}
	
	FGameplayCueParameters CueParams;
	CueParams.RawMagnitude = DamageAmount;
	CueParams.Location = HitLocation; // HitLocation을 OwingSpec으로부터 가져올까?
	CueParams.EffectContext = OwningSpec.GetEffectContext();

	if (bIsCritical)
	{
		CueParams.AggregatedSourceTags.AddTag(WxGameplayTags::Damage_Critical);
	}
	
	if (!bDisplayDamagefloater)
	{
		CueParams.AggregatedSourceTags.AddTag(WxGameplayTags::Damage_SuppressFloater);
	}

	// 데미지 GameplayCue 실행
	TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
}
