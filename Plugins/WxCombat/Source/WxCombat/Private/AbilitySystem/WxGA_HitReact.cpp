// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/WxGA_HitReact.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxGA_HitReact::UWxGA_HitReact()
{
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HitReactMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, HitReactMontage);

	MontageTask->OnCompleted.AddDynamic(this, &UWxGA_HitReact::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxGA_HitReact::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxGA_HitReact::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxGA_HitReact::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxGA_HitReact::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxGA_HitReact::HandleMontageBlendOut()
{
}

void UWxGA_HitReact::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxGA_HitReact::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
