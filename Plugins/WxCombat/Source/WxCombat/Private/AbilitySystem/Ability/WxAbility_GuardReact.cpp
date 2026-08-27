// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_GuardReact.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_GuardReact::UWxAbility_GuardReact()
{
	// 항상 서버에서 발행된 GameplayEvent로 트리거된다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_GuardReact);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_GuardReact);

	// 가드가 배타 점유 중인 채로 그 위에 얹혀야 한다.
	ActivationGroup = EWxAbilityActivationGroup::Reaction;

	// 가드 어빌리티가 도는 중에만 성립한다. HitReact가 차단에 쓰는 것과 같은 태그라 한 히트에 둘 중 하나만 뜬다.
	// 이 검사는 서버에서만 돈다 — 소유 클라는 ClientActivateAbilitySucceedWithEventData가 CallActivateAbility를 직접 불러 태그 요건을 건너뛴다.
	ActivationRequiredTags.AddTag(WxGameplayTags::Ability_Guard);

	// 가드 중 연속 피격은 앞 연출을 끊고 새로 튼다.
	bRetriggerInstancedAbility = true;

	// 반응 없는 평타는 부모 그대로 오므로 부모를 등록한다. 자식까지 등록하면 조상마다 한 번씩 발화해 같은 히트에 두 번 뜬다.
	// 반응 종류는 TriggerEventData의 EventTag에 원래 자식 태그로 실려 온다.
	FAbilityTriggerData HitTrigger;
	HitTrigger.TriggerTag = WxGameplayTags::Event_Hit;
	HitTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(HitTrigger);

	FAbilityTriggerData PerfectGuardTrigger;
	PerfectGuardTrigger.TriggerTag = WxGameplayTags::Event_PerfectGuard;
	PerfectGuardTrigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(PerfectGuardTrigger);
}

float UWxAbility_GuardReact::GetMontagePlayRate() const
{
	return 1.f;
}

void UWxAbility_GuardReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const FGameplayTag TriggerTag = TriggerEventData ? TriggerEventData->EventTag : WxGameplayTags::Event_Hit;

	if (TriggerTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		// 방어 판정은 가드 어빌리티가 쥐고 있으므로 그것을 끊어 걷는다.
		// 커밋·몽타주보다 먼저 끊어야 한다 — 어느 쪽이든 실패해 가드가 남으면 SP 회복이 Effect.Guard에 막혀 다시는 깨지지 않는 상태가 된다.
		// 이 어빌리티는 서버와 소유 클라 양쪽에서 활성화되므로 취소도 양쪽에서 로컬로 일어난다.
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			const FGameplayTagContainer GuardAbilityTags(WxGameplayTags::Ability_Guard);
			ASC->CancelAbilities(&GuardAbilityTags);
		}
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo) || !PlayMontage(SelectMontage(TriggerTag)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주가 실제로 걸린 뒤에 건다 — 실패 경로에서 걸면 태스크가 같은 프레임에 파괴되며 복제된 딜레이션만 한 번 튄다.
	if (TriggerTag == WxGameplayTags::Event_PerfectGuard)
	{
		if (UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectGuardSlowTimeDilation, PerfectGuardSlowTimeDuration))
		{
			SlowTimeTask->ReadyForActivation();
		}
	}
}

void UWxAbility_GuardReact::HandleMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UAnimMontage* UWxAbility_GuardReact::SelectMontage(FGameplayTag TriggerTag) const
{
	if (TriggerTag == WxGameplayTags::Event_Hit_GuardBreak)
	{
		return GuardBreakMontage;
	}

	if (TriggerTag == WxGameplayTags::Event_PerfectGuard)
	{
		return PerfectGuardMontage;
	}

	// 반응 종류는 Event.Hit 자식으로 온다. 평타는 부모 그대로이고, Normal 이외는 전부 넉 계열로 본다.
	const bool bIsKnockHit = TriggerTag != WxGameplayTags::Event_Hit && TriggerTag != WxGameplayTags::Event_Hit_Normal;

	return (bIsKnockHit && GuardKnockbackMontage) ? GuardKnockbackMontage.Get() : GuardHitReactMontage.Get();
}
