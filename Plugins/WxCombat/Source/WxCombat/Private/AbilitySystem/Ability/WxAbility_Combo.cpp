// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Combo.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

UAnimMontage* UWxAbility_Combo::GetMontage() const
{
	const int32 MontageIndex = ComboIndex == INDEX_NONE ? 0 : ComboIndex;
	return ComboMontages.IsValidIndex(MontageIndex) ? ComboMontages[MontageIndex].Get() : nullptr;
}

float UWxAbility_Combo::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

void UWxAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboIndex = ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;

	if (!PlayMontage(GetMontage()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Combo::HandleMontageCompleted()
{
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}
