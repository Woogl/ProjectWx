// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Pattern.h"
#include "WxGameplayTags.h"

UWxAbility_Pattern::UWxAbility_Pattern()
{
	// 슬롯마다 다른 애셋 태그(Ability.Pattern.1~)는 BP 서브클래스가 지정한다.
	// BP가 애셋 태그를 편집하면 컨테이너를 통째로 갖게 되므로, 여기 마커는 아직 편집하지 않은 신규 BP에만 상속된다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Trait_Exclusive);
	SetAssetTags(AssetTags);

	// 슬롯 태그는 BP 소관이라 코드가 알 수 없으므로 부모 태그로 활성 표식을 보장한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Pattern);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);
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
