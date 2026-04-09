// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/Effect/WxEffect_Reflect.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Perception/AISense_Damage.h"
#include "WxGameplayTags.h"

struct FWxDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ATK);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DEF);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritRate);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDMG);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(SP);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DP);
	FWxDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, ATK, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, DEF, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritRate, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, CritDMG, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, IncomingDamage, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UWxCombatAttributeSet, PP, Target, false);
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

	// --- 1. 무적 판정 ---
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

	// 공격력 계수: 무기 ANS_WeaponAttack가 SetByCaller로 전달. 미설정 시 1.f.
	const float ATKCoeff = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Coeff_ATK, false, 1.f);
	SourceATK *= ATKCoeff;

	float TargetDEF = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DEFDef, EvalParams, TargetDEF);

	float TargetPP = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.PPDef, EvalParams, TargetPP);

	const float DefenseMultiplier = 100.f / (100.f + TargetDEF);

	// --- 3. 상태 판정 ---
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::ANS_PerfectGuard);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);

	// --- 4. 대미지 적용 ---
	FWxDamageResult DamageResult;

	if (bHasPerfectGuard)
	{
		DamageResult.FinalDamage = FMath::Max(SourceATK * DefenseMultiplier, 0.f);
		ReflectPerfectGuard(SourceASC, DamageResult.FinalDamage);

		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		EventData.Target = TargetASC->GetOwnerActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetOwnerActor(), WxGameplayTags::Event_PerfectGuard, EventData);
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

			ApplyHitReaction(SourceASC, TargetASC, OwningSpec, DamageResult.FinalDamage, TargetPP, bIsGuarding, bIsUnblockable, OutExecutionOutput);

			if (SourceASC)
			{
				FGameplayEventData EventData;
				EventData.Instigator = SourceASC->GetOwnerActor();
				EventData.Target = TargetASC->GetOwnerActor();
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceASC->GetOwnerActor(), WxGameplayTags::Event_AttackHit, EventData);
			}
		}
	}

	// --- 5. AI 대미지 감지 ---
	AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	AActor* TargetActor = TargetASC->GetOwnerActor();
	if (SourceActor && TargetActor)
	{
		// 퍼펙트 가드 시 Target에 실제 대미지가 없으므로 0으로 보고
		const float ReportedDamage = bHasPerfectGuard ? 0.f : DamageResult.FinalDamage;
		UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, ReportedDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	}

	// --- 6. 대미지 GameplayCue ---
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
	ExecuteGameplayCueDamage(TargetASC, DamageResult.FinalDamage, HitLocation, OwningSpec, DamageResult.bIsCritical, !bHasPerfectGuard);
}

// ────────────────────────────────────────────────────────────────────────────
//  방어 판정
// ────────────────────────────────────────────────────────────────────────────

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
	if (!bIsUnblockable && TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard))
	{
		constexpr float GuardDamageReductionRate = 0.5f;
		Result.FinalDamage *= GuardDamageReductionRate;
	}

	return Result;
}

// ────────────────────────────────────────────────────────────────────────────
//  피격 반응
// ────────────────────────────────────────────────────────────────────────────

void UWxExecCalc_Damage::ApplyHitReaction(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& OwningSpec, float FinalDamage, float TargetPP, bool bIsGuarding, bool bIsUnblockable, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FWxDamageStatics& Statics = GetDamageStatics();

	AActor* TargetActor = TargetASC->GetOwnerActor();
	FGameplayEventData EventData;
	EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	EventData.Target = TargetActor;

	// 공격 GE의 Event.HitReact.* 자식 태그로 HitReact 디스패치 종류 결정.
	// Spec에 명시된 태그만 사용하며, 없으면 HitReact 이벤트를 발송하지 않는다.
	// Knock 종류(Knockback/Knockdown/Knockup)는 PP 잔량과 무관하게 강제 발동.
	// Event.HitReact.Normal은 PP 소진 시에만 발동.
	const FGameplayTagContainer& DynamicTags = OwningSpec.GetDynamicAssetTags();
	const FGameplayTagContainer HitReactTagMatches = DynamicTags.Filter(FGameplayTagContainer(WxGameplayTags::Event_HitReact));

	const bool bHasHitReactTag = !HitReactTagMatches.IsEmpty();
	const FGameplayTag HitReactEventTag = bHasHitReactTag ? HitReactTagMatches.First() : FGameplayTag();
	const bool bForceHitReact = bHasHitReactTag && HitReactEventTag != WxGameplayTags::Event_HitReact_Normal;

	if (bIsGuarding && !bIsUnblockable)
	{
		// 일반 가드: SP 차감 → Guard 어빌리티가 GuardHitReact/GuardBreak 분기
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.SPProperty, EGameplayModOp::Additive, -FinalDamage));
		EventData.EventMagnitude = FinalDamage;

		// Knock 계열 공격을 가드했을 때는 GuardKnockback, 그 외에는 일반 GuardHit 이벤트를 디스패치한다.
		const FGameplayTag GuardHitEventTag = bForceHitReact
			? WxGameplayTags::Event_GuardHit_Knockback
			: WxGameplayTags::Event_GuardHit_Normal;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, GuardHitEventTag, EventData);
	}
	else
	{
		// Unblockable 가드 or 비가드: PP 차감
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.PPProperty, EGameplayModOp::Additive, -FinalDamage));

		if (bIsGuarding) // Unblockable 가드: Guard Cancel 후 HitReact
		{
			const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
			TargetASC->CancelAbilities(&GuardAbilityTags);
			if (bHasHitReactTag)
			{
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, HitReactEventTag, EventData);
			}
		}
		else // 비가드: Knock* 태그가 있거나 PP 소진 시 HitReact
		{
			if (bHasHitReactTag && (bForceHitReact || (TargetPP - FinalDamage) <= 0.f))
			{
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, HitReactEventTag, EventData);
			}
		}
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
