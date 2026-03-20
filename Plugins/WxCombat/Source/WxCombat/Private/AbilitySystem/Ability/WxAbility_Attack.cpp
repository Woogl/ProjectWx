// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "AbilitySystem/Task/WxAbilityTask_RotateToTarget.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ComboMontages.IsEmpty() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentComboIndex = 0;
	RotateToTarget();
	PlayComboMontage();
}

void UWxAbility_Attack::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		return;
	}

	int32 NextIndex = CurrentComboIndex + 1;
	if (NextIndex >= ComboMontages.Num())
	{
		return;
	}

	CurrentComboIndex = NextIndex;
	RotateToTarget();
	PlayComboMontage();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CurrentComboIndex = 0;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Attack::PlayComboMontage()
{
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageCompleted);
		MontageTask->OnBlendOut.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageBlendOut);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageCancelled);
	}

	if (!ComboMontages.IsValidIndex(CurrentComboIndex) || !ComboMontages[CurrentComboIndex])
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboMontages[CurrentComboIndex]);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Attack::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Attack::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Attack::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Attack::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Attack::RotateToTarget()
{
	if (!TargetingPreset)
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetWorld());
	if (!TargetingSubsystem)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = AvatarActor;
	SourceContext.InstigatorActor = AvatarActor;

	FTargetingRequestHandle Handle = TargetingSubsystem->MakeTargetRequestHandle(TargetingPreset, SourceContext);
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(Handle);

	TArray<AActor*> Targets;
	TargetingSubsystem->GetTargetingResultsActors(Handle, Targets);
	TargetingSubsystem->ReleaseTargetRequestHandle(Handle);

	if (Targets.Num() > 0)
	{
		float RotationRateYaw = 360.f;
		if (const ACharacter* Character = Cast<ACharacter>(AvatarActor))
		{
			RotationRateYaw = Character->GetCharacterMovement()->RotationRate.Yaw;
		}

		UWxAbilityTask_RotateToTarget* RotateTask = UWxAbilityTask_RotateToTarget::CreateTask(this, Targets[0], RotationRateYaw);
		RotateTask->ReadyForActivation();
	}
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
