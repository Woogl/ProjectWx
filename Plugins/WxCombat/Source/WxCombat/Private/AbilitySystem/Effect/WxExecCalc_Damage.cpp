// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Damage/WxCombatEffectContext.h"
#include "GameplayEffect.h"
#include "GenericTeamAgentInterface.h"
#include "WxGameplayTags.h"

struct FWxDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDMG);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DP);
	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
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
	// 여기 남긴 판정 결과가 GE 적용 후 발행 단계의 유일한 입력이다.
	FGameplayEffectContextHandle ContextHandle = OwningSpec.GetContext();
	FGameplayEffectContext* RawContext = ContextHandle.Get();
	FWxCombatEffectContext* CombatContext = (RawContext && RawContext->GetScriptStruct() == FWxCombatEffectContext::StaticStruct())
		? static_cast<FWxCombatEffectContext*>(RawContext)
		: nullptr;
	ensureMsgf(CombatContext, TEXT("대미지 컨텍스트가 FWxCombatEffectContext가 아니다. DefaultGame.ini의 AbilitySystemGlobalsClassName 등록을 확인할 것."));

	// 같은 컨텍스트를 여러 스펙이 공유하므로, 어느 경로로 빠져나가든 이전 판정이 남지 않게 먼저 지운다.
	if (CombatContext)
	{
		CombatContext->ClearDamageResult();
	}

	const EWxDamageResult DamageCheck = CheckDamage(SourceASC, TargetASC);
	if (DamageCheck != EWxDamageResult::Damaged)
	{
		if (CombatContext && DamageCheck == EWxDamageResult::Evaded)
		{
			CombatContext->SetEvaded();
		}
		return;
	}

	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bCanCritical = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_CanCritical);
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_PerfectGuard);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);
	const bool bIsGroggy = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy);

	const bool bPerfectGuardApplied = bHasPerfectGuard && !bIsUnblockable;

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	// 퍼펙트 가드는 반사량 산출을 위해 크리를 스킵한다.
	FWxDamageResult DamageResult = CalcDamage(ExecutionParams, EvalParams, bPerfectGuardApplied || !bCanCritical);

	if (bPerfectGuardApplied)
	{
		ReflectPerfectGuard(SourceASC, DamageResult.FinalDamage);

		if (CombatContext)
		{
			CombatContext->SetPerfectGuard(DamageResult.FinalDamage);
		}
		return;
	}

	// 가드 감소율은 발동 중인 Guard 어빌리티 인스턴스가 들고 있다.
	if (!bIsUnblockable && bIsGuarding)
	{
		for (const FGameplayAbilitySpec& Spec : TargetASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive())
			{
				continue;
			}
			if (const UWxAbility_Guard* Guard = Cast<UWxAbility_Guard>(Spec.GetPrimaryInstance()))
			{
				DamageResult.FinalDamage *= Guard->GetDamageReductionRate();
				break;
			}
		}
	}

	if (DamageResult.FinalDamage <= 0.f)
	{
		return;
	}

	const FWxDamageStatics& Statics = GetDamageStatics();
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));

	if (!bIsGroggy)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
	}

	// 그로기 진입 히트의 Knock 치환은 DP가 적용되기 전인 지금 상태로 판정해야 하므로, 태그를 여기서 확정해 컨텍스트에 싣는다.
	const FGameplayTag HitReactTag = ResolveHitReaction(ExecutionParams, OutExecutionOutput, DamageResult.FinalDamage);

	const float RecoveryUP = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, false, 0.f);
	const float RecoveryMP = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, false, 0.f);
	UWxEffect_RecoverResource::ApplyTo(SourceASC, RecoveryUP, RecoveryMP);

	if (CombatContext)
	{
		CombatContext->SetDamaged(DamageResult.FinalDamage, DamageResult.bIsCritical, HitReactTag);
	}
}

EWxDamageResult UWxExecCalc_Damage::CheckDamage(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target)
{
	if (!Target)
	{
		return EWxDamageResult::None;
	}

	// 대미지 GE 자체가 사망 타겟을 IgnoreTags로 거르므로 대미지 경로는 여기 닿지 않는다.
	// ExecCalc 밖에서 부르는 화상·히트스톱이 시체를 걸러내는 지점이다.
	if (Target->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
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

	if (Target->HasMatchingGameplayTag(WxGameplayTags::State_Invincible))
	{
		return EWxDamageResult::Evaded;
	}

	return EWxDamageResult::Damaged;
}

void UWxExecCalc_Damage::ReflectPerfectGuard(UAbilitySystemComponent* SourceASC, float ReflectAmount) const
{
	if (!SourceASC)
	{
		return;
	}

	const float ClampedReflect = FMath::Max(ReflectAmount, 0.f);
	const FGameplayAttribute DPAttribute = UWxCombatAttributeSet::GetDPAttribute();

	// 상한 클램프와 그로기 판정은 어트리뷰트 셋의 변경 훅이 맡으므로 여기서는 가산만 한다.
	SourceASC->SetNumericAttributeBase(DPAttribute, SourceASC->GetNumericAttributeBase(DPAttribute) + ClampedReflect);
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

FGameplayTag UWxExecCalc_Damage::ResolveHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	if (!TargetASC)
	{
		return FGameplayTag();
	}

	const FWxDamageStatics& Statics = GetDamageStatics();
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);
	const bool bGuardHit = bIsGuarding && !bIsUnblockable;
	const bool bIsGroggy = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy);

	if (bGuardHit)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.SPProperty, EGameplayModOp::Additive, -FinalDamage));
	}

	// 공격 GE가 실은 Event.HitReact.* 자식 태그를 꺼낸다(없으면 이벤트 미발송).
	const FGameplayTagContainer HitReactMatches = OwningSpec.GetDynamicAssetTags().Filter(FGameplayTagContainer(WxGameplayTags::Event_HitReact));
	const FGameplayTag HitReactTag = HitReactMatches.IsEmpty() ? FGameplayTag() : HitReactMatches.First();

	if (bIsGroggy)
	{
		FGameplayTagContainer KnockTags;
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockBack);
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockDown);
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockUp);
		if (HitReactTag.MatchesAny(KnockTags))
		{
			return WxGameplayTags::Event_HitReact_Normal;
		}

		return HitReactTag;
	}

	if (bGuardHit)
	{
		// 일반 가드 — Knock 계열과 Normal 분기는 Guard 어빌리티가 하므로 태그를 그대로 넘긴다.
		// 명시 태그가 없으면 가드 흡수 애니메이션을 위해 Normal로 폴백한다.
		return HitReactTag.IsValid() ? HitReactTag : WxGameplayTags::Event_HitReact_Normal;
	}

	if (bIsGuarding)
	{
		// Unblockable 가드 — 가드를 끊고 명시된 HitReact를 보낸다.
		const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
		TargetASC->CancelAbilities(&GuardAbilityTags);
	}

	return HitReactTag;
}
