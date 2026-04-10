// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxAbility_Groggy::UWxAbility_Groggy()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::State_Groggy;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagPresent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_Groggy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GroggyMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	// PP를 0으로 설정
	ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetPPAttribute(), 0.f);

	// State.Groggy 태그 제거 감지 → 어빌리티 종료
	GroggyTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxAbility_Groggy::HandleGroggyTagChanged);

	// 다른 몽타주가 재생 중이면 그 몽타주가 끝날 때까지 대기한 뒤 그로기 몽타주를 재생한다.
	if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
	{
		if (AnimInstance->GetCurrentActiveMontage() != nullptr)
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &UWxAbility_Groggy::HandleMontageEnded);
		}
		else
		{
			PlayGroggyMontage();
		}
	}
	else
	{
		PlayGroggyMontage();
	}

	StartDPDrain();
}

void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// PP를 MaxPP로 회복
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (const UWxCombatAttributeSet* AttributeSet = ASC->GetSet<UWxCombatAttributeSet>())
		{
			ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetPPAttribute(), AttributeSet->GetMaxPP());
		}
	}

	// OnMontageEnded 델리게이트 정리
	if (ActorInfo)
	{
		if (UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &UWxAbility_Groggy::HandleMontageEnded);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		ActorInfo->OwnerActor->GetWorldTimerManager().ClearTimer(DPDrainTimerHandle);
	}

	if (GroggyTagDelegateHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
			.Remove(GroggyTagDelegateHandle);
		GroggyTagDelegateHandle.Reset();
	}
}

void UWxAbility_Groggy::HandleGroggyTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount == 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Groggy::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Groggy::HandleMontageInterrupted()
{
	// 다른 몽타주(HitReact 등)에 의해 중단된 경우, 해당 몽타주 종료 시 그로기 몽타주를 재재생한다.
	if (CurrentActorInfo && CurrentActorInfo->AbilitySystemComponent.IsValid()
		&& CurrentActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		if (UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.AddDynamic(this, &UWxAbility_Groggy::HandleMontageEnded);
		}
	}
}

void UWxAbility_Groggy::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Groggy::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 그로기 자신의 몽타주 blend out 완료는 무시한다.
	if (Montage == GroggyMontage)
	{
		return;
	}

	// 다른 몽타주에 의해 interrupt된 경우, 새 몽타주가 끝날 때까지 계속 대기한다.
	if (bInterrupted)
	{
		if (UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance())
		{
			if (AnimInstance->GetCurrentActiveMontage() != nullptr)
			{
				return;
			}
		}
	}

	if (UAnimInstance* AnimInstance = CurrentActorInfo->GetAnimInstance())
	{
		AnimInstance->OnMontageEnded.RemoveDynamic(this, &UWxAbility_Groggy::HandleMontageEnded);
	}

	PlayGroggyMontage();
}

void UWxAbility_Groggy::PlayGroggyMontage()
{
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, GroggyMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Groggy::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Groggy::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Groggy::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Groggy::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Groggy::StartDPDrain()
{
	if (!CurrentActorInfo || !CurrentActorInfo->OwnerActor.IsValid())
	{
		return;
	}

	constexpr float DrainTickRate = 1.f / 30.f;
	CurrentActorInfo->OwnerActor->GetWorldTimerManager().SetTimer(
		DPDrainTimerHandle, this, &UWxAbility_Groggy::HandleDPDrainTick, DrainTickRate, true);
}

void UWxAbility_Groggy::HandleDPDrainTick()
{
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return;
	}

	const UWxCombatAttributeSet* AttrSet = ASC->GetSet<UWxCombatAttributeSet>();
	if (!AttrSet)
	{
		return;
	}

	const float MaxDP = AttrSet->GetMaxDP();
	const float CurrentDP = AttrSet->GetDP();

	constexpr float DrainTickRate = 1.f / 30.f;
	const float DrainAmount = (MaxDP / GroggyDuration) * DrainTickRate;
	const float NewDP = FMath::Max(CurrentDP - DrainAmount, 0.f);

	ASC->ApplyModToAttribute(UWxCombatAttributeSet::GetDPAttribute(), EGameplayModOp::Additive, -DrainAmount);

	if (NewDP <= 0.f)
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Groggy, 1, EGameplayTagReplicationState::TagOnly);
	}
}
