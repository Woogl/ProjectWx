// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Inventory.h"

#include "Inventory/WxInventoryComponent.h"
#include "Inventory/WxItemDefinition.h"
#include "Inventory/WxItemInstance.h"
#include "UI/MVVM/WxViewModel_Item.h"
#include "UI/MVVM/WxViewModelUtils.h"
#include "Types/MVVMViewModelCollection.h"
#include "WxGameplayTags.h"

UWxViewModel_Inventory::UWxViewModel_Inventory()
{
	CurrentCategory = WxGameplayTags::Item_Category_Equipment;
}

FMVVMViewModelContext UWxViewModel_Inventory::GetPlayerContext()
{
	FMVVMViewModelContext Context;
	Context.ContextClass = StaticClass();
	Context.ContextName = TEXT("VM_Inventory");
	return Context;
}

UWxViewModel_Inventory* UWxViewModel_Inventory::FindPlayer(const UObject* WorldContextObject)
{
	UMVVMViewModelCollectionObject* Collection = WxViewModel::GetGlobalCollection(WorldContextObject);
	return Collection ? Cast<UWxViewModel_Inventory>(Collection->FindViewModelInstance(GetPlayerContext())) : nullptr;
}

void UWxViewModel_Inventory::Initialize(UWxInventoryComponent* InInventory)
{
	Inventory = InInventory;

	InInventory->OnInventoryStackChanged.AddUObject(this, &ThisClass::HandleInventoryStackChanged);
	InInventory->OnInventoryChargeChanged.AddUObject(this, &ThisClass::HandleInventoryChargeChanged);
	InInventory->OnInventoryContentsChanged.AddUObject(this, &ThisClass::RefreshItems);
	RefreshItems();
}

void UWxViewModel_Inventory::Deinitialize()
{
	if (UWxInventoryComponent* InventoryComponent = Inventory.Get())
	{
		InventoryComponent->OnInventoryStackChanged.RemoveAll(this);
		InventoryComponent->OnInventoryChargeChanged.RemoveAll(this);
		InventoryComponent->OnInventoryContentsChanged.RemoveAll(this);
	}
	Inventory.Reset();
}

UWxViewModel_Item* UWxViewModel_Inventory::GetOrCreateItemViewModel(const UWxItemDefinition* ItemDef)
{
	if (!ItemDef)
	{
		return nullptr;
	}

	for (UWxViewModel_Item* Existing : DefinitionItems)
	{
		if (Existing && Existing->GetDefinition() == ItemDef)
		{
			return Existing;
		}
	}

	UWxViewModel_Item* ItemViewModel = CreateItemViewModel(*ItemDef, nullptr);
	DefinitionItems.Add(ItemViewModel);
	return ItemViewModel;
}

void UWxViewModel_Inventory::SetCurrentCategory(FGameplayTag NewCategory)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CurrentCategory, NewCategory))
	{
		RefreshCategorizedItems();
	}
}

void UWxViewModel_Inventory::HandleInventoryStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (Delta > 0 && ItemDef)
	{
		LastAcquiredItem = CreateItemViewModel(*ItemDef, nullptr);
		LastAcquiredItem->AcquiredCount = Delta;
		OnItemAcquired.Broadcast();
	}

	RefreshItems();
}

void UWxViewModel_Inventory::HandleInventoryChargeChanged(UWxItemInstance* Instance, int32 NewCharges, int32 Delta)
{
	RefreshItemViewModels();
}

void UWxViewModel_Inventory::RefreshItems()
{
	const UWxInventoryComponent* InventoryComponent = Inventory.Get();
	if (!InventoryComponent)
	{
		return;
	}

	TArray<TObjectPtr<UWxViewModel_Item>> NewSlotItems;
	for (UWxItemInstance* Instance : InventoryComponent->GetAllItems())
	{
		const UWxItemDefinition* ItemDef = Instance ? Instance->GetItemDef() : nullptr;
		if (!ItemDef || InventoryComponent->GetStackCountByInstance(Instance) <= 0)
		{
			continue;
		}

		UWxViewModel_Item* ItemViewModel = nullptr;
		for (UWxViewModel_Item* Existing : SlotItems)
		{
			if (Existing && Existing->GetInstance() == Instance)
			{
				ItemViewModel = Existing;
				break;
			}
		}

		NewSlotItems.Add(ItemViewModel ? ItemViewModel : CreateItemViewModel(*ItemDef, Instance));
	}

	SlotItems = MoveTemp(NewSlotItems);
	RefreshCategorizedItems();
	RefreshItemViewModels();
}

void UWxViewModel_Inventory::RefreshItemViewModels()
{
	const UWxInventoryComponent* InventoryComponent = Inventory.Get();
	if (!InventoryComponent)
	{
		return;
	}

	for (UWxViewModel_Item* ItemViewModel : SlotItems)
	{
		if (ItemViewModel)
		{
			ItemViewModel->Refresh(*InventoryComponent);
		}
	}
	for (UWxViewModel_Item* ItemViewModel : DefinitionItems)
	{
		if (ItemViewModel)
		{
			ItemViewModel->Refresh(*InventoryComponent);
		}
	}
}

UWxViewModel_Item* UWxViewModel_Inventory::CreateItemViewModel(const UWxItemDefinition& Definition, const UWxItemInstance* Instance)
{
	UWxViewModel_Item* ItemViewModel = NewObject<UWxViewModel_Item>(this);
	ItemViewModel->Initialize(Definition, Instance);

	// 초기화 전에 만든 합계 VM 은 초기화가 다시 채운다.
	if (const UWxInventoryComponent* InventoryComponent = Inventory.Get())
	{
		ItemViewModel->Refresh(*InventoryComponent);
	}
	return ItemViewModel;
}

void UWxViewModel_Inventory::RefreshCategorizedItems()
{
	TArray<TObjectPtr<UWxViewModel_Item>> NewCategorized;
	NewCategorized.Reserve(SlotItems.Num());

	for (UWxViewModel_Item* ItemViewModel : SlotItems)
	{
		if (ItemViewModel && ItemViewModel->Category.MatchesTagExact(CurrentCategory))
		{
			NewCategorized.Add(ItemViewModel);
		}
	}

	CategorizedItems = MoveTemp(NewCategorized);
	// 구성이 그대로여도 ListView 엔트리 UMG 에서 VM 재연결이 가능하도록 항상 브로드캐스트한다.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CategorizedItems);
}
