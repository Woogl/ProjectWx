// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Task/WxAbilityTask_TurnAround.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

UWxAbility_Dodge::UWxAbility_Dodge()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Dodge);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	ActivationInputTag = WxGameplayTags::Input_Dodge;
	CooldownTag = WxGameplayTags::Cooldown_Dodge;
}

void UWxAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 이동 입력 방향으로 부드럽게 회전
	if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		const FVector InputDirection = Character->GetLastMovementInputVector();
		if (!InputDirection.IsNearlyZero())
		{
			UWxAbilityTask_TurnAround* TurnAroundTask = UWxAbilityTask_TurnAround::CreateTask(
				this, InputDirection);
			TurnAroundTask->ReadyForActivation();
		}
	}

	if (!DodgeMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DodgeMontage);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Dodge::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Dodge::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Dodge::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Dodge::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
