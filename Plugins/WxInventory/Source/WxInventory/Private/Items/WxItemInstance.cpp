// Copyright Woogle. All Rights Reserved.

#include "Items/WxItemInstance.h"

#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UWxItemInstance::UWxItemInstance()
	: ItemDef(nullptr)
	, CurrentCharges(0)
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
	DOREPLIFETIME(ThisClass, CurrentCharges);
}

const UWxItemDefinition* UWxItemInstance::GetItemDef() const
{
	return ItemDef;
}

const UWxItemFragment* UWxItemInstance::FindFragmentByClass(TSubclassOf<UWxItemFragment> FragmentClass) const
{
	return ItemDef ? ItemDef->FindFragmentByClass(FragmentClass) : nullptr;
}

int32 UWxItemInstance::GetCurrentCharges() const
{
	return CurrentCharges;
}

int32 UWxItemInstance::GetMaxCharges() const
{
	const UWxItemFragment_Charges* Charges = FindFragmentByClass<UWxItemFragment_Charges>();
	return Charges ? Charges->MaxCharges : 0;
}

TSoftObjectPtr<UObject> UWxItemInstance::GetDisplayIcon() const
{
	if (!ItemDef)
	{
		return nullptr;
	}

	if (const UWxItemFragment_Charges* Charges = FindFragmentByClass<UWxItemFragment_Charges>())
	{
		if (Charges->ChargeIcons.IsValidIndex(CurrentCharges) && !Charges->ChargeIcons[CurrentCharges].IsNull())
		{
			return Charges->ChargeIcons[CurrentCharges];
		}
	}

	return ItemDef->Icon;
}

void UWxItemInstance::SetCurrentCharges(int32 InCharges)
{
	CurrentCharges = FMath::Clamp(InCharges, 0, GetMaxCharges());
}

void UWxItemInstance::SetItemDef(const UWxItemDefinition* InItemDef)
{
	check(ItemDef == nullptr);
	ItemDef = InItemDef;
}

void UWxItemInstance::OnRep_CurrentCharges(int32 OldCharges)
{
	// 서버는 사용 처리 시점에 직접 통지하므로, 이 경로는 클라이언트 구독자 전달만 맡는다.
	if (UWxInventoryManagerComponent* Manager = UWxInventoryManagerComponent::FindInventory(GetTypedOuter<AActor>()))
	{
		Manager->NotifyChargeChangedFromSource(this, CurrentCharges, CurrentCharges - OldCharges);
	}
}
