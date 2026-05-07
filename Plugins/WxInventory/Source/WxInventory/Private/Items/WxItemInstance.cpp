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

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

void UWxItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void UWxItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 UWxItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool UWxItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
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
