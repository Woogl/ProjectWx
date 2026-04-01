// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Death.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxAbility_Death::UWxAbility_Death()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::State_Dead;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 사망 몽타주가 없으면 즉시 래그돌 활성화
	if (!DeathMontage)
	{
		EnableRagdoll();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EnableRagdoll();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Death::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Death::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Death::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Death::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Death::HandleMontageCompleted()
{
	EnableRagdoll();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Death::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Death::HandleMontageInterrupted()
{
	EnableRagdoll();
}

void UWxAbility_Death::HandleMontageCancelled()
{
	EnableRagdoll();
}

void UWxAbility_Death::EnableRagdoll()
{
	UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (WxASC)
	{
		WxASC->MulticastEnableRagdoll();
	}
}
