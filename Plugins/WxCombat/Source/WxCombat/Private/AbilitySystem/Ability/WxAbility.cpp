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

UGameplayEffect* UWxAbility::GetCooldownGameplayEffect() const
{
	// Super(UGameplayAbility)에 CooldownGameplayEffectClass가 설정된 경우 그대로 사용
	if (UGameplayEffect* ParentCooldownGE = Super::GetCooldownGameplayEffect())
	{
		return ParentCooldownGE;
	}

	// ApplyCooldown에서 동적 Duration으로 직접 적용하므로 여기서는 nullptr 반환.
	// CDO를 반환하면 Super::ApplyCooldown이 기본 Duration(1초)으로 이중 적용하게 된다.
	return nullptr;
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
	if (MaxCharges > 1 && CooldownTag.IsValid())
	{
		const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (!ASC)
		{
			return false;
		}

		// GE 스택 1개 = 소모된 충전 1회. 스택 수가 MaxCharges 미만이면 사용 가능.
		const int32 ConsumedCharges = ASC->GetGameplayEffectCount(UWxEffect_Cooldown::StaticClass(), nullptr);
		return ConsumedCharges < MaxCharges;
	}

	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UWxAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.f || !CooldownTag.IsValid())
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetDuration(CooldownDuration, true);
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}
