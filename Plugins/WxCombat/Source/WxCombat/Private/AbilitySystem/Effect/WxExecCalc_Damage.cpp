// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Effect/WxEffect_Reflect.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
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

	// --- 2. 상태 판정 ---
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bHasPerfectGuard = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_PerfectGuard);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);

	// Unblockable 공격은 퍼펙트 가드를 포함한 모든 가드를 무시한다.
	const bool bPerfectGuardApplied = bHasPerfectGuard && !bIsUnblockable;

	// --- 3. 대미지 계산 ---
	// 퍼펙트 가드 시 반사량 산출을 위해 크리 스킵. CalcDamage 내부에서 Raw 모드도 자동으로 크리 스킵.
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = OwningSpec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = OwningSpec.CapturedTargetTags.GetAggregatedTags();

	FWxDamageResult DamageResult = CalcDamage(ExecutionParams, EvalParams, bPerfectGuardApplied);

	// --- 4. 대미지 적용 ---
	if (bPerfectGuardApplied)
	{
		ReflectPerfectGuard(SourceASC, DamageResult.FinalDamage);

		FGameplayEventData EventData;
		EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
		EventData.Target = TargetASC->GetOwnerActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetOwnerActor(), WxGameplayTags::Event_PerfectGuard, EventData);

		// 패리 피격: 공격자에게 HitReact 송출
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
		// 가드 감소: Unblockable이 아닌 가드 상태에서 Guard 어빌리티의 DamageReductionRate 만큼 감소
		if (!bIsUnblockable && bIsGuarding)
		{
			TArray<FGameplayAbilitySpec*> GuardSpecs;
			TargetASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(WxGameplayTags::Ability_Guard), GuardSpecs);
			if (GuardSpecs.Num() > 0)
			{
				if (const UWxAbility_Guard* Guard = Cast<UWxAbility_Guard>(GuardSpecs[0]->GetPrimaryInstance()))
				{
					DamageResult.FinalDamage *= Guard->GetDamageReductionRate();
				}
			}
		}

		if (DamageResult.FinalDamage > 0.f)
		{
			const FWxDamageStatics& Statics = GetDamageStatics();
			OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.IncomingDamageProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));

			if (!TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
			{
				OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.DPProperty, EGameplayModOp::Additive, DamageResult.FinalDamage));
			}

			ApplyHitReaction(ExecutionParams, OutExecutionOutput, DamageResult.FinalDamage);

			const float RecoveryUP = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, false, 0.f);
			const float RecoveryMP = OwningSpec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, false, 0.f);
			UWxEffect_RecoverResource::ApplyTo(SourceASC, RecoveryUP, RecoveryMP);
		}
	}

	// --- 5. AI 대미지 감지 ---
	AActor* SourceActor = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	AActor* TargetActor = TargetASC->GetOwnerActor();
	if (SourceActor && TargetActor)
	{
		// 퍼펙트 가드 성공 시 Target에 실제 대미지가 없으므로 0으로 보고
		const float ReportedDamage = bPerfectGuardApplied ? 0.f : DamageResult.FinalDamage;
		UAISense_Damage::ReportDamageEvent(TargetActor->GetWorld(), TargetActor, SourceActor, ReportedDamage, SourceActor->GetActorLocation(), TargetActor->GetActorLocation());
	}

	// --- 6. GameplayCue ---
	// 퍼펙트 가드 시 PerfectGuard 큐, 그 외 대미지 발생 시 Damage 큐 발행.
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

FWxDamageResult UWxExecCalc_Damage::CalcDamage(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FAggregatorEvaluateParameters& EvalParams, bool bSkipCrit) const
{
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();
	const FWxDamageStatics& Statics = GetDamageStatics();

	// SetByCaller.RawDamage 양수면 ATK/DEF/Coeff 우회 환경 대미지 모드. 그 외에는 ATK·DEF 공식 적용.
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

	// Raw 모드와 외부 요청(퍼펙트 가드 등)에서 크리 스킵
	if (bRawMode || bSkipCrit)
	{
		return Result;
	}

	float SourceCritRate = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritRateDef, EvalParams, SourceCritRate);

	float SourceCritDMG = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDMGDef, EvalParams, SourceCritDMG);

	// 치명타: CritRate 1당 1%, 치명타 시 (1 + CritDMG * 0.01) 배율
	const float CritChance = FMath::Clamp(SourceCritRate * 0.01f, 0.f, 1.f);
	Result.bIsCritical = FMath::FRand() < CritChance;
	if (Result.bIsCritical)
	{
		Result.FinalDamage *= (1.f + SourceCritDMG * 0.01f);
	}

	return Result;
}

// ────────────────────────────────────────────────────────────────────────────
//  피격 반응
// ────────────────────────────────────────────────────────────────────────────

void UWxExecCalc_Damage::ApplyHitReaction(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput, float FinalDamage) const
{
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	const FGameplayEffectSpec& OwningSpec = ExecutionParams.GetOwningSpec();

	const FWxDamageStatics& Statics = GetDamageStatics();
	const bool bIsUnblockable = OwningSpec.GetDynamicAssetTags().HasTag(WxGameplayTags::Damage_Unblockable);
	const bool bIsGuarding = TargetASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard);
	const bool bGuardHit = bIsGuarding && !bIsUnblockable;

	// --- 자원 차감 ---
	// 일반 가드는 SP 를 차감한다. 그 외(Unblockable 가드/비가드)는 자원 차감 없음.
	if (bGuardHit)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Statics.SPProperty, EGameplayModOp::Additive, -FinalDamage));
	}

	// --- 이벤트 태그 결정 ---
	// 공격 GE의 Event.HitReact.* 자식 태그를 추출. 태그가 비어있으면 이벤트 미발송.
	const FGameplayTagContainer HitReactMatches = OwningSpec.GetDynamicAssetTags().Filter(FGameplayTagContainer(WxGameplayTags::Event_HitReact));
	const FGameplayTag HitReactTag = HitReactMatches.IsEmpty() ? FGameplayTag() : HitReactMatches.First();

	FGameplayTag EventTag;
	if (bGuardHit)
	{
		// 일반 가드: HitReact 태그를 그대로 송출. Guard 어빌리티가 Knock 계열 vs Normal 분기 처리.
		// 명시 태그가 없으면 가드 흡수 애니메이션을 위해 Normal 로 폴백.
		EventTag = HitReactTag.IsValid() ? HitReactTag : WxGameplayTags::Event_HitReact_Normal;
	}
	else if (bIsGuarding)
	{
		// Unblockable 가드: Guard 어빌리티 Cancel 후 명시된 HitReact 송출
		const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
		TargetASC->CancelAbilities(&GuardAbilityTags);
		EventTag = HitReactTag;
	}
	else
	{
		// 비가드: 명시된 HitReact 송출 (태그가 없으면 이벤트 미발송)
		EventTag = HitReactTag;
	}

	if (!EventTag.IsValid())
	{
		return;
	}

	// 타겟이 Ability.Pattern 발동 중이면 Ability.Attack(일반 공격)으로는 경직 무효
	if (TargetASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Pattern))
	{
		if (const UGameplayAbility* SourceAbility = OwningSpec.GetContext().GetAbility())
		{
			if (SourceAbility->GetAssetTags().HasTag(WxGameplayTags::Ability_Attack))
			{
				return;
			}
		}
	}

	// --- 이벤트 송출 ---
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	AActor* TargetActor = TargetASC->GetOwnerActor();
	FGameplayEventData EventData;
	EventData.Instigator = SourceASC ? SourceASC->GetOwnerActor() : nullptr;
	EventData.Target = TargetActor;
	EventData.EventMagnitude = FinalDamage;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, EventData);
}

// ────────────────────────────────────────────────────────────────────────────
//  GameplayCue
// ────────────────────────────────────────────────────────────────────────────

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
