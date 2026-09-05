// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Inventory.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemInstance.h"
#include "MVVM/WxViewModel_Item.h"

void UWxViewModel_Inventory::StartObserving(APlayerController* PC)
{
	Deinitialize();
	ObservedController = PC;
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_Inventory::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_Inventory::HandleInventoryEnded);
	BindSource(PC ? PC->FindComponentByClass<UWxInventoryComponent>() : nullptr);
}

void UWxViewModel_Inventory::BindSource(UWxInventoryComponent* Inventory)
{
	if (CachedInventory.Get() == Inventory)
	{
		return;
	}
	UnbindSource();
	if (!Inventory || !Inventory->HasBegunPlay() || Inventory->IsBeingDestroyed())
	{
		return;
	}
	CachedInventory = Inventory;
	StackChangedHandle = Inventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Inventory::HandleStackChanged);
	ContentsChangedHandle = Inventory->OnInventoryContentsChanged.AddUObject(this, &UWxViewModel_Inventory::HandleContentsChanged);
	RefreshAllItems();
	UE_MVVM_SET_PROPERTY_VALUE(bIsInventoryAvailable, true);
}

void UWxViewModel_Inventory::UnbindSource()
{
	if (UWxInventoryComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
		Inventory->OnInventoryContentsChanged.Remove(ContentsChangedHandle);
	}
	StackChangedHandle.Reset();
	ContentsChangedHandle.Reset();
	CachedInventory.Reset();
	if (HasAnyFlags(RF_BeginDestroyed))
	{
		return;
	}
	for (UWxViewModel_Item* ChildVM : AllItems)
	{
		if (ChildVM)
		{
			ChildVM->Deinitialize();
		}
	}
	if (LastAcquiredItem)
	{
		LastAcquiredItem->Deinitialize();
	}
	UE_MVVM_SET_PROPERTY_VALUE(bIsInventoryAvailable, false);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedItemDef, nullptr);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedAmount, 0);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedDelta, 0);
	UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, nullptr);
	AllItems.Reset();
	CategorizedItems.Reset();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AllItems);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CategorizedItems);
}

void UWxViewModel_Inventory::Deinitialize()
{
	StopObserving();
	UnbindSource();
	Super::Deinitialize();
}

void UWxViewModel_Inventory::HandleContentsChanged()
{
	RefreshAllItems();
}
int32 UWxViewModel_Inventory::GetCurrencyAmount(const UWxItemDefinition* ItemDef) const
{
	const UWxInventoryComponent* Inventory = CachedInventory.Get();
	return Inventory ? Inventory->GetTotalItemCountByDefinition(ItemDef) : 0;
}

void UWxViewModel_Inventory::SetCurrentCategory(EWxItemCategory NewCategory)
{
	if (CurrentCategory == NewCategory)
	{
		return;
	}

	UE_MVVM_SET_PROPERTY_VALUE(CurrentCategory, NewCategory);

	RefreshCategorizedItems();
}

void UWxViewModel_Inventory::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedItemDef, ItemDef);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedAmount, NewCount);
	UE_MVVM_SET_PROPERTY_VALUE(LastChangedDelta, Delta);

	if (Delta > 0 && ItemDef)
	{
		if (UWxInventoryComponent* Inventory = CachedInventory.Get())
		{
			UWxViewModel_Item* AcquisitionVM = NewObject<UWxViewModel_Item>(this);
			AcquisitionVM->Initialize(Inventory, ItemDef);
			AcquisitionVM->AcquiredCount = Delta;
			UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, AcquisitionVM);
		}
	}

	RefreshAllItems();
}

void UWxViewModel_Inventory::RefreshAllItems()
{
	UWxInventoryComponent* Inventory = CachedInventory.Get();
	TArray<TObjectPtr<UWxViewModel_Item>> NewItems;

	if (Inventory)
	{
		for (UWxItemInstance* Instance : Inventory->GetAllItems())
		{
			if (!Instance || !Instance->GetItemDef() || Inventory->GetStackCountByInstance(Instance) <= 0)
			{
				continue;
			}

			UWxViewModel_Item* ChildVM = nullptr;
			for (UWxViewModel_Item* Existing : AllItems)
			{
				if (Existing && Existing->GetTargetInstance() == Instance)
				{
					ChildVM = Existing;
					break;
				}
			}

			if (!ChildVM)
			{
				ChildVM = NewObject<UWxViewModel_Item>(this);
				ChildVM->Initialize(Inventory, Instance);
			}

			NewItems.Add(ChildVM);
		}
	}

	for (UWxViewModel_Item* OldVM : AllItems)
	{
		if (OldVM && !NewItems.Contains(OldVM))
		{
			OldVM->Deinitialize();
		}
	}

	AllItems = MoveTemp(NewItems);
	// 슬롯 구성이 그대로여도 ListView 엔트리 UMG 에서 VM 재연결이 가능하도록 항상 브로드캐스트한다.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AllItems);

	RefreshCategorizedItems();
}

void UWxViewModel_Inventory::RefreshCategorizedItems()
{
	TArray<TObjectPtr<UWxViewModel_Item>> NewCategorized;
	NewCategorized.Reserve(AllItems.Num());

	for (UWxViewModel_Item* ItemVM : AllItems)
	{
		if (!ItemVM)
		{
			continue;
		}

		const UWxItemInstance* Instance = ItemVM->GetTargetInstance();
		const UWxItemDefinition* Def = Instance ? Instance->GetItemDef() : nullptr;
		if (Def && Def->GetItemCategory() == CurrentCategory)
		{
			NewCategorized.Add(ItemVM);
		}
	}

	CategorizedItems = MoveTemp(NewCategorized);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CategorizedItems);
}

void UWxViewModel_Inventory::HandleInventoryReady(UWxInventoryComponent* Inventory)
{
	if (ObservedController.IsValid() && Inventory && Inventory->GetOwner() == ObservedController.Get())
	{
		BindSource(Inventory);
	}
}

void UWxViewModel_Inventory::HandleInventoryEnded(UWxInventoryComponent* Inventory)
{
	if (Inventory == CachedInventory.Get())
	{
		UnbindSource();
	}
}
void UWxViewModel_Inventory::StopObserving()
{
	UWxInventoryComponent::OnAnyInventoryReady.Remove(ReadyHandle);
	UWxInventoryComponent::OnAnyInventoryEnded.Remove(EndedHandle);
	ReadyHandle.Reset();
	EndedHandle.Reset();
	ObservedController.Reset();
}

UObject* UWxViewModelResolver_Inventory::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_Inventory::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	UWxViewModel_Inventory* ViewModel = NewObject<UWxViewModel_Inventory>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
	ViewModel->StartObserving(UserWidget->GetOwningPlayer());
	return ViewModel;
}

void UWxViewModelResolver_Inventory::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_Inventory* Inventory = Cast<UWxViewModel_Inventory>(ViewModel))
	{
		Inventory->Deinitialize();
	}
}
