// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Inventory.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"
#include "MVVM/WxViewModel_Item.h"

UWxViewModel_Inventory* UWxViewModel_Inventory::GetOrCreate(APlayerController* PC)
{
	if (!PC)
	{
		return nullptr;
	}

	if (UWxViewModel* Existing = FindSharedViewModel(PC, StaticClass()))
	{
		return CastChecked<UWxViewModel_Inventory>(Existing);
	}

	UWxViewModel_Inventory* ViewModel = NewObject<UWxViewModel_Inventory>(PC);
	ViewModel->Initialize(PC);

	return ViewModel;
}

void UWxViewModel_Inventory::Initialize(APlayerController* PC)
{
	ObservedController = PC;
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_Inventory::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_Inventory::HandleInventoryEnded);
	BindSource(PC->FindComponentByClass<UWxInventoryComponent>());
}

void UWxViewModel_Inventory::Deinitialize()
{
	UWxInventoryComponent::OnAnyInventoryReady.Remove(ReadyHandle);
	UWxInventoryComponent::OnAnyInventoryEnded.Remove(EndedHandle);
	ReadyHandle.Reset();
	EndedHandle.Reset();
	ObservedController.Reset();
	UnbindSource();

	// 합계 VM 은 배열에서 떼기만 한다 — 위젯이 아직 붙들고 있는 공유본을 끊으면 그 표시가 언다.
	ItemViewModels.Empty();

	Super::Deinitialize();
}

UWxViewModel_Item* UWxViewModel_Inventory::GetOrCreateItemViewModel(const UWxItemDefinition* ItemDef)
{
	if (!ItemDef)
	{
		return nullptr;
	}

	for (UWxViewModel_Item* Existing : ItemViewModels)
	{
		if (Existing && Existing->SourceObject == ItemDef)
		{
			return Existing;
		}
	}

	UWxViewModel_Item* ItemViewModel = CreateItemViewModel(ItemDef);
	ItemViewModels.Add(ItemViewModel);
	return ItemViewModel;
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
	SlotChangedHandle = Inventory->OnInventorySlotChanged.AddUObject(this, &UWxViewModel_Inventory::HandleInstanceChanged);
	ChargeChangedHandle = Inventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_Inventory::HandleInstanceChanged);
	ContentsChangedHandle = Inventory->OnInventoryContentsChanged.AddUObject(this, &UWxViewModel_Inventory::RefreshAllItems);
	RefreshAllItems();
}

void UWxViewModel_Inventory::UnbindSource()
{
	if (UWxInventoryComponent* Inventory = CachedInventory.Get())
	{
		Inventory->OnInventoryStackChanged.Remove(StackChangedHandle);
		Inventory->OnInventorySlotChanged.Remove(SlotChangedHandle);
		Inventory->OnInventoryChargeChanged.Remove(ChargeChangedHandle);
		Inventory->OnInventoryContentsChanged.Remove(ContentsChangedHandle);
	}
	StackChangedHandle.Reset();
	SlotChangedHandle.Reset();
	ChargeChangedHandle.Reset();
	ContentsChangedHandle.Reset();
	CachedInventory.Reset();
	if (HasAnyFlags(RF_BeginDestroyed))
	{
		return;
	}
	AllItems.Reset();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AllItems);
	RefreshCategorizedItems();
	UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, nullptr);
	RefreshItemViewModels();
}

void UWxViewModel_Inventory::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (Delta > 0 && ItemDef)
	{
		UWxViewModel_Item* AcquisitionVM = CreateItemViewModel(ItemDef);
		AcquisitionVM->AcquiredCount = Delta;
		UE_MVVM_SET_PROPERTY_VALUE(LastAcquiredItem, AcquisitionVM);
	}

	RefreshAllItems();
}

void UWxViewModel_Inventory::HandleInstanceChanged(UWxItemInstance* Instance, int32 NewValue, int32 Delta)
{
	RefreshItemViewModels();
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
				if (Existing && Existing->SourceObject == Instance)
				{
					ChildVM = Existing;
					break;
				}
			}

			NewItems.Add(ChildVM ? ChildVM : CreateItemViewModel(Instance));
		}
	}

	AllItems = MoveTemp(NewItems);
	// 슬롯 구성이 그대로여도 ListView 엔트리 UMG 에서 VM 재연결이 가능하도록 항상 브로드캐스트한다.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AllItems);

	RefreshCategorizedItems();
	RefreshItemViewModels();
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

void UWxViewModel_Inventory::RefreshCategorizedItems()
{
	TArray<TObjectPtr<UWxViewModel_Item>> NewCategorized;
	NewCategorized.Reserve(AllItems.Num());

	for (UWxViewModel_Item* ItemVM : AllItems)
	{
		const UWxItemInstance* Instance = ItemVM ? Cast<UWxItemInstance>(ItemVM->SourceObject) : nullptr;
		const UWxItemDefinition* ItemDef = Instance ? Instance->GetItemDef() : nullptr;
		if (ItemDef && ItemDef->GetItemCategory() == CurrentCategory)
		{
			NewCategorized.Add(ItemVM);
		}
	}

	CategorizedItems = MoveTemp(NewCategorized);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CategorizedItems);
}

void UWxViewModel_Inventory::RefreshItemViewModels()
{
	for (UWxViewModel_Item* ItemViewModel : AllItems)
	{
		if (ItemViewModel)
		{
			RefreshItemViewModel(*ItemViewModel);
		}
	}
	for (UWxViewModel_Item* ItemViewModel : ItemViewModels)
	{
		if (ItemViewModel)
		{
			RefreshItemViewModel(*ItemViewModel);
		}
	}
}

void UWxViewModel_Inventory::RefreshItemViewModel(UWxViewModel_Item& ItemViewModel) const
{
	const UWxItemInstance* Instance = Cast<UWxItemInstance>(ItemViewModel.SourceObject);
	const UWxItemDefinition* ItemDef = Instance ? Instance->GetItemDef() : Cast<UWxItemDefinition>(ItemViewModel.SourceObject);
	if (!ItemDef)
	{
		return;
	}

	ItemViewModel.SetDisplayName(ItemDef->DisplayName);
	const UWxItemFragment_Grade* GradeFragment = ItemDef->FindFragmentByClass<UWxItemFragment_Grade>();
	ItemViewModel.SetGradeColor(GradeFragment ? GradeFragment->Color : UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade::Common));

	const UWxInventoryComponent* Inventory = CachedInventory.Get();
	int32 TotalCount = 0;
	const UWxItemInstance* DisplayInstance = Instance;
	if (Inventory)
	{
		TotalCount = Instance ? Inventory->GetStackCountByInstance(Instance) : Inventory->GetTotalItemCountByDefinition(ItemDef);
		// 합계 VM 은 첫 스택의 충전량과 아이콘을 대표로 보여 준다.
		if (!DisplayInstance)
		{
			DisplayInstance = Inventory->FindFirstItemStackByDefinition(ItemDef);
		}
	}
	const bool bHasStack = DisplayInstance && TotalCount > 0;

	ItemViewModel.SetTotalCount(TotalCount);
	ItemViewModel.SetCurrentCharges(bHasStack ? DisplayInstance->GetCurrentCharges() : 0);
	ItemViewModel.SetIcon(bHasStack ? DisplayInstance->GetDisplayIcon() : ItemDef->Icon);
}

UWxViewModel_Item* UWxViewModel_Inventory::CreateItemViewModel(const UObject* Source)
{
	UWxViewModel_Item* ItemViewModel = NewObject<UWxViewModel_Item>(this);
	ItemViewModel->SetSourceObject(Source);
	RefreshItemViewModel(*ItemViewModel);
	return ItemViewModel;
}

UObject* UWxViewModelResolver_Inventory::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	return UWxViewModel_Inventory::GetOrCreate(UserWidget ? UserWidget->GetOwningPlayer() : nullptr);
}

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	UWxViewModel_Inventory* InventoryViewModel = UWxViewModel_Inventory::GetOrCreate(UserWidget ? UserWidget->GetOwningPlayer() : nullptr);
	return InventoryViewModel ? InventoryViewModel->GetOrCreateItemViewModel(ItemToDisplay) : nullptr;
}
