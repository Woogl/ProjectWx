// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/WxGA_MeleeAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxGA_MeleeAttack::UWxGA_MeleeAttack()
{
	ActivationInputTag = WxGameplayTags::Input_Attack;
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

void UWxGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!AttackMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage);

	MontageTask->OnCompleted.AddDynamic(this, &UWxGA_MeleeAttack::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxGA_MeleeAttack::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxGA_MeleeAttack::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxGA_MeleeAttack::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxGA_MeleeAttack::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxGA_MeleeAttack::HandleMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxGA_MeleeAttack::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxGA_MeleeAttack::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
