// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxMontageSelector.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

UAnimMontage* FWxMontageSelector::SelectMontage(const UAbilitySystemComponent* ASC, FGameplayTag TriggerEventTag)
{
	Reset();
	return AdvanceMontage(ASC, TriggerEventTag);
}

UAnimMontage* FWxMontageSelector::AdvanceMontage(const UAbilitySystemComponent* ASC, FGameplayTag TriggerEventTag)
{
	if (!MontageSets.IsValidIndex(CurrentSetIndex))
	{
		CurrentSetIndex = FindMontageSetIndex(ASC, TriggerEventTag);
		CurrentMontageIndex = INDEX_NONE;

		if (!MontageSets.IsValidIndex(CurrentSetIndex))
		{
			return nullptr;
		}
	}

	const TArray<TObjectPtr<UAnimMontage>>& Montages = MontageSets[CurrentSetIndex].Montages;
	CurrentMontageIndex = Montages.IsValidIndex(CurrentMontageIndex + 1) ? CurrentMontageIndex + 1 : 0;

	return Montages.IsValidIndex(CurrentMontageIndex) ? Montages[CurrentMontageIndex].Get() : nullptr;
}

void FWxMontageSelector::Reset()
{
	CurrentSetIndex = INDEX_NONE;
	CurrentMontageIndex = INDEX_NONE;
}

int32 FWxMontageSelector::FindMontageSetIndex(const UAbilitySystemComponent* ASC, FGameplayTag TriggerEventTag) const
{
	FGameplayTagContainer OwnedTags;
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	for (int32 Index = 0; Index < MontageSets.Num(); ++Index)
	{
		const FWxMontageSet& MontageSet = MontageSets[Index];

		// 이벤트 조건을 건 세트는 트리거 없이 발동한 어빌리티(입력·AI)에서는 후보가 되지 않는다.
		if (!MontageSet.TriggerEventTags.IsEmpty() && !MontageSet.TriggerEventTags.HasTag(TriggerEventTag))
		{
			continue;
		}

		if (MontageSet.AvatarTagRequirements.RequirementsMet(OwnedTags))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}
