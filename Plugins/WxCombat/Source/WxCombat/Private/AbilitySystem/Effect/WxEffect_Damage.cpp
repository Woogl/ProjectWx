// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_Guard.h"
#include "AbilitySystemComponent.h"
#include "Damage/WxCombatEffectContext.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GenericTeamAgentInterface.h"
#include "WxGameplayTags.h"

UWxEffect_Damage::UWxEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxExecCalc_Damage::StaticClass();
	Executions.Add(ExecDef);

	// GE가 Cue를 들고 있으면 예측 적용한 클라에서도 엔진이 발행해준다 — 임팩트 연출이 서버 왕복을 기다리지 않는다.
	// 플로터는 크리 판정이 서버에 있어 여기 얹지 않는다.
	FGameplayEffectCue Cue;
	Cue.GameplayCueTags.AddTag(WxGameplayTags::GameplayCue_Hit);
	GameplayCues.Add(Cue);

	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->ApplicationTagRequirements.IgnoreTags.AddTag(WxGameplayTags::Ability_Death);
	GEComponents.Add(TagReqComp);
}

struct FWxDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDMG);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingReflect);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DP);
	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingReflect, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, SP, Target, false);
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

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	// GetContext는 핸들을 값으로 주므로 const 스펙에서도 쓰기가 열린다.
	FGameplayEffectContextHandle ContextHandle = OwningSpec.GetContext();
	FGameplayEffectContext* RawContext = ContextHandle.Get();
	FWxCombatEffectContext* CombatContext = (RawContext && RawContext->GetScriptStruct() == FWxCombatEffectContext::StaticStruct())
		? static_cast<FWxCombatEffectContext*>(RawContext)
		: nullptr;
	ensureMsgf(CombatContext, TEXT("대미지 컨텍스트가 FWxCombatEffectContext가 아니다. DefaultGame.ini의 AbilitySystemGlobalsClassName 등록을 확인할 것."));

	// 같은 컨텍스트를 여러 스펙이 공유하므로, 어느 경로로 빠져나가든 이전 판정이 남지 않게 먼저 지운다.
	if (CombatContext)
	{
		CombatContext->SetCritical(false);
	}

	// 회피 성공 발행은 적용 전에 같은 판정을 돌리는 UWxCombatLibrary::ApplyDamage가 맡는다.
	if (CheckDamage(SourceASC, TargetASC) != EWxDamageResult::Damaged)
	{
		return;
	}

	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bCanCritical = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanCritical);
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_PerfectGuard);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_Guard);
	const bool bIsGroggy = TargetASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy);

	const bool bPerfectGuardApplied = bHasPerfectGuard && !bIsUnblockable;

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	// 퍼펙트 가드는 반사량 산출을 위해 크리를 스킵한다.
	FWxDamageResult DamageResult = CalcDamage(ExecutionParams, EvalParams, bPerfectGuardApplied || !bCanCritical);

	const FWxDamageStatics& Statics = GetDamageStatics();

	if (bPerfectGuardApplied)
	{
		// 대상 어트리뷰트는 하나도 바뀌지 않으므로, 반사량을 메타 어트리뷰트로 실어야 PostGameplayEffectExecute가 돈다.
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingReflectProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
		return;
	}

	const bool bGuardHit = bIsGuarding && !bIsUnblockable;

	if (bGuardHit)
	{
		DamageResult.FinalDamage *= UWxEffect_Guard::DamageMultiplier;
	}

	if (DamageResult.FinalDamage <= 0.f)
	{
		return;
	}

	// 컨텍스트에는 어트리뷰트로 복원할 수 없는 것만 싣는다 — 수치는 IncomingDamage로, 반응 태그는 스펙으로 소비 지점에 닿는다.
	if (CombatContext)
	{
		CombatContext->SetCritical(DamageResult.bIsCritical);
	}

	// 출력 순서가 곧 계약이라 IncomingDamage가 맨 뒤다.
	// SP: 가드가 이 히트로 깨졌는지 판정하고, DP: 그로기 진입이 HitReact 어빌리티에 보인다.
	if (bGuardHit)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.SPProperty, EGameplayModOp::Additive, -DamageResult.FinalDamage));
	}

	if (!bIsGroggy)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
}

EWxDamageResult UWxExecCalc_Damage::CheckDamage(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target)
{
	if (!Target)
	{
		return EWxDamageResult::None;
	}

	// 대미지 GE 자체가 사망 타겟을 IgnoreTags로 거르므로 대미지 경로는 여기 닿지 않는다.
	// ExecCalc 밖에서 부르는 화상·히트스톱이 시체를 걸러내는 지점이다.
	if (Target->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
	{
		return EWxDamageResult::None;
	}

	// 적대 관계에만 피해가 성립한다 — 아군·중립은 대미지도 연출도 발생하지 않는다.
	// 자기 자신은 자해 경로라 제외하고, 팀 개념이 없는 공격자는 판정 근거가 없어 통과시킨다.
	const AActor* SourceAvatar = Source ? Source->GetAvatarActor() : nullptr;
	const AActor* TargetAvatar = Target->GetAvatarActor();
	if (SourceAvatar && TargetAvatar && SourceAvatar != TargetAvatar)
	{
		const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(SourceAvatar);
		if (SourceTeamAgent && SourceTeamAgent->GetTeamAttitudeTowards(*TargetAvatar) != ETeamAttitude::Hostile)
		{
			return EWxDamageResult::None;
		}
	}

	if (Target->HasMatchingGameplayTag(WxGameplayTags::Effect_Invincible))
	{
		return EWxDamageResult::Evaded;
	}

	return EWxDamageResult::Damaged;
}

FWxDamageResult UWxExecCalc_Damage::CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const
{
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const FWxDamageStatics& Statics = GetDamageStatics();

	const float RawDamage = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_RawDamage, false, 0.f);
	const bool bRawMode = RawDamage > 0.f;

	float BaseDamage;
	if (bRawMode)
	{
		BaseDamage = RawDamage;
	}
	else
	{
		float SourceATK = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ATKDef, EvalParams, SourceATK);
		const float ATKCoeff = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, false, 0.f);
		SourceATK *= ATKCoeff;

		float TargetDEF = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);
		const float DefenseMultiplier = 100.f / (100.f + TargetDEF);

		BaseDamage = SourceATK * DefenseMultiplier;
	}

	FWxDamageResult Result;
	Result.FinalDamage = FMath::Max(BaseDamage, 0.f);

	if (bRawMode || bSkipCrit)
	{
		return Result;
	}

	float SourceCritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritRateDef, EvalParams, SourceCritRate);

	float SourceCritDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDMGDef, EvalParams, SourceCritDMG);

	const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
	Result.bIsCritical = FMath::FRand() < CritChance;
	if (Result.bIsCritical)
	{
		Result.FinalDamage *= (1.f + SourceCritDMG * 0.01f);
	}

	return Result;
}
