// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Groggy.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystem/Effect/WxEffect_DrainDP.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
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

	ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetPPAttribute(), 0.f);

	GroggyTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxAbility_Groggy::HandleGroggyTagChanged);

	FGameplayEffectSpecHandle DrainSpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_DrainDP::StaticClass(), GetAbilityLevel());
	if (DrainSpecHandle.IsValid())
	{
		DrainDPEffectHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, DrainSpecHandle);
	}

	if (UWorld* World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
	{
		World->GetTimerManager().SetTimer(MontagePollingTimerHandle, this, &UWxAbility_Groggy::TickPlayMontage, 0.1f, true);
	}

	TickPlayMontage();
}

void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo)
	{
		if (UWorld* World = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(MontagePollingTimerHandle);
		}

		if (ActorInfo->AbilitySystemComponent.IsValid())
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
			if (const UWxCombatAttributeSet* AttributeSet = ASC->GetSet<UWxCombatAttributeSet>())
			{
				ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetPPAttribute(), AttributeSet->GetMaxPP());
			}

			ASC->StopMontageIfCurrent(*GroggyMontage);

			if (DrainDPEffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(DrainDPEffectHandle);
				DrainDPEffectHandle.Invalidate();
			}

			if (GroggyTagDelegateHandle.IsValid())
			{
				ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Groggy, EGameplayTagEventType::NewOrRemoved)
					.Remove(GroggyTagDelegateHandle);
				GroggyTagDelegateHandle.Reset();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Groggy::HandleGroggyTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount == 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_Groggy::TickPlayMontage()
{
	if (!CurrentActorInfo)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	if (ASC->GetCurrentMontage() != nullptr)
	{
		return;
	}

	ASC->PlayMontage(this, CurrentActivationInfo, GroggyMontage, 1.f);
}
