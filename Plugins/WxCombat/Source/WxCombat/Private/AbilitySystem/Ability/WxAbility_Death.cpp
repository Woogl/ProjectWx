// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Death.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"

namespace
{
	constexpr float MaxHitReactWaitSeconds = 0.15f;
}

UWxAbility_Death::UWxAbility_Death()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

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

	// HitReact 몽타주 재생 중이면 자연 종료까지 대기한 뒤 래그돌 (최대 MaxHitReactWaitSeconds 초)
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::State_HitReact))
	{
		USkeletalMeshComponent* Mesh = ActorInfo->SkeletalMeshComponent.Get();
		if (UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr)
		{
			PendingWaitMontage = AnimInstance->GetCurrentActiveMontage();
			AnimInstance->OnMontageEnded.AddDynamic(this, &UWxAbility_Death::HandleActiveMontageEnded);
		}
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(WaitTimerHandle, this, &UWxAbility_Death::FinishWaitAndRagdoll, MaxHitReactWaitSeconds, false);
		}
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

void UWxAbility_Death::HandleActiveMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (PendingWaitMontage && Montage != PendingWaitMontage)
	{
		return;
	}

	FinishWaitAndRagdoll();
}

void UWxAbility_Death::FinishWaitAndRagdoll()
{
	USkeletalMeshComponent* Mesh = CurrentActorInfo ? CurrentActorInfo->SkeletalMeshComponent.Get() : nullptr;
	if (UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr)
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UWxAbility_Death::HandleActiveMontageEnded);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WaitTimerHandle);
	}

	PendingWaitMontage = nullptr;

	EnableRagdoll();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Death::EnableRagdoll()
{
	UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (WxASC)
	{
		WxASC->MulticastEnableRagdoll();
	}
}
