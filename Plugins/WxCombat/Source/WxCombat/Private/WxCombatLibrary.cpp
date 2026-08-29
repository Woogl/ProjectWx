// Copyright Woogle. All Rights Reserved.

#include "WxCombatLibrary.h"
#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "GenericTeamAgentInterface.h"
#include "WxGameplayTags.h"
#include "Damage/WxDamageTableRow.h"

bool UWxCombatLibrary::IsHostile(const AActor* Source, const AActor* Target)
{
	const IGenericTeamAgentInterface* SourceTeamAgent = Cast<IGenericTeamAgentInterface>(Source);
	if (!SourceTeamAgent)
	{
		return false;
	}
	
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(Target);
	if (!TargetTeamAgent)
	{
		return false;
	}
	
	return SourceTeamAgent->GetTeamAttitudeTowards(*Target) == ETeamAttitude::Hostile;
}

EWxDamageCheck UWxCombatLibrary::CheckDamage(const UAbilitySystemComponent* Source, const UAbilitySystemComponent* Target)
{
	if (!Target)
	{
		return EWxDamageCheck::None;
	}

	// 대미지 GE가 사망 대상을 거르기 전, 히트스톱·투사체 연출도 시체를 제외해야 한다.
	if (Target->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
	{
		return EWxDamageCheck::None;
	}

	// 팀킬 방지
	const AActor* SourceAvatar = Source ? Source->GetAvatarActor() : nullptr;
	const AActor* TargetAvatar = Target->GetAvatarActor();
	if (!IsHostile(SourceAvatar, TargetAvatar))
	{
		return EWxDamageCheck::None;
	}

	if (Target->HasMatchingGameplayTag(WxGameplayTags::Effect_Invincible))
	{
		return EWxDamageCheck::Evaded;
	}

	return EWxDamageCheck::Damaged;
}

bool UWxCombatLibrary::ApplyDamage(AActor* Causer, const AActor* Target, const FDataTableRowHandle& DamageTableRow, const FHitResult& HitResult, float HitStopDuration)
{
	if (!Causer || !Target)
	{
		return false;
	}

	AActor* SourceActor = Causer;
	UAbilitySystemComponent* Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Causer);
	if (!Source)
	{
		SourceActor = Causer->GetOwner();
		Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!Source || !TargetASC)
	{
		return false;
	}

	const UGameplayAbility* AnimatingAbility = Source->GetAnimatingAbility();

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.AddInstigator(SourceActor, Causer);
	Context.SetAbility(AnimatingAbility);
	Context.AddHitResult(HitResult);

	// 노티파이는 활성화 스코프 밖이라 ASC의 ScopedPredictionKey가 무효다.
	FPredictionKey PredictionKey;
	if (AnimatingAbility)
	{
		PredictionKey = AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	// 적용 전에 판정한다 — 이 히트로 죽는 대상에도 히트스톱이 걸려야 한다.
	const EWxDamageCheck DamageCheck = CheckDamage(Source, TargetASC);

	if (DamageCheck == EWxDamageCheck::Evaded)
	{
		AActor* TargetActor = TargetASC->GetOwnerActor();

		FGameplayEventData EventData;
		EventData.Instigator = SourceActor;
		EventData.Target = TargetActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, WxGameplayTags::Event_DodgeSuccess, EventData);
	}

	// 흘려낸 히트에 GE를 걸면 히트 큐와 상태이상만 새어 나간다.
	if (DamageCheck != EWxDamageCheck::Damaged)
	{
		return false;
	}

	FWxDamageTableRow* DamageRow = DamageTableRow.GetRow<FWxDamageTableRow>(TEXT("GetDamageTableRow"));
	if (!DamageRow)
	{
		return false;
	}
	
	bool bDamageApplied = false;
	const bool bPerfectGuardApplied = DamageRow->bCanGuard && TargetASC->HasMatchingGameplayTag(WxGameplayTags::Effect_PerfectGuard);
	const TArray<FGameplayEffectSpecHandle> Specs = DamageRow->MakeSpecs(Source, Context);
	for (const FGameplayEffectSpecHandle& Spec : Specs)
	{
		const bool bIsDamageSpec = Spec.IsValid() && Spec.Data->Def->IsA<UWxEffect_Damage>();
		if (!bIsDamageSpec)
		{
			continue;
		}

		const FActiveGameplayEffectHandle AppliedHandle = Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC, PredictionKey);
		bDamageApplied = AppliedHandle.WasSuccessfullyApplied();
		break;
	}

	// Damage GE가 거부되거나 퍼펙트 가드가 성립하면 부가 효과를 적용하지 않는다.
	if (bDamageApplied && !bPerfectGuardApplied)
	{
		for (const FGameplayEffectSpecHandle& Spec : Specs)
		{
			const bool bIsDamageSpec = Spec.IsValid() && Spec.Data->Def->IsA<UWxEffect_Damage>();
			if (Spec.IsValid() && !bIsDamageSpec)
			{
				Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC, PredictionKey);
			}
		}
	}

	// 적용 뒤라야 동기로 도착한 반응(패리 등)에 히트스톱이 몽타주를 양보한다.
	if (HitStopDuration > 0.f)
	{
		if (UWxAbilitySystemComponent* SourceWxASC = Cast<UWxAbilitySystemComponent>(Source))
		{
			SourceWxASC->ApplyHitStop(HitStopDuration, AnimatingAbility);
		}
	}

	return bDamageApplied;
}

void UWxCombatLibrary::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, const UGameplayAbility* PredictingAbility)
{
	if (!TargetASC || !EffectClass)
	{
		return;
	}

	const UGameplayEffect* CDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	const float Level = PredictingAbility ? PredictingAbility->GetAbilityLevel() : 0.f;
	FGameplayEffectSpec Spec(CDO, TargetASC->MakeEffectContext(), Level);

	FPredictionKey PredictionKey;
	if (PredictingAbility)
	{
		PredictionKey = PredictingAbility->GetCurrentActivationInfo().GetActivationPredictionKey();
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec, PredictionKey);
}

void UWxCombatLibrary::ApplyAttributeChange(UAbilitySystemComponent* TargetASC, const FGameplayAttribute& Attribute, float Delta)
{
	UGameplayEffect* DynamicGE = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ApplyAttributeChange")));
	DynamicGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	int32 Idx = DynamicGE->Modifiers.Num();
	DynamicGE->Modifiers.SetNum(Idx + 1);

	FGameplayModifierInfo& InfoXP = DynamicGE->Modifiers[Idx];
	InfoXP.ModifierMagnitude = FScalableFloat(Delta);
	InfoXP.ModifierOp = EGameplayModOp::Additive;
	InfoXP.Attribute = Attribute;

	TargetASC->ApplyGameplayEffectToSelf(DynamicGE, 0.f, TargetASC->MakeEffectContext());
}
