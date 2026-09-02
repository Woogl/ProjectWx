// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Guard.h"
#include "AbilitySystem/Effect/WxEffect_Guard.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "WxGameplayTags.h"

UWxAbility_Guard::UWxAbility_Guard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Guard);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Guard);
	ActivationOwnedEffects.Add(UWxEffect_Guard::StaticClass());


	// 배타 판정에는 자기 예외가 없어 활성 중 자기 재발동까지 막히는데, 가드는 그 성질에 기대어 상태를 유지한다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;
}

bool UWxAbility_Guard::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 뗀 뒤 버퍼 재생으로 뒤늦게 올라가면 다음 누름까지 가드가 고정된다.
	// 서버 스펙의 키 상태는 발동 RPC가 무조건 true로 세우므로 여기서 걸러낼 수 없다 — 소유 클라에서만 본다.
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

	// 리액션이 도는 중이면 끊지 않는다 — 퍼펙트 가드 뒤에도 Effect.Guard가 남아야 반격 윈도우가 선다.
	// 미뤄 둔 종료는 리액션이 끝나는 지점에서 반영한다. 여기서도 대기를 걸어 둬야 리액션이 자세를 밀어내지 않은 경우에도 가드가 고착되지 않는다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_GuardReact))
	{
		ListenForGuardReactEnded();
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
	}
}

void UWxAbility_Guard::HandleMontageCompleted()
{
}

void UWxAbility_Guard::HandleMontageInterrupted()
{
	// 그로기·사망은 Ability 태그로 이 어빌리티를 먼저 취소하므로 여기 걸리지 않는다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_GuardReact))
	{
		ListenForGuardReactEnded();
		return;
	}

	Super::HandleMontageInterrupted();
}

void UWxAbility_Guard::ListenForGuardReactEnded()
{
	// 연속 피격은 리액션이 스스로 재발동하며 자세 밀어내기를 되풀이하므로, 한 번 쓰고 그 지점에서 다시 건다.
	// 두 진입점에서 겹쳐 걸릴 수 있으나 핸들러가 같은 프레임에 두 번 도는 것뿐이라 무해하다.
	UAbilityTask_WaitGameplayTagRemoved* RemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, WxGameplayTags::Ability_GuardReact, nullptr, true);
	RemovedTask->Removed.AddDynamic(this, &UWxAbility_Guard::HandleGuardReactEnded);
	RemovedTask->ReadyForActivation();
}

void UWxAbility_Guard::HandleGuardReactEnded()
{
	if (!IsInputHeld())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	PlayMontage(GuardMontage);
}

bool UWxAbility_Guard::IsInputHeld() const
{
	if (!IsLocallyControlled())
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(CurrentSpecHandle) : nullptr;
	return !Spec || Spec->InputPressed;
}
