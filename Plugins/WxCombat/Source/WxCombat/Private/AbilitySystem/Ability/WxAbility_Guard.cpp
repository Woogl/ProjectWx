// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
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

	// bRetriggerInstancedAbility로 인해 어빌리티가 진행 중에 재진입되면 현재 페이즈를 유지한다.
	// 이벤트 리스너 중복 등록을 방지하고, GuardBreak 중 재진입 시 태스크를 보호한다.
	if (ActiveMontage)
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

float UWxAbility_Guard::GetDamageReductionRate() const
{
	return DamageReductionRate;
}

bool UWxAbility_Guard::PlayMontage(UAnimMontage* Montage)
{
	// 페이즈 전환 시 이전 몽타주 태스크를 명시적으로 정리해 콜백 잔여 발생을 차단한다.
	// HandleMontageBlendingOut 콜백 내에서 호출될 수 있으나, EndTask가 AnimInstance 바인딩을
	// 해제하므로 구 태스크의 OnInterrupted 등 후속 이벤트는 발송되지 않는다.
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
	// Event.HitReact 부모 태그로 등록하여 자식 태그(.Normal/.Knockback/.Knockdown/.Knockup)를 모두 수신한다.
	// HitReact 어빌리티는 ActivationBlockedTags=State.Guard 로 가드 중에는 활성화되지 않으므로 라우팅 충돌 없음.
	UAbilityTask_WaitGameplayEvent* HitReactTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_HitReact, nullptr, false, false);
	if (HitReactTask)
	{
		HitReactTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandleGuardHitReact);
		HitReactTask->ReadyForActivation();
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
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(UWxEffect_RecoverResource::StaticClass(), GetAbilityLevel());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_UP, 0.f);
		SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Recovery_MP, PerfectGuardMPRecovery);
		ApplyGameplayEffectSpecToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle);
	}

	// 퍼펙트 가드 성공 시 짧은 슬로우 타임 연출
	if (UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectGuardSlowTimeDilation, PerfectGuardSlowTimeDuration))
	{
		SlowTimeTask->ReadyForActivation();
	}

	// GuardMontage 페이즈에서만 GuardHitReactMontage를 재생한다.
	// HitReact/Knockback 재생 중 퍼펙트 가드 이벤트가 오면 MP 회복만 처리하고 몽타주는 전환하지 않는다.
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
	else
	{
		// Knock 계열(Knockback/Knockdown/Knockup)이면 GuardKnockback, 그 외엔 GuardHitReact 재생.
		const bool bIsKnockHit = Payload.EventTag.IsValid() && Payload.EventTag != WxGameplayTags::Event_HitReact_Normal;

		if (bIsKnockHit && GuardKnockbackMontage)
		{
			PlayMontage(GuardKnockbackMontage);
		}
		else if (GuardHitReactMontage)
		{
			PlayMontage(GuardHitReactMontage);
		}
	}
}

void UWxAbility_Guard::HandleMontageBlendingOut()
{
	if (ActiveMontage == GuardHitReactMontage || ActiveMontage == GuardKnockbackMontage)
	{
		PlayMontage(GuardMontage);
	}
}

void UWxAbility_Guard::HandleMontageCompleted()
{
	// GuardMontage는 루핑 몽타주이므로 OnCompleted가 발생하지 않는다.
	if (ActiveMontage == GuardMontage)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Guard::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Guard::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
