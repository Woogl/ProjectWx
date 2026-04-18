// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemInstance.h"

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

void UWxItemInstance::SetItemDef(const UWxItemDefinition* InItemDef)
{
	ItemDef = InItemDef;
}

const UWxItemDefinition* UWxItemInstance::GetItemDef() const
{
	return ItemDef;
}
