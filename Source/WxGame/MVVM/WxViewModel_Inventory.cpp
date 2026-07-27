// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Inventory.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemInstance.h"
#include "MVVM/WxViewModel_Item.h"

void UWxViewModel_Inventory::StartObserving(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	ObservedController = PC;

	// 이미 붙어 있으면(호스트에선 위젯보다 항상 먼저다) 기다릴 것 없이 바로 연결한다.
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(PC))
	{
		Initialize(Inventory);
		return;
	}

	InventoryReadyHandle = UWxInventoryManagerComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_Inventory::HandleInventoryReady);
}

void UWxViewModel_Inventory::Initialize(UWxInventoryManagerComponent* InInventory)
{
	if (!InInventory)
	{
		return;
	}

	Deinitialize();

	CachedInventory = InInventory;
	StackChangedHandle = InInventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Inventory::HandleStackChanged);

	RefreshAllItems();
}

void UWxViewModel_Inventory::Deinitialize()
{
	if (UWxInventoryManagerComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
	}
	StackChangedHandle.Reset();
	CachedInventory.Reset();

	LastChangedItemDef = nullptr;
	LastChangedAmount = 0;
	LastChangedDelta = 0;
	LastAcquiredItem = nullptr;

	for (UWxViewModel_Item* ChildVM : AllItems)
	{
		if (ChildVM)
		{
			ChildVM->Deinitialize();
		}
	}
	AllItems.Reset();
	CategorizedItems.Reset();

	Super::Deinitialize();
}

void UWxViewModel_Inventory::BeginDestroy()
{
	StopObserving();

	Super::BeginDestroy();
}

int32 UWxViewModel_Inventory::GetCurrencyAmount(const UWxItemDefinition* ItemDef) const
{
	const UWxInventoryManagerComponent* Inventory = CachedInventory.Get();
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
		if (UWxInventoryManagerComponent* Inventory = CachedInventory.Get())
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
	UWxInventoryManagerComponent* Inventory = CachedInventory.Get();
	TArray<TObjectPtr<UWxViewModel_Item>> NewItems;
	TArray<TObjectPtr<UWxViewModel_Item>> Retained;

	if (Inventory)
	{
		for (UWxItemInstance* Instance : Inventory->GetAllItems())
		{
			if (!Instance)
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
			Retained.Add(ChildVM);
		}
	}

	for (UWxViewModel_Item* OldVM : AllItems)
	{
		if (OldVM && !Retained.Contains(OldVM))
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

void UWxViewModel_Inventory::HandleInventoryReady(UWxInventoryManagerComponent* Inventory)
{
	// 신호는 클래스 차원이라 남의 인벤토리도 온다. 관찰 중인 컨트롤러의 것만 받는다.
	if (!Inventory || Inventory->GetOwner() != ObservedController.Get())
	{
		return;
	}

	StopObserving();
	Initialize(Inventory);
}

void UWxViewModel_Inventory::StopObserving()
{
	if (InventoryReadyHandle.IsValid())
	{
		UWxInventoryManagerComponent::OnAnyInventoryReady.Remove(InventoryReadyHandle);
		InventoryReadyHandle.Reset();
	}
}

UObject* UWxViewModelResolver_Inventory::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	if (!PC)
	{
		return nullptr;
	}

	// 인벤토리가 아직 없을 수 있으므로 Outer 는 PC 로 잡는다. 연결은 VM 이 관찰로 스스로 처리한다.
	UWxViewModel_Inventory* ViewModel = NewObject<UWxViewModel_Inventory>(PC);
	ViewModel->StartObserving(PC);
	return ViewModel;
}
