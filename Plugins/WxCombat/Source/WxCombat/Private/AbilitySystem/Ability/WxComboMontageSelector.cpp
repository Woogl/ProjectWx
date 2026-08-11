// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxComboMontageSelector.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

UAnimMontage* FWxComboMontageSelector::GetNextMontage(const UAbilitySystemComponent* ASC)
{
	if (!MontageSets.IsValidIndex(CurrentSetIndex))
	{
		CurrentSetIndex = FindMontageSetIndex(ASC);
		CurrentMontageIndex = INDEX_NONE;

		if (!MontageSets.IsValidIndex(CurrentSetIndex))
		{
			return nullptr;
		}
	}

	const TArray<TObjectPtr<UAnimMontage>>& ComboMontages = MontageSets[CurrentSetIndex].ComboMontages;
	CurrentMontageIndex = ComboMontages.IsValidIndex(CurrentMontageIndex + 1) ? CurrentMontageIndex + 1 : 0;

	return ComboMontages.IsValidIndex(CurrentMontageIndex) ? ComboMontages[CurrentMontageIndex].Get() : nullptr;
}

void FWxComboMontageSelector::Reset()
{
	CurrentSetIndex = INDEX_NONE;
	CurrentMontageIndex = INDEX_NONE;
}

int32 FWxComboMontageSelector::FindMontageSetIndex(const UAbilitySystemComponent* ASC) const
{
	FGameplayTagContainer OwnedTags;
	if (ASC)
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	for (int32 Index = 0; Index < MontageSets.Num(); ++Index)
	{
		if (MontageSets[Index].EntryTagRequirements.RequirementsMet(OwnedTags))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}
