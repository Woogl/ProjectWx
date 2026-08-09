// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
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

	if (HandleInvincible(SourceASC, TargetASC))
	{
		return;
	}

	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_PerfectGuard);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);
	const bool bIsGroggy = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy);

	// Unblockable 공격은 퍼펙트 가드를 포함한 모든 가드를 무시한다.
	const bool bPerfectGuardApplied = bHasPerfectGuard && !bIsUnblockable;

	// 퍼펙트 가드는 반사량 산출을 위해 크리를 스킵한다.
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	FWxDamageResult DamageResult = CalcDamage(ExecutionParams, EvalParams, bPerfectGuardApplied);

	if (bPerfectGuardApplied)
	{
		ReflectPerfectGuard(SourceASC, DamageResult.FinalDamage);

		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		EventData.Target = TargetASC->GetOwnerActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetOwnerActor(), WxGameplayTags::Event_PerfectGuard, EventData);

		if (SourceASC && OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_ParryHitReact))
		{
			FGameplayEventData ParryEventData;
			ParryEventData.Instigator = TargetASC->GetOwnerActor();
			ParryEventData.Target = SourceASC->GetOwnerActor();
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceASC->GetOwnerActor(), WxGameplayTags::Event_HitReact_Parry, ParryEventData);
		}
	}
	else
	{
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

		if (DamageResult.FinalDamage > 0.f)
		{
			const FWxDamageStatics& Statics = GetDamageStatics();
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));

			if (!bIsGroggy)
			{
				OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
			}

			ApplyHitReaction(ExecutionParams, OutExecutionOutput, DamageResult.FinalDamage);

			const float RecoveryUP = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, false, 0.f);
			const float RecoveryMP = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, false, 0.f);
			UWxEffect_RecoverResource::ApplyTo(SourceASC, RecoveryUP, RecoveryMP);
		}
	}

	FVector HitLocation = FVector::ZeroVector;
	const FHitResult* HitResult = OwningSpec.GetEffectContext().GetHitResult();
	if (HitResult)
	{
		HitLocation = FVector(HitResult->ImpactPoint);
	}
	else if (const AActor* AvatarActor = TargetASC->GetAvatarActor())
	{
		HitLocation = AvatarActor->GetActorLocation();
	}

	if (bPerfectGuardApplied)
	{
		ExecuteGameplayCuePerfectGuard(TargetASC, HitLocation, OwningSpec);
	}
	else if (DamageResult.FinalDamage > 0.f)
	{
		ExecuteGameplayCueDamage(TargetASC, DamageResult.FinalDamage, HitLocation, OwningSpec, DamageResult.bIsCritical);
	}

	// 무적 회피로 조기 리턴한 경우를 빼면 퍼펙트 가드까지 포함해 모든 적중에서 보낸다.
	// 공격자 ASC가 이 이벤트를 받아, 컨텍스트의 어빌리티가 재생 중인 몽타주를 그 시간만큼 잠깐 멈춘다.
	if (AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr)
	{
		const float HitStopDuration = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_HitStop, false, 0.f);
		if (HitStopDuration > 0.f)
		{
			FGameplayEventData HitStopEvent;
			HitStopEvent.Instigator = SourceActor;
			HitStopEvent.Target = TargetASC->GetOwnerActor();
			HitStopEvent.EventMagnitude = HitStopDuration;
			HitStopEvent.ContextHandle = OwningSpec.GetEffectContext();
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceActor, WxGameplayTags::Event_HitStop, HitStopEvent);
		}
	}
}

bool UWxExecCalc_Damage::HandleInvincible(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const
{
	if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Invincible))
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	EventData.Target = TargetASC->GetOwnerActor();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetOwnerActor(), WxGameplayTags::Event_DodgeSuccess, EventData);

	return true;
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

void UWxExecCalc_Damage::ApplyHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	if (!TargetASC)
	{
		return;
	}

	const FWxDamageStatics& Statics = GetDamageStatics();
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);
	const bool bGuardHit = bIsGuarding && !bIsUnblockable;
	const bool bIsGroggy = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy);

	// 일반 가드만 SP를 차감한다(Unblockable 가드·비가드는 차감 없음).
	if (bGuardHit)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.SPProperty, EGameplayModOp::Additive, -FinalDamage));
	}

	// 공격 GE가 실은 Event.HitReact.* 자식 태그를 꺼낸다(없으면 이벤트 미발송).
	const FGameplayTagContainer HitReactMatches = OwningSpec.GetDynamicAssetTags().Filter(FGameplayTagContainer(WxGameplayTags::Event_HitReact));
	const FGameplayTag HitReactTag = HitReactMatches.IsEmpty() ? FGameplayTag() : HitReactMatches.First();

	FGameplayTag EventTag;
	if (bIsGroggy)
	{
		FGameplayTagContainer KnockTags;
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockBack);
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockDown);
		KnockTags.AddTagFast(WxGameplayTags::Event_HitReact_KnockUp);
		if (HitReactTag.MatchesAny(KnockTags))
		{
			// 그로기 중에는 Knock 계열을 쓰지 않고 Normal로 치환한다.
			EventTag = WxGameplayTags::Event_HitReact_Normal;
		}
		else
		{
			EventTag = HitReactTag;
		}
	}
	else if (bGuardHit)
	{
		// 일반 가드 — Knock 계열과 Normal 분기는 Guard 어빌리티가 하므로 태그를 그대로 넘긴다.
		// 명시 태그가 없으면 가드 흡수 애니메이션을 위해 Normal로 폴백한다.
		EventTag = HitReactTag.IsValid() ? HitReactTag : WxGameplayTags::Event_HitReact_Normal;
	}
	else if (bIsGuarding)
	{
		// Unblockable 가드 — 가드를 끊고 명시된 HitReact를 보낸다.
		const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
		TargetASC->CancelAbilities(&GuardAbilityTags);
		EventTag = HitReactTag;
	}
	else
	{
		// 비가드 — 명시된 태그를 그대로 쓴다.
		EventTag = HitReactTag;
	}

	if (!EventTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	AActor* TargetActor = TargetASC->GetOwnerActor();
	FGameplayEventData EventData;
	EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	EventData.Target = TargetActor;
	EventData.EventMagnitude = FinalDamage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, EventData);
}

void UWxExecCalc_Damage::ExecuteGameplayCueDamage(UAbilitySystemComponent* TargetASC, float DamageAmount, FVector HitLocation, const FGameplayEffectSpec& OwningSpec, bool bIsCritical) const
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

	TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_Damage, CueParams);
}

void UWxExecCalc_Damage::ExecuteGameplayCuePerfectGuard(UAbilitySystemComponent* TargetASC, FVector HitLocation, const FGameplayEffectSpec& OwningSpec) const
{
	if (!TargetASC)
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Location = HitLocation;
	CueParams.EffectContext = OwningSpec.GetEffectContext();

	TargetASC->ExecuteGameplayCue(WxGameplayTags::GameplayCue_PerfectGuard, CueParams);
}
