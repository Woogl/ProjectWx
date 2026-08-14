// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_Guard.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "WxGameplayTags.h"

UWxAbility_Guard::UWxAbility_Guard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Guard);
	AssetTags.AddTag(WxGameplayTags::Ability_Exclusive);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Guard);
	ActivationOwnedEffects.Add(UWxEffect_Guard::StaticClass());

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);

	// 자기 애셋 태그를 스스로 막는다. 엔진 블록 경로에는 취소와 달리 self-exception이 없어
	// 활성 중 재발동이 전부 차단되며, 이것이 가드가 페이즈를 유지하는 방식이다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Exclusive);
}

void UWxAbility_Guard::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// 스스로 끝나야 하는 페이즈는 입력 릴리즈로 끊지 않는다.
	// - GuardBreak: 가드 깨짐 연출 완주 보장
	// - PerfectGuard: 가드 키를 떼도 Effect.Guard를 남겨 반격 윈도우 보존
	UAnimMontage* PhaseMontage = GetActiveMontage();
	if (PhaseMontage == GuardBreakMontage || PhaseMontage == PerfectGuardMontage)
	{
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

float UWxAbility_Guard::GetDamageReductionRate() const
{
	return DamageReductionRate;
}

float UWxAbility_Guard::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!GuardMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!PlayMontage(GuardMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ListenForGuardHit();
	ListenForPerfectGuard();
}

void UWxAbility_Guard::HandleMontageBlendOut()
{
	UAnimMontage* PhaseMontage = GetActiveMontage();
	if (PhaseMontage == GuardHitReactMontage || PhaseMontage == GuardKnockbackMontage)
	{
		PlayMontage(GuardMontage);
	}
}

void UWxAbility_Guard::HandleMontageCompleted()
{
	// GuardMontage는 루핑 몽타주이므로 OnCompleted가 발생하지 않는다.
	if (GetActiveMontage() == GuardMontage)
	{
		return;
	}

	Super::HandleMontageCompleted();
}

void UWxAbility_Guard::ListenForGuardHit()
{
	// 부모 태그로 등록해 자식 태그를 모두 수신한다.
	// HitReact 어빌리티는 ActivationBlockedTags=Effect.Guard라 가드 중엔 뜨지 않으므로 라우팅 충돌이 없다.
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
	// 깨지는 중에 또 맞아도 브레이크 연출을 처음부터 다시 틀지 않는다.
	if (GetActiveMontage() == GuardBreakMontage)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UWxCombatAttributeSet* AttributeSet = ASC ? ASC->GetSet<UWxCombatAttributeSet>() : nullptr;

	// 이벤트는 대미지 GE 적용이 끝난 뒤에 오므로 GetSP()가 이미 차감된 실제 값이다.
	// SP 고갈은 페이즈를 가리지 않는다 — 리액션·패링 재생 중에 0이 되어도 그 자리에서 깨진다.
	if (AttributeSet && AttributeSet->GetSP() <= 0.f)
	{
		// 브레이크 연출은 완주해야 하므로 어빌리티는 살려 두고 방어 판정만 먼저 걷는다.
		RemoveActivationOwnedEffect(UWxEffect_Guard::StaticClass());

		if (!PlayMontage(GuardBreakMontage))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

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
	if (GetActiveMontage() == GuardMontage && PerfectGuardMontage)
	{
		PlayMontage(PerfectGuardMontage);
	}
}
