// Copyright Woogle. All Rights Reserved.

#include "WxLocatorUtils.h"

#if WITH_EDITOR
#include "GameFramework/Actor.h"
#include "UniversalObjectLocator.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"

FText FWxLocatorUtils::GetDisplayName(const FUniversalObjectLocator& Locator)
{
	if (Locator.IsEmpty())
	{
		return INVTEXT("unset");
	}

	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
	{
		return FText::FromString(Actor->GetActorNameOrLabel());
	}

	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
	const FActorLocatorFragment* Payload = nullptr;
	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
	{
		const FString SubPath = Payload->Path.GetSubPathString();
		int32 DotIndex = INDEX_NONE;
		return FText::FromString(SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath);
	}

	return INVTEXT("unresolved");
}

FText FWxLocatorUtils::GetDisplayNames(const TArray<FUniversalObjectLocator>& Locators)
{
	if (Locators.IsEmpty())
	{
		return INVTEXT("unset");
	}

	constexpr int32 MaxNames = 3;
	TArray<FString> Names;
	for (int32 Index = 0; Index < Locators.Num() && Index < MaxNames; ++Index)
	{
		Names.Add(GetDisplayName(Locators[Index]).ToString());
	}

	FString Joined = FString::Join(Names, TEXT(", "));
	if (Locators.Num() > MaxNames)
	{
		Joined += FString::Printf(TEXT(" +%d"), Locators.Num() - MaxNames);
	}
	return FText::FromString(Joined);
}
#endif
