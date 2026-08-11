// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "WxGameplayTags.h"

UWxAbility_Guard::UWxAbility_Guard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Guard);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	bRetriggerInstancedAbility = true;
}

void UWxAbility_Guard::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// 스스로 끝나야 하는 페이즈는 입력 릴리즈로 끊지 않는다.
	// - GuardBreak: 가드 깨짐 연출 완주 보장
	// - PerfectGuard: 가드 키를 떼도 State.Guard를 남겨 반격 윈도우 보존
	if (ActiveMontage == GuardBreakMontage || ActiveMontage == PerfectGuardMontage)
	{
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Guard::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 패링 연출 중 가드를 재입력하면 연출을 끊고 즉시 가드 자세로 복귀한다.
	// State.Guard는 패링 중에도 유지되므로 GuardMontage만 다시 재생하면 가드가 이어진다.
	if (ActiveMontage == PerfectGuardMontage && GuardMontage)
	{
		PlayMontage(GuardMontage);
	}
}

float UWxAbility_Guard::GetDamageReductionRate() const
{
	return DamageReductionRate;
}

void UWxAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 재발동으로 재진입하면 현재 페이즈를 그대로 유지한다.
	// 이벤트 리스너 중복 등록을 막고, GuardBreak 중 재진입에서 태스크를 보호한다.
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

	// 취소로 ANS_PerfectGuard의 NotifyEnd가 스킵되면 State.PerfectGuard가 잔존해 영구 패링이 된다.
	// 이 태그는 ANS_PerfectGuard만 부여하므로 이 시점의 잔존분은 가드가 흘린 것이다.
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::State_PerfectGuard))
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_PerfectGuard);
	}

	ActiveMontage = nullptr;
	CurrentMontageTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWxAbility_Guard::PlayMontage(UAnimMontage* Montage)
{
	// 페이즈 몽타주는 전부 선택적이다.
	// 널이면 태스크가 즉시 OnCancelled를 쏘므로 성공으로 돌려선 안 된다.
	if (!Montage)
	{
		return false;
	}

	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
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
	// 부모 태그로 등록해 자식 태그(.Normal/.KnockBack/.KnockDown/.KnockUp)를 모두 수신한다.
	// HitReact 어빌리티는 ActivationBlockedTags=State.Guard라 가드 중엔 뜨지 않으므로 라우팅 충돌이 없다.
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

void UWxAbility_Guard::HandleGuardHitReact(FGameplayEventData Payload)
{
	if (ActiveMontage != GuardMontage)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UWxCombatAttributeSet* AttributeSet = ASC ? ASC->GetSet<UWxCombatAttributeSet>() : nullptr;

	// 이벤트는 대미지 GE 적용이 끝난 뒤에 오므로 GetSP()가 이미 차감된 실제 값이다.
	const bool bGuardBroken = AttributeSet && AttributeSet->GetSP() <= 0.f;

	if (bGuardBroken)
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
		// Normal이 아닌 HitReact 자식 태그는 전부 Knock 계열이다.
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

void UWxAbility_Guard::HandlePerfectGuard(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	UWxEffect_RecoverResource::ApplyTo(ASC, 0.f, PerfectGuardMPRecovery);

	if (UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectGuardSlowTimeDilation, PerfectGuardSlowTimeDuration))
	{
		SlowTimeTask->ReadyForActivation();
	}

	// HitReact·Knockback 재생 중에 이벤트가 오면 보상만 주고 몽타주는 전환하지 않는다.
	if (ActiveMontage == GuardMontage && PerfectGuardMontage)
	{
		PlayMontage(PerfectGuardMontage);
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
