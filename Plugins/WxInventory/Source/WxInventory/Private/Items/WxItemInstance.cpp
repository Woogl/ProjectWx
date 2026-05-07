// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemInstance.h"

#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"

#include "Net/UnrealNetwork.h"

UWxItemInstance::UWxItemInstance()
	: ItemDef(nullptr)
{
}

bool UWxItemInstance::IsSupportedForNetworking() const
{
	return true;
}

void UWxItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemDef);
}

const UWxItemDefinition* UWxItemInstance::GetItemDef() const
{
	return ItemDef;
}

const UWxItemFragment* UWxItemInstance::FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const
{
	return ItemDef ? ItemDef->FindFragmentByClass(FragmentClass) : nullptr;
}

void UWxItemInstance::SetItemDef(const UWxItemDefinition* InItemDef)
{
	ItemDef = InItemDef;
}
