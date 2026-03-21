// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Sprint.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/WxAttributeSet.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

UWxAbility_Sprint::UWxAbility_Sprint()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Sprint);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	ActivationInputTag = WxGameplayTags::Input_Sprint;

	// SPD += SprintSpeedBonus (Infinite, ActivateAbility에서 적용 / EndAbility에서 제거)
	SprintSpeedEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("SprintSpeedEffect")));
	SprintSpeedEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UWxAttributeSet::GetSPDAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(SprintSpeedBonus));
	SprintSpeedEffect->Modifiers.Add(Modifier);
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

	SpeedEffectHandle = ASC->ApplyGameplayEffectToSelf(SprintSpeedEffect, 1.f, ASC->MakeEffectContext());
}

void UWxAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (SpeedEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(SpeedEffectHandle);
		SpeedEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
