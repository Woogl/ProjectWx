// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Pattern.h"
#include "WxGameplayTags.h"

UWxAbility_Pattern::UWxAbility_Pattern()
{
	// 슬롯마다 다른 애셋 태그(Ability.Pattern.1~)는 BP 서브클래스가 지정한다.
	// 애셋 태그를 편집한 BP는 컨테이너를 통째로 갖게 되므로 여기서 단 마커가 그 BP에는 닿지 않는다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Trait_Ability_Exclusive);
	SetAssetTags(AssetTags);

	// 슬롯 태그는 BP 소관이라 코드가 알 수 없으므로 부모 태그로 활성 표식을 보장한다.
	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Pattern);

	ActivationBlockedTags.AddTag(WxGameplayTags::Ability_Death);

	// 패턴 재생 중에는 다른 패턴이나 액션이 겹쳐 들어오지 못한다.
	// 그로기·사망은 마커를 달지 않아 그대로 패턴을 끊고, 피격은 공격·스킬만 끊으므로 패턴을 남긴다.
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Trait_Ability_Exclusive);
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
