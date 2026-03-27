// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Sprint.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"
#include "AbilitySystem/Effect/WxEffect_Sprint.h"

UWxAbility_Sprint::UWxAbility_Sprint()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Sprint);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	ActivationInputTag = WxGameplayTags::Input_Sprint;
	
	SprintEffectClass = UWxEffect_Sprint::StaticClass();
}

void UWxAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SprintEffectClass)
	{
		return;
	}

	UGameplayEffect* EffectCDO = SprintEffectClass->GetDefaultObject<UGameplayEffect>();
	SpeedEffectHandle = ASC->ApplyGameplayEffectToSelf(EffectCDO, 1.f, ASC->MakeEffectContext());
}

void UWxAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	if (SpeedEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
    {
    	ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(SpeedEffectHandle);
    	SpeedEffectHandle.Invalidate();
    }
}

void UWxAbility_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
