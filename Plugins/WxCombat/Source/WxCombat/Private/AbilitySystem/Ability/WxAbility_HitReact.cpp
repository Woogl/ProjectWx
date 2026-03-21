// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_HitReact.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "WxGameplayTags.h"

UWxAbility_HitReact::UWxAbility_HitReact()
{
	// AssetTag 의도적 미설정: 이벤트 트리거 전용 어빌리티이므로 입력/BT에서 태그 검색 대상이 아님
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(WxGameplayTags::ANS_Invincible);

	bRetriggerInstancedAbility = true;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UWxAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bWasGuardHitReact = ActorInfo->AbilitySystemComponent.IsValid()
		&& ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::ANS_Guard);
	UAnimMontage* MontageToPlay = bWasGuardHitReact ? GuardHitReactMontage : HitReactMontage;

	if (!MontageToPlay || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_HitReact::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_HitReact::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_HitReact::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_HitReact::HandleMontageCompleted()
{
	if (!bWasGuardHitReact)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_HitReact::HandleMontageBlendOut()
{
	if (bWasGuardHitReact)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UWxAbility_HitReact::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_HitReact::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
