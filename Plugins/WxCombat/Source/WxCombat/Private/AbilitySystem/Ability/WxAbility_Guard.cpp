// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_Guard.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "WxGameplayTags.h"

UWxAbility_Guard::UWxAbility_Guard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Guard);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Guard);
	ActivationOwnedEffects.Add(UWxEffect_Guard::StaticClass());


	// 배타 판정에는 자기 예외가 없어 활성 중 자기 재발동까지 막히는데, 가드는 그 성질에 기대어 페이즈를 유지한다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive_Blocking;
}

bool UWxAbility_Guard::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 뗀 뒤 버퍼 재생으로 뒤늦게 올라가면 다음 누름까지 가드가 고정된다.
	// 서버 스펙의 키 상태는 발동 RPC가 도착해야 채워지므로 소유 클라에서만 본다.
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
		if (Spec && !Spec->InputPressed)
		{
			return false;
		}
	}

	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
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
	// HitReact 어빌리티는 ActivationBlockedTags=Effect.Guard라 가드 중엔 뜨지 않으므로, 같은 피격 이벤트를 여기서 받아도 라우팅 충돌이 없다.
	// 반응 히트는 Event.Hit 자식으로 나가므로 정확 매칭을 끄고 부모로 받는다.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WxGameplayTags::Event_Hit, nullptr, false, false);
	if (HitTask)
	{
		HitTask->EventReceived.AddDynamic(this, &UWxAbility_Guard::HandleHit);
		HitTask->ReadyForActivation();
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

void UWxAbility_Guard::HandleHit(FGameplayEventData Payload)
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
		const FGameplayTagContainer GuardTags = WxGameplayTags::Effect_Guard.GetTag().GetSingleTagContainer();
		ASC->RemoveActiveEffectsWithTags(GuardTags);

		if (!PlayMontage(GuardBreakMontage))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

	// 반응 종류는 이벤트 태그(Event.Hit 자식)로 온다. 평타는 부모 그대로이고, Normal 이외는 전부 넉 계열로 본다.
	const bool bIsKnockHit = Payload.EventTag != WxGameplayTags::Event_Hit && Payload.EventTag != WxGameplayTags::Event_Hit_Normal;

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
