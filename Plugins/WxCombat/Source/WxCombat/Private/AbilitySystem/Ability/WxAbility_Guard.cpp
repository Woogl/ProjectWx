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

	if (IsPlayingGuardBreakMontage() || !GuardMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 어빌리티 활성 동안 ANS_Guard 유지 (몽타주 전환과 무관하게 가드 판정 보장)
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::ANS_Guard);
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetPPAttribute()).AddUObject(this, &UWxAbility_Guard::HandlePPChanged);

	PlayGuardMontage();
	ListenForHitReact();
}

void UWxAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetPPAttribute()).RemoveAll(this);
		ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_Guard);
	}

	bPlayingHitReact = false;
	bMontageSwapInProgress = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Guard::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (IsPlayingGuardBreakMontage())
	{
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ────────────────────────────────────────────────────────────────────────────
//  몽타주 관리
// ────────────────────────────────────────────────────────────────────────────

bool UWxAbility_Guard::IsPlayingGuardBreakMontage() const
{
	const UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	return AnimInstance && GuardBreakMontage && AnimInstance->Montage_IsPlaying(GuardBreakMontage);
}

void UWxAbility_Guard::PlayGuardMontage()
{
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, GuardMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Guard::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Guard::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Guard::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Guard::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UWxAbility_Guard::PlayGuardBreakMontage()
{
	// 가드 해제
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::ANS_Guard);
	}

	if (!GuardBreakMontage)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	bMontageSwapInProgress = true;

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, GuardBreakMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Guard::HandleMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Guard::HandleMontageBlendOut);
		MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Guard::HandleMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Guard::HandleMontageCancelled);
		MontageTask->ReadyForActivation();
	}
}

// ────────────────────────────────────────────────────────────────────────────
//  가드 중 피격 처리
// ────────────────────────────────────────────────────────────────────────────

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
	if (!GuardHitReactMontage)
	{
		return;
	}

	bMontageSwapInProgress = true;
	bPlayingHitReact = true;

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, GuardHitReactMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Guard::HandleMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Guard::HandleMontageBlendOut);
		MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Guard::HandleMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Guard::HandleMontageCancelled);
		MontageTask->ReadyForActivation();
	}

	// 다음 피격 이벤트 즉시 대기 (연속 피격 대응)
	ListenForHitReact();
}

// ────────────────────────────────────────────────────────────────────────────
//  몽타주 콜백
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Guard::HandleMontageCompleted()
{
	if (bPlayingHitReact)
	{
		bPlayingHitReact = false;
		PlayGuardMontage();
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Guard::HandleMontageBlendOut()
{
	if (bPlayingHitReact)
	{
		bPlayingHitReact = false;
		PlayGuardMontage();
	}
}

void UWxAbility_Guard::HandleMontageInterrupted()
{
	if (bMontageSwapInProgress)
	{
		bMontageSwapInProgress = false;
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Guard::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Guard::HandlePPChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue <= 0.f)
	{
		PlayGuardBreakMontage();
	}
}
