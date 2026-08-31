// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Skill.h"
#include "AbilitySystemComponent.h"
#include "WxGameplayTags.h"

UWxAbility_Skill::UWxAbility_Skill()
{
	// 슬롯마다 다른 애셋 태그(Ability.Skill.1~4)와 입력 액션은 BP 소관이라, 부모 태그를 기본값으로 깔아 BP가 태그를 빠뜨려도 종류 단위 지목·잠금에서 빠지지 않게 한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Skill);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Skill);
	
	ActivationGroup = EWxAbilityActivationGroup::Exclusive;

	bRetriggerInstancedAbility = true;
}

void UWxAbility_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboIndex = ComboMontages.IsValidIndex(ComboIndex + 1) ? ComboIndex + 1 : 0;

	UAnimMontage* ComboMontage = ComboMontages.IsValidIndex(ComboIndex) ? ComboMontages[ComboIndex].Get() : nullptr;
	if (!PlayMontage(ComboMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UWxAbility_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		ComboIndex = INDEX_NONE;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_Skill::HandleMontageCompleted()
{
	ComboIndex = INDEX_NONE;

	Super::HandleMontageCompleted();
}
