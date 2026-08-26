// Copyright Woogle. All Rights Reserved.

#include "WxLocatorUtils.h"

#if WITH_EDITOR
#include "GameFramework/Actor.h"
#include "UniversalObjectLocator.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"

FString FWxLocatorUtils::GetDisplayName(const FUniversalObjectLocator& Locator)
{
	if (Locator.IsEmpty())
	{
		return TEXT("unset");
	}

	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
	{
		return Actor->GetActorNameOrLabel();
	}

	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
	const FActorLocatorFragment* Payload = nullptr;
	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
	{
		const FString SubPath = Payload->Path.GetSubPathString();
		int32 DotIndex = INDEX_NONE;
		return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
	}

	return TEXT("unresolved");
}

FText FWxLocatorUtils::GetDisplayNamesText(const TArray<FUniversalObjectLocator>& Locators)
{
	if (Locators.IsEmpty())
	{
		return INVTEXT("unset");
	}

	constexpr int32 MaxNames = 3;
	TArray<FString> Names;
	for (int32 Index = 0; Index < Locators.Num() && Index < MaxNames; ++Index)
	{
		Names.Add(GetDisplayName(Locators[Index]));
	}

	FString Joined = FString::Join(Names, TEXT(", "));
	if (Locators.Num() > MaxNames)
	{
		Joined += FString::Printf(TEXT(" +%d"), Locators.Num() - MaxNames);
	}
	return FText::FromString(Joined);
}
#endif
