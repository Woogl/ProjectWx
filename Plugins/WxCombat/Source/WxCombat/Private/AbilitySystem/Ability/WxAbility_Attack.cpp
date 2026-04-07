// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "AbilitySystem/Effect/WxEffect_RecoveryMP.h"
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

	if (!ComboMap.Contains(FName(*CurrentPath)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayComboMontage();
	ListenForAttackHit();
}

void UWxAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

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

	UAnimMontage* Montage = ComboMap.FindRef(FName(*CurrentPath));
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

	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	WaitInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitInputTask->OnPress.AddDynamic(this, &UWxAbility_Attack::HandleComboInputPressed);
	WaitInputTask->ReadyForActivation();
}

bool UWxAbility_Attack::HasNextCombo() const
{
	return ComboMap.Contains(FName(*(CurrentPath + TEXT("L")))) || ComboMap.Contains(FName(*(CurrentPath + TEXT("H"))));
}

void UWxAbility_Attack::HandleComboInputPressed(float TimeWaited)
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (!ASC)
	{
		return;
	}

	const TCHAR* Suffix = (ASC->GetLastPressedInputTag() == WxGameplayTags::Input_Attack_Heavy)
		? TEXT("H")
		: TEXT("L");

	if (!TryAdvanceCombo(Suffix))
	{
		WaitForComboInput();
	}
}

bool UWxAbility_Attack::TryAdvanceCombo(const TCHAR* Suffix)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		return false;
	}

	const FString NextPath = CurrentPath + Suffix;
	if (!ComboMap.Contains(FName(*NextPath)))
	{
		return false;
	}

	CurrentPath = NextPath;
	PlayComboMontage();
	return true;
}

void UWxAbility_Attack::ListenForAttackHit()
{
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_AttackHit);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Attack::HandleAttackHit);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Attack::HandleAttackHit(FGameplayEventData Payload)
{
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_RecoveryMP::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery, HitMPRecovery);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
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
