// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_DodgeSuccess.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_DodgeSuccess::UWxAbility_DodgeSuccess()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_DodgeSuccess;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_DodgeSuccess::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!DodgeSuccessMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 공격 입력 태그를 스펙에 동적 추가하여, ASC의 입력 라우팅이 이 어빌리티에 도달하도록 함
	if (DodgeCounterMontage)
	{
		FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec)
		{
			Spec->GetDynamicSpecSourceTags().AddTag(WxGameplayTags::Input_Attack);
			ActorInfo->AbilitySystemComponent->MarkAbilitySpecDirty(*Spec);
		}
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DodgeSuccessMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 반격 몽타주가 설정된 경우에만 입력 대기
	if (DodgeCounterMontage)
	{
		WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
		WaitInputTask->OnPress.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleCounterInputPressed);
		WaitInputTask->ReadyForActivation();
	}
}

void UWxAbility_DodgeSuccess::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	// 동적으로 추가한 공격 입력 태그 제거
	if (DodgeCounterMontage && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec)
		{
			Spec->GetDynamicSpecSourceTags().RemoveTag(WxGameplayTags::Input_Attack);
			ActorInfo->AbilitySystemComponent->MarkAbilitySpecDirty(*Spec);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_DodgeSuccess::HandleCounterInputPressed(float TimeWaited)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// ANS_ComboWindow 구간 내에서만 반격 허용
	if (!ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		// 콤보 윈도우 밖이면 다시 입력 대기
		WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
		WaitInputTask->OnPress.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleCounterInputPressed);
		WaitInputTask->ReadyForActivation();
		return;
	}

	PlayDodgeCounterMontage();
}

void UWxAbility_DodgeSuccess::PlayDodgeCounterMontage()
{
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageCompleted);
		MontageTask->OnBlendOut.RemoveDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageBlendOut);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageCancelled);
		MontageTask->EndTask();
	}

	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DodgeCounterMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_DodgeSuccess::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_DodgeSuccess::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_DodgeSuccess::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_DodgeSuccess::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_DodgeSuccess::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
