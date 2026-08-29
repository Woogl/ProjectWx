// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_Guard.h"
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
	FWxDamageBaseStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
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

UWxExecCalc_Damage::UWxExecCalc_Damage()
{
	const FWxDamageBaseStatics& BaseStatics = GetDamageBaseStatics();
	const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();
	RelevantAttributesToCapture.Add(BaseStatics.ATKDef);
	RelevantAttributesToCapture.Add(BaseStatics.DEFDef);
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
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_Guard);
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
	// 방어력 보정 상수
	static constexpr float DefenseConstant = 100.f;
	const float DefenseMultiplier = DefenseConstant / (DefenseConstant + TargetDEF);
	float FinalDamage = FMath::Max(SourceATK * ATKCoeff * DefenseMultiplier, 0.f);
	bool bIsCritical = false;
	if (!bPerfectGuardApplied && bCanCritical)
	{
		const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();

		float SourceCritRate = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExecutionStatics.CritRateDef, EvalParams, SourceCritRate);

		float SourceCritDMG = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExecutionStatics.CritDMGDef, EvalParams, SourceCritDMG);

		const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
		bIsCritical = FMath::FRand() < CritChance;
		if (bIsCritical)
		{
			FinalDamage *= (1.f + SourceCritDMG * 0.01f);
		}
	}

	const FWxDamageExecutionStatics& ExecutionStatics = GetDamageExecutionStatics();

	if (bPerfectGuardApplied)
	{
		// 대상 어트리뷰트는 하나도 바뀌지 않으므로, 반사량을 메타 어트리뷰트로 실어야 PostGameplayEffectExecute가 돈다.
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ExecutionStatics.IncomingReflectProperty, EGameplayModOp::Additive, FinalDamage));
		return;
	}

	const bool bGuardHit = bIsGuarding && bCanGuard;

	if (bGuardHit)
	{
		FinalDamage *= UWxEffect_Guard::DamageMultiplier;
	}

	if (FinalDamage <= 0.f)
	{
		return;
	}

	// 컨텍스트에는 어트리뷰트로 전달할 수 없는 크리 결과만 기록한다.
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
