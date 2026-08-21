// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Pattern.h"
#include "WxGameplayTags.h"

UWxAbility_Pattern::UWxAbility_Pattern()
{
	// 슬롯마다 다른 애셋 태그(Ability.Pattern.1~)는 BP 소관이라 코드가 알 수 없으므로, 부모 태그로 활성 표식을 보장한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Pattern);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 그로기·사망은 본동작까지 끊는 반응이라 패턴을 끊고, 피격은 공격·스킬만 종류로 끊으므로 패턴을 남긴다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive_Blocking;
}

void UWxAbility_Pattern::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!PlayMontage(Montage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}
