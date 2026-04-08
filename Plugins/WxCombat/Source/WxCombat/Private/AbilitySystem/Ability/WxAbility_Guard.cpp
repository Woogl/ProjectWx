// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoveryMP.h"
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
}

void UWxAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// GuardBreak 재생 중에는 bRetriggerInstancedAbility로 인한 재진입을 무시한다.
	// EndAbility를 호출하면 진행 중인 GuardBreak 태스크가 함께 종료되므로 단순 early-return.
	if (ActiveMontage == GuardBreakMontage)
	{
		return;
	}

	if (!GuardMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
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

	if (!PlayMontage(GuardMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ListenForGuardHit();
	ListenForPerfectGuard();
}

void UWxAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::State_Guard))
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
	}

	ActiveMontage = nullptr;
	CurrentMontageTask = nullptr;

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
	// 페이즈 전환 시 이전 몽타주 태스크를 명시적으로 정리해 콜백 잔여 발생을 차단한다.
	if (CurrentMontageTask)
	{
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	CurrentMontageTask = MontageTask;
	ActiveMontage = Montage;

	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Guard::HandleMontageBlendingOut);
	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Guard::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Guard::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Guard::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
	return true;
}

void UWxAbility_Guard::ListenForGuardHit()
{
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_GuardHit);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandleGuardHitReact);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Guard::ListenForPerfectGuard()
{
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_PerfectGuard);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandlePerfectGuard);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Guard::HandlePerfectGuard(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// 퍼펙트 가드 성공 보상: MP 회복
	const UGameplayEffect* Effect = UWxEffect_RecoveryMP::StaticClass()->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.f);
	Spec.SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery, PerfectGuardMPRecovery);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// 퍼펙트 가드 성공 시 GuardHitReact 몽타주를 재생한다.
	if (ActiveMontage == GuardMontage && GuardHitReactMontage)
	{
		PlayMontage(GuardHitReactMontage);
	}
}

void UWxAbility_Guard::HandleGuardHitReact(FGameplayEventData Payload)
{
	if (ActiveMontage != GuardMontage)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UWxCombatAttributeSet* AttributeSet = ASC ? ASC->GetSet<UWxCombatAttributeSet>() : nullptr;

	// Payload.EventMagnitude는 ExecCalc가 전달한 SP 차감량.
	// ExecCalc는 SP OutputModifier를 큐잉한 직후 동기적으로 이벤트를 디스패치하므로
	// 이 시점의 GetSP()는 차감 적용 전 값이며, (GetSP() - Magnitude)가 차감 후 예상 SP다.
	const bool bWillBreak = AttributeSet && (AttributeSet->GetSP() - Payload.EventMagnitude) <= 0.f;

	if (bWillBreak)
	{
		if (ASC)
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Guard);
		}

		if (!PlayMontage(GuardBreakMontage))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
	}
	else if (GuardHitReactMontage)
	{
		PlayMontage(GuardHitReactMontage);
	}
}

void UWxAbility_Guard::HandleMontageBlendingOut()
{
	if (ActiveMontage == GuardHitReactMontage)
	{
		PlayMontage(GuardMontage);
	}
}

void UWxAbility_Guard::HandleMontageCompleted()
{
	if (ActiveMontage == GuardMontage)
	{
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
