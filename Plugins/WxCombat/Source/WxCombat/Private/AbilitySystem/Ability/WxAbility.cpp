// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystemComponent.h"

UWxAbility::UWxAbility()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UWxAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}


const FGameplayTagContainer* UWxAbility::GetCooldownTags() const
{
	CooldownTagContainer.Reset();

	const FGameplayTagContainer* ParentTags = Super::GetCooldownTags();
	if (ParentTags)
	{
		CooldownTagContainer.AppendTags(*ParentTags);
	}

	if (CooldownTag.IsValid())
	{
		CooldownTagContainer.AddTag(CooldownTag);
	}

	return &CooldownTagContainer;
}

bool UWxAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (CooldownTag.IsValid())
	{
		const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (!ASC)
		{
			return false;
		}

		// 활성 쿨다운 GE 수(= 소모된 충전 수)가 MaxCharges 미만이면 사용 가능.
		return ASC->GetTagCount(CooldownTag) < MaxCharges;
	}

	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UWxAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);

	if (CooldownDuration <= 0.f || !CooldownTag.IsValid())
	{
		return;
	}

	// 기존 활성 쿨다운 GE 중 가장 늦게 만료되는 잔여 시간을 구해 순차 복구를 구현한다.
	// Duration = 기존 최대 잔여시간 + CooldownDuration
	// → 충전이 CooldownDuration 간격으로 순차 복구된다.
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(CooldownTag);
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyEffectTags(TagContainer);

	float MaxRemainingTime = 0.f;
	for (const float Remaining : ASC->GetActiveEffectsTimeRemaining(Query))
	{
		MaxRemainingTime = FMath::Max(MaxRemainingTime, Remaining);
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
	{
		SpecHandle.Data->SetDuration(MaxRemainingTime + CooldownDuration, true);
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
		SpecHandle.Data->DynamicAssetTags.AddTag(CooldownTag);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}
