// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystem/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// HitReact는 항상 서버의 ExecCalc에서 GameplayEvent로 트리거되므로 ServerInitiated를 사용한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::ANS_Invincible);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Guard);

	bRetriggerInstancedAbility = true;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 플레이어가 아니고 그로기 상태가 아니면 PP를 MaxPP만큼 회복
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC && !ActorInfo->PlayerController.IsValid() && !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Groggy))
	{
		if (const UWxCombatAttributeSet* AttributeSet = ASC->GetSet<UWxCombatAttributeSet>())
		{
			ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetPPAttribute(), AttributeSet->GetMaxPP());
		}
	}

	if (!HitReactMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, HitReactMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_HitReact::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_HitReact::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_HitReact::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
