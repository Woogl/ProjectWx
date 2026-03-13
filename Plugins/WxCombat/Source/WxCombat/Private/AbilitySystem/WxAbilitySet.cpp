// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxAbilitySet.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Ability/WxGameplayAbility.h"
#include "AbilitySystem/WxAttributeSet.h"

void FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem(UWxAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : EffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	AbilitySpecHandles.Reset();
	EffectHandles.Reset();
}

void UWxAbilitySet::GiveToAbilitySystem(UWxAbilitySystemComponent* ASC, FWxAbilitySetGrantedHandles* OutHandles) const
{
	if (!ASC)
	{
		return;
	}
	
	// Effect 적용
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwner());

	for (const TSubclassOf<UGameplayEffect>& Effect : GrantedEffects)
	{
		if (!Effect)
		{
			continue;
		}

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1, Context);
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			if (OutHandles)
			{
				OutHandles->EffectHandles.Add(Handle);
			}
		}
	}

	// Ability 부여
	for (const TSubclassOf<UWxGameplayAbility>& Ability : GrantedAbilities)
	{
		if (!Ability)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Ability, 1);

		if (const UWxGameplayAbility* DefaultAbility = Ability.GetDefaultObject())
		{
			if (DefaultAbility->ActivationInputTag.IsValid())
			{
				Spec.GetDynamicSpecSourceTags().AddTag(DefaultAbility->ActivationInputTag);
			}
		}

		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		if (OutHandles)
		{
			OutHandles->AbilitySpecHandles.Add(Handle);
		}
	}
}
