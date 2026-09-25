// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Pattern.h"
#include "WxGameplayTags.h"

UWxAbility_Pattern::UWxAbility_Pattern()
{
	// BT가 부르는 번호 태그(Ability.Pattern.1 등)는 GA_가 에셋 태그와 소유 태그에 더한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Pattern);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Pattern);
	
	// 그로기·사망은 본동작까지 끊는 반응이라 패턴을 끊고, 피격은 공격·스킬만 종류로 끊으므로 패턴을 남긴다.
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;
}

void UWxAbility_Pattern::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* ComboMontage = GetMontage();
	ComboIndex = ComboIndex + 1 < GetComboStageCount(ComboMontage) ? ComboIndex + 1 : 0;

	if (!PlayMontage(ComboMontage, GetComboStageSection(ComboMontage, ComboIndex)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Pattern::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Pattern::HandleMontageBlendOut()
{
	UAnimMontage* ComboMontage = GetMontage();
	if (ComboIndex + 1 >= GetComboStageCount(ComboMontage))
	{
		return;
	}

	// 섹션을 이어 재생하면 블렌드 없이 튀므로 다음 단은 새 인스턴스로 튼다.
	ComboIndex = ComboIndex + 1;
	if (!PlayMontage(ComboMontage, GetComboStageSection(ComboMontage, ComboIndex)))
	{
		ComboIndex = INDEX_NONE;
		EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}
