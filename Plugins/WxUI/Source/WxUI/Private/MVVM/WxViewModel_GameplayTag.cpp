// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_GameplayTag.h"
#include "AbilitySystemComponent.h"

void UWxViewModel_GameplayTag::Initialize(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	Deinitialize();
	CachedASC = InASC;

	FGameplayTagContainer CurrentTags;
	InASC->GetOwnedGameplayTags(CurrentTags);
	SetOwnedTags(CurrentTags);

	InASC->RegisterGenericGameplayTagEvent()
		.AddUObject(this, &UWxViewModel_GameplayTag::HandleGameplayTagChanged);
}

void UWxViewModel_GameplayTag::Deinitialize()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->RegisterGenericGameplayTagEvent().RemoveAll(this);
	}

	CachedASC.Reset();
	OwnedTags.Reset();

	Super::Deinitialize();
}

FGameplayTagContainer UWxViewModel_GameplayTag::GetOwnedTags() const
{
	return OwnedTags;
}

void UWxViewModel_GameplayTag::SetOwnedTags(const FGameplayTagContainer& NewTags)
{
	UE_MVVM_SET_PROPERTY_VALUE(OwnedTags, NewTags);
}

void UWxViewModel_GameplayTag::HandleGameplayTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		FGameplayTagContainer CurrentTags;
		ASC->GetOwnedGameplayTags(CurrentTags);
		SetOwnedTags(CurrentTags);
	}
}
