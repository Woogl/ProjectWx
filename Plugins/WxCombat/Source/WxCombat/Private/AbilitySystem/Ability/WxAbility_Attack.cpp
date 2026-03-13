// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!AttackMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage);

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Attack::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Attack::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Attack::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Attack::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Attack::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Attack::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Attack::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Attack::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
