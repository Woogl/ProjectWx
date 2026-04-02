// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Attack::UWxAbility_Attack()
{
	ActivationInputTag = WxGameplayTags::Input_Attack;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Attack);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Attack);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Attack);
}

void UWxAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 런타임 룩업 맵 빌드
	if (ComboMap.IsEmpty())
	{
		for (const FWxComboEntry& Entry : ComboEntries)
		{
			if (Entry.Montage)
			{
				ComboMap.Add(Entry.Path, Entry.Montage);
			}
		}
	}

	// 첫 입력 종류 판별
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (ASC && ASC->GetLastPressedInputTag() == WxGameplayTags::Input_Attack_Heavy)
	{
		CurrentPath = TEXT("H");
	}
	else
	{
		CurrentPath = TEXT("L");
	}

	if (!FindMontage(CurrentPath))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayComboMontage();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CleanupComboInputTasks();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CurrentPath.Empty();
}

void UWxAbility_Attack::PlayComboMontage()
{
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageCompleted);
		MontageTask->OnBlendOut.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageBlendOut);
		MontageTask->OnInterrupted.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &UWxAbility_Attack::HandleMontageCancelled);
		MontageTask->EndTask();
	}

	UAnimMontage* Montage = FindMontage(CurrentPath);
	if (!Montage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
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

	WaitForComboInput();
}

void UWxAbility_Attack::WaitForComboInput()
{
	if (!HasNextCombo())
	{
		return;
	}

	CleanupComboInputTasks();

	WaitLightInputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_Combo_Light);
	WaitLightInputTask->EventReceived.AddDynamic(this, &UWxAbility_Attack::HandleComboLightInput);
	WaitLightInputTask->ReadyForActivation();

	WaitHeavyInputTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_Combo_Heavy);
	WaitHeavyInputTask->EventReceived.AddDynamic(this, &UWxAbility_Attack::HandleComboHeavyInput);
	WaitHeavyInputTask->ReadyForActivation();
}

void UWxAbility_Attack::CleanupComboInputTasks()
{
	if (WaitLightInputTask)
	{
		WaitLightInputTask->EndTask();
		WaitLightInputTask = nullptr;
	}

	if (WaitHeavyInputTask)
	{
		WaitHeavyInputTask->EndTask();
		WaitHeavyInputTask = nullptr;
	}
}

bool UWxAbility_Attack::HasNextCombo() const
{
	return FindMontage(CurrentPath + TEXT("L")) || FindMontage(CurrentPath + TEXT("H"));
}

UAnimMontage* UWxAbility_Attack::FindMontage(const FString& InPath) const
{
	const TObjectPtr<UAnimMontage>* Found = ComboMap.Find(InPath);
	return Found ? Found->Get() : nullptr;
}

void UWxAbility_Attack::HandleComboLightInput(FGameplayEventData Payload)
{
	TryAdvanceCombo(TEXT("L"));
}

void UWxAbility_Attack::HandleComboHeavyInput(FGameplayEventData Payload)
{
	TryAdvanceCombo(TEXT("H"));
}

bool UWxAbility_Attack::TryAdvanceCombo(const FString& Suffix)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		return false;
	}

	const FString NextPath = CurrentPath + Suffix;
	if (!FindMontage(NextPath))
	{
		return false;
	}

	CurrentPath = NextPath;
	PlayComboMontage();
	return true;
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
