// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/Effect/WxEffect_RecoveryMP.h"
#include "AbilitySystem/Effect/WxEffect_RecoveryUP.h"
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
	DECLARE_ATTRIBUTE_CAPTUREDEF(PP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DP);
	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, PP, Target, false);
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
	RelevantAttributesToCapture.Add(Statics.PPDef);
}

// ────────────────────────────────────────────────────────────────────────────
//  Execute
// ────────────────────────────────────────────────────────────────────────────

void UWxExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	// --- 1. 무적 판정 (극한 회피) ---
	if (HandleInvincible(SourceASC, TargetASC))
	{
		return;
	}

	// --- 2. 어트리뷰트 캡처 ---
	const FWxDamageStatics& Statics = GetDamageStatics();
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	const float DefenseMultiplier = 100.f / (100.f + TargetDEF);

	// --- 3. 방어 판정 및 대미지 계산 ---
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_PerfectGuard);

	FWxDamageResult DamageResult;

	if (bHasPerfectGuard)
	{
		ReflectPerfectGuard(SourceASC, SourceATK * DefenseMultiplier);
		ApplyResourceRecovery(TargetASC, UWxEffect_RecoveryMP::StaticClass(), 5.f);
	}
	else
	{
		DamageResult = CalcDamage(ExecutionParams, EvalParams, SourceATK, DefenseMultiplier, TargetASC, bIsUnblockable);
		if (DamageResult.FinalDamage > 0.f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));

			if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
			{
				OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
			}
		}
		RecoverAttackerResource(SourceASC, OwningSpec);

		// 가드 중이면 PP를 대미지만큼 추가 차감
		if (!bIsUnblockable && TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Guard) && DamageResult.FinalDamage > 0.f)
		{
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.PPProperty, EGameplayModOp::Additive, -DamageResult.FinalDamage));
		}
	}

	// --- 4. PP 차감 및 HitReact ---
	if (!bHasPerfectGuard && DamageResult.FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.PPProperty, EGameplayModOp::Additive, -DamageResult.FinalDamage));
	}

	float TargetPP = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.PPDef, EvalParams, TargetPP);
	const float PPAfterDamage = bHasPerfectGuard ? TargetPP : TargetPP - DamageResult.FinalDamage;

	if (PPAfterDamage <= 0.f)
	{
		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		EventData.Target = TargetASC->GetOwnerActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetOwnerActor(), WxGameplayTags::Event_HitReact, EventData);
	}

	// --- 5. AI 대미지 감지 ---
	AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	if (SourceActor)
	{
		AActor* TargetActor = TargetASC->GetOwnerActor();
		UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, DamageResult.FinalDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	}

	// --- 6. 대미지 GameplayCue ---
	FVector HitLocation = FVector(TargetASC->GetAvatarActor()->GetActorLocation());
	const FHitResult* HitResult = OwningSpec.GetEffectContext().GetHitResult();
	if (HitResult)
	{
		HitLocation = FVector(HitResult->ImpactPoint);
	}
	ExecuteGameplayCueDamage(TargetASC, DamageResult.FinalDamage, HitLocation, OwningSpec, DamageResult.bIsCritical, !bHasPerfectGuard);
}

// ────────────────────────────────────────────────────────────────────────────
//  방어 판정
// ────────────────────────────────────────────────────────────────────────────

bool UWxExecCalc_Damage::HandleInvincible(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const
{
	if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Invincible))
	{
		return false;
	}

	const UGameplayAbility* Ability = TargetASC->GetAnimatingAbility();
	if (Ability && Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Dodge))
	{
		ApplyResourceRecovery(TargetASC, UWxEffect_RecoveryMP::StaticClass(), 5.f);

		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		EventData.Target = TargetASC->GetOwnerActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetOwnerActor(), WxGameplayTags::Event_DodgeSuccess, EventData);
	}

	return true;
}

void UWxExecCalc_Damage::ReflectPerfectGuard(UAbilitySystemComponent* SourceASC, float ReflectAmount) const
{
	if (!SourceASC)
	{
		return;
	}

	const float ClampedReflect = FMath::Max(ReflectAmount, 0.f);
	const UGameplayEffect* ReflectEffect = UWxEffect_Reflect::StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(ReflectEffect, SourceASC->MakeEffectContext(), 1.f);
	Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_ReflectDP, ClampedReflect);
	SourceASC->ApplyGameplayEffectSpecToSelf(Spec);
}

// ────────────────────────────────────────────────────────────────────────────
//  대미지 계산
// ────────────────────────────────────────────────────────────────────────────

FWxDamageResult UWxExecCalc_Damage::CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, float SourceATK, float DefenseMultiplier, UAbilitySystemComponent* TargetASC, bool bIsUnblockable) const
{
	const FWxDamageStatics& Statics = GetDamageStatics();

	float SourceCritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritRateDef, EvalParams, SourceCritRate);

	float SourceCritDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDMGDef, EvalParams, SourceCritDMG);

	// 기본 대미지: ATK * (100 / (100 + DEF))
	FWxDamageResult Result;
	Result.FinalDamage = FMath::Max(SourceATK * DefenseMultiplier, 0.f);

	// 치명타: CritRate 1당 1%, 치명타 시 (1 + CritDMG * 0.01) 배율
	const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
	Result.bIsCritical = FMath::FRand() < CritChance;
	if (Result.bIsCritical)
	{
		Result.FinalDamage *= (1.f + SourceCritDMG * 0.01f);
	}

	// 가드 감소: Unblockable이 아닌 경우에만 50% 감소
	if (!bIsUnblockable && TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_Guard))
	{
		constexpr float GuardDamageReductionRate = 0.5f;
		Result.FinalDamage *= GuardDamageReductionRate;
	}

	return Result;
}

// ────────────────────────────────────────────────────────────────────────────
//  자원 회복
// ────────────────────────────────────────────────────────────────────────────

void UWxExecCalc_Damage::RecoverAttackerResource(UAbilitySystemComponent* SourceASC, const FGameplayEffectSpec& OwningSpec) const
{
	if (!SourceASC)
	{
		return;
	}

	const UGameplayAbility* OwningAbility = OwningSpec.GetContext().GetAbility();
	if (!OwningAbility)
	{
		return;
	}

	const FGameplayTagContainer AbilityTags = OwningAbility->GetAssetTags();

	if (AbilityTags.HasTag(WxGameplayTags::Ability_Attack))
	{
		ApplyResourceRecovery(SourceASC, UWxEffect_RecoveryMP::StaticClass(), 5.f);
	}
	else if (AbilityTags.HasTag(WxGameplayTags::Ability_Skill))
	{
		ApplyResourceRecovery(SourceASC, UWxEffect_RecoveryUP::StaticClass(), 5.f);
	}
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

// ────────────────────────────────────────────────────────────────────────────
//  GameplayCue
// ────────────────────────────────────────────────────────────────────────────

void UWxExecCalc_Damage::ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical, bool bDisplayDamageFloater) const
{
	if (!TargetASC)
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.RawMagnitude = DamageAmount;
	CueParams.Location = HitLocation;
	CueParams.EffectContext = OwningSpec.GetEffectContext();

	if (bIsCritical)
	{
		CueParams.AggregatedSourceTags.AddTag(WxGameplayTags::Damage_Critical);
	}

	if (!bDisplayDamageFloater)
	{
		CueParams.AggregatedSourceTags.AddTag(WxGameplayTags::Damage_SuppressFloater);
	}

	TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
}
