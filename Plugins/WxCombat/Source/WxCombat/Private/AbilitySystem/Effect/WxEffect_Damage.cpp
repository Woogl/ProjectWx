// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Damage/WxCombatEffectContext.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Damage::UWxEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxExecCalc_Damage::StaticClass();
	Executions.Add(ExecDef);

	// 예측 Cue는 타격 연출만 처리하고, 서버 크리 판정이 필요한 플로터는 AttributeSet에서 처리한다.
	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Hit);
	GameplayCues.Add(Cue);

	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->ApplicationTagRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);
	GEComponents.Add(TagReqComp);
}

struct FWxDamageBaseStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(GuardReductionScale);
	FWxDamageBaseStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, GuardReductionScale, Target, false);
	}
};

static const FWxDamageBaseStatics& GetDamageBaseStatics()
{
	static FWxDamageBaseStatics DamageBaseStatics;
	return DamageBaseStatics;
}

struct FWxDamageExecutionStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDMG);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingReflect);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(GP);
	FWxDamageExecutionStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingReflect, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, SP, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, GP, Target, false);
	}
};

static const FWxDamageExecutionStatics& GetDamageExecutionStatics()
{
	static FWxDamageExecutionStatics DamageExecutionStatics;
	return DamageExecutionStatics;
}

static float CalculateDefenseMultiplier(float TargetDEF)
{
	static constexpr float DefenseConstant = 100.f;
	return DefenseConstant / (DefenseConstant + TargetDEF);
}

static float CalculateBaseDamage(float SourceATK, float ATKCoeff, float DefenseMultiplier)
{
	return FMath::Max(SourceATK * ATKCoeff * DefenseMultiplier, 0.f);
}

static float CalculateCriticalMultiplier(float SourceCritDMG, bool bIsCritical)
{
	if (bIsCritical)
	{
		return 1.f + SourceCritDMG * 0.01f;
	}
	return 1.f;	
}

// 경감원이 겹치면 합이 1을 넘겨 피해 부호가 뒤집힌다. 어트리뷰트의 PreAttributeChange는 캡처 경로를 거치지 않아 상한을 걸지 못한다.
static float CalculateGuardMultiplier(float GuardReductionScale)
{
	return 1.f - FMath::Clamp(GuardReductionScale, 0.f, 1.f);
}

static float CalculateFinalDamage(float SourceATK, float TargetDEF, float ATKCoeff, float SourceCritDMG, bool bIsCritical, float GuardReductionScale)
{
	const float DefenseMultiplier = CalculateDefenseMultiplier(TargetDEF);
	const float BaseDamage = CalculateBaseDamage(SourceATK, ATKCoeff, DefenseMultiplier);
	const float CriticalMultiplier = CalculateCriticalMultiplier(SourceCritDMG, bIsCritical);
	const float GuardMultiplier = CalculateGuardMultiplier(GuardReductionScale);

	// 여기서 정수로 못박아야 HP·GP·가드 SP·반사량이 함께 정수로 움직인다 — 소수 잔량으로 생존하거나 그로기 임계에 못 닿는 상황을 없앤다.
	return FMath::RoundToFloat(BaseDamage * CriticalMultiplier * GuardMultiplier);
}

UWxExecCalc_Damage::UWxExecCalc_Damage()
{
	const FWxDamageBaseStatics& BaseStatics = GetDamageBaseStatics();
	const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();
	RelevantAttributesToCapture.Add(BaseStatics.ATKDef);
	RelevantAttributesToCapture.Add(BaseStatics.DEFDef);
	RelevantAttributesToCapture.Add(BaseStatics.GuardReductionScaleDef);
	RelevantAttributesToCapture.Add(ExecutionStatics.CritRateDef);
	RelevantAttributesToCapture.Add(ExecutionStatics.CritDMGDef);
	RelevantAttributesToCapture.Add(ExecutionStatics.SPDef);
}
void UWxExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	// GetContext는 핸들을 값으로 주므로 const 스펙에서도 쓰기가 열린다.
	FGameplayEffectContextHandle ContextHandle = OwningSpec.GetContext();
	FGameplayEffectContext* RawContext = ContextHandle.Get();
	FWxCombatEffectContext* CombatContext = (RawContext && RawContext->GetScriptStruct() == FWxCombatEffectContext::StaticStruct())
		? static_cast<FWxCombatEffectContext*>(RawContext)
		: nullptr;
	ensureMsgf(CombatContext, TEXT("대미지 컨텍스트가 FWxCombatEffectContext가 아니다. DefaultGame.ini의 AbilitySystemGlobalsClassName 등록을 확인할 것."));

	const bool bCanGuard = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanGuard);
	const bool bCanCritical = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanCritical);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_GuardReduction);
	const bool bIsGroggy = TargetASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy);
	const bool bPerfectGuardApplied = bCanGuard && TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_PerfectGuard);

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	// 퍼펙트 가드는 반사량 산출을 위해 크리를 스킵한다.
	const FWxDamageBaseStatics& BaseStatics = GetDamageBaseStatics();
	float SourceATK = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(BaseStatics.ATKDef, EvalParams, SourceATK);

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(BaseStatics.DEFDef, EvalParams, TargetDEF);

	const float ATKCoeff = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, false, 0.f);
	const bool bCanApplyCritical = !bPerfectGuardApplied && bCanCritical;
	const bool bGuardHit = !bPerfectGuardApplied && bIsGuarding && bCanGuard;

	float SourceCritRate = 0.f;
	float SourceCritDMG = 0.f;
	bool bIsCritical = false;
	if (bCanApplyCritical)
	{
		const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExecutionStatics.CritRateDef, EvalParams, SourceCritRate);
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExecutionStatics.CritDMGDef, EvalParams, SourceCritDMG);

		const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
		bIsCritical = FMath::FRand() < CritChance;
	}

	float GuardReductionScale = 0.f;
	if (bGuardHit)
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(BaseStatics.GuardReductionScaleDef, EvalParams, GuardReductionScale);
	}

	const float FinalDamage = CalculateFinalDamage(SourceATK, TargetDEF, ATKCoeff, SourceCritDMG, bIsCritical, GuardReductionScale);

	const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();

	if (bPerfectGuardApplied)
	{
		// 대상 어트리뷰트는 하나도 바뀌지 않으므로, 반사량을 메타 어트리뷰트로 실어야 PostGameplayEffectExecute가 돈다.
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.IncomingReflectProperty, EGameplayModOp::Additive, FinalDamage));
		return;
	}

	if (FinalDamage <= 0.f)
	{
		return;
	}

	if (CombatContext)
	{
		CombatContext->SetCritical(bIsCritical);
	}

	if (bGuardHit)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.SPProperty, EGameplayModOp::Additive, -FinalDamage));

		// 가드 브레이크는 차감 전 SP가 있어야 판정할 수 있다.
		float TargetSP = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExecutionStatics.SPDef, EvalParams, TargetSP);
		if (TargetSP <= FinalDamage)
		{
			ExecutionParams.GetOwningSpecForPreExecuteMod()->AddDynamicAssetTag(WxGameplayTags::Damage_GuardBreak);
		}
	}

	if (!bIsGroggy)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.GPProperty, EGameplayModOp::Additive, FinalDamage));
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.IncomingDamageProperty, EGameplayModOp::Additive, FinalDamage));
}
