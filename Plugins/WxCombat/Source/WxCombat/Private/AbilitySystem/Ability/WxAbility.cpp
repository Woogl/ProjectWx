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
	// ApplyCooldown에서 DynamicGrantedTags를 포함해 직접 GE를 적용하므로 nullptr을 반환해 Super::ApplyCooldown을 no-op으로 만든다.
	// CooldownGameplayEffectClass가 설정되어 있어도 무시한다.
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

		// 스택 1개 = 소모된 충전 1회. 스택 수가 MaxCharges 미만이면 사용 가능.
		const int32 ConsumedCharges = ASC->GetGameplayEffectCount(UWxEffect_Cooldown::StaticClass(), nullptr);
		return ConsumedCharges < MaxCharges;
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

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_Cooldown::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
	{
		SpecHandle.Data->SetDuration(CooldownDuration, true);
		SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}
