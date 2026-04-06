// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "WxGameplayTags.h"

UWxAbility_Guard::UWxAbility_Guard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Guard);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);

	bRetriggerInstancedAbility = true;

	ActivationInputTag = WxGameplayTags::Input_Guard;
	CooldownTag = WxGameplayTags::Cooldown_Guard;
}

void UWxAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ActiveMontage == GuardBreakMontage || !GuardMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	ASC->AddLooseGameplayTag(WxGameplayTags::State_Guard);
	ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetPPAttribute()).AddUObject(this, &UWxAbility_Guard::HandlePPChanged);

	if (!PlayMontage(GuardMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ListenForHitReact();
}

void UWxAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetPPAttribute()).RemoveAll(this);
		if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
		}
	}

	ActiveMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Guard::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (ActiveMontage == GuardBreakMontage)
	{
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UWxAbility_Guard::PlayMontage(UAnimMontage* Montage)
{
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	ActiveMontage = Montage;

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Guard::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Guard::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Guard::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
	return true;
}

void UWxAbility_Guard::ListenForHitReact()
{
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_HitReact);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandleGuardHitReact);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Guard::HandleGuardHitReact(FGameplayEventData Payload)
{
	if (ActiveMontage != GuardMontage || !GuardHitReactMontage)
	{
		return;
	}

	PlayMontage(GuardHitReactMontage);
}

void UWxAbility_Guard::HandlePPChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue > 0.f || ActiveMontage == GuardBreakMontage)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
	}

	if (!PlayMontage(GuardBreakMontage))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Guard::HandleMontageCompleted()
{
	if (ActiveMontage == GuardHitReactMontage)
	{
		PlayMontage(GuardMontage);
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Guard::HandleMontageInterrupted()
{
	const UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Guard::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
