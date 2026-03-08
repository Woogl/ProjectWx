// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/WxGA_MeleeAttack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/WxCharacterBase.h"
#include "Weapon/WxWeaponBase.h"
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

	MontageTask->OnCompleted.AddDynamic(this, &UWxGA_MeleeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxGA_MeleeAttack::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxGA_MeleeAttack::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxGA_MeleeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxGA_MeleeAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AWxCharacterBase* Character = Cast<AWxCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (AWxWeaponBase* Weapon = Character->GetEquippedWeapon())
		{
			Weapon->SetWeaponCollisionEnabled(false);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxGA_MeleeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxGA_MeleeAttack::OnMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxGA_MeleeAttack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxGA_MeleeAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
