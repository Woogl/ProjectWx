// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_InventoryItem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"

void UWxViewModel_InventoryItem::StartObserving(APlayerController* PC, const UWxItemDefinition* InItemDef)
{
	Deinitialize();
	ObservedController = PC;
	TargetItemDef = InItemDef;
	ApplyStaticDataFromDef(InItemDef);
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_InventoryItem::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_InventoryItem::HandleInventoryEnded);
	BindSource(PC ? PC->FindComponentByClass<UWxInventoryComponent>() : nullptr);
}

void UWxViewModel_InventoryItem::Initialize(UWxInventoryComponent* InInventory, UWxItemInstance* InInstance)
{
	Deinitialize();
	TargetInstance = InInstance;
	TargetItemDef = InInstance ? InInstance->GetItemDef() : nullptr;
	ApplyStaticDataFromDef(TargetItemDef);
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_InventoryItem::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_InventoryItem::HandleInventoryEnded);
	BindSource(InInventory);
}

void UWxViewModel_InventoryItem::Initialize(UWxInventoryComponent* InInventory, const UWxItemDefinition* InItemDef)
{
	Deinitialize();
	TargetItemDef = InItemDef;
	ApplyStaticDataFromDef(InItemDef);
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_InventoryItem::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_InventoryItem::HandleInventoryEnded);
	BindSource(InInventory);
}

void UWxViewModel_InventoryItem::BindSource(UWxInventoryComponent* Inventory)
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
	StackChangedHandle = Inventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_InventoryItem::HandleStackChanged);
	SlotChangedHandle = Inventory->OnInventorySlotChanged.AddUObject(this, &UWxViewModel_InventoryItem::HandleSlotChanged);
	ChargeChangedHandle = Inventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_InventoryItem::HandleChargeChanged);
	ContentsChangedHandle = Inventory->OnInventoryContentsChanged.AddUObject(this, &UWxViewModel_InventoryItem::HandleContentsChanged);
	RefreshFromSource();
	UE_MVVM_SET_PROPERTY_VALUE(bIsInventoryAvailable, true);
}

void UWxViewModel_InventoryItem::UnbindSource()
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
	// 인벤토리 연결만 끊을 때는 원본과 정적 표시 데이터를 유지한다.
	UWxViewModel::Deinitialize();
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_SET_PROPERTY_VALUE(bIsInventoryAvailable, false);
		UE_MVVM_SET_PROPERTY_VALUE(TotalCount, 0);
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, 0);
		SetIcon(TargetItemDef ? TargetItemDef->Icon : TSoftObjectPtr<UObject>());
	}
}

void UWxViewModel_InventoryItem::StopObserving()
{
	UWxInventoryComponent::OnAnyInventoryReady.Remove(ReadyHandle);
	UWxInventoryComponent::OnAnyInventoryEnded.Remove(EndedHandle);
	ReadyHandle.Reset();
	EndedHandle.Reset();
	ObservedController.Reset();
}

void UWxViewModel_InventoryItem::HandleInventoryReady(UWxInventoryComponent* Inventory)
{
	if (ObservedController.IsValid() && Inventory && Inventory->GetOwner() == ObservedController.Get())
	{
		BindSource(Inventory);
	}
}

void UWxViewModel_InventoryItem::HandleInventoryEnded(UWxInventoryComponent* Inventory)
{
	if (Inventory == CachedInventory.Get())
	{
		UnbindSource();
	}
}
void UWxViewModel_InventoryItem::Deinitialize()
{
	StopObserving();
	TargetItemDef = nullptr;
	UnbindSource();
	TargetInstance.Reset();
	AcquiredCount = 0;
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_SET_PROPERTY_VALUE(MaxCharges, 0);
		UE_MVVM_SET_PROPERTY_VALUE(Grade, EWxItemGrade::Common);
		UE_MVVM_SET_PROPERTY_VALUE(GradeColor, FLinearColor::White);
	}
	Super::Deinitialize();
}

void UWxViewModel_InventoryItem::RefreshFromSource()
{
	const UWxInventoryComponent* Inventory = CachedInventory.Get();
	if (!Inventory || !Inventory->HasBegunPlay() || Inventory->IsBeingDestroyed())
	{
		return;
	}
	const UWxItemInstance* Instance = TargetInstance.Get();
	if (Instance && TargetItemDef != Instance->GetItemDef())
	{
		TargetItemDef = Instance->GetItemDef();
		ApplyStaticDataFromDef(TargetItemDef);
	}
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, Instance ? Inventory->GetStackCountByInstance(Instance) : Inventory->GetTotalItemCountByDefinition(TargetItemDef));
	if (!Instance)
	{
		Instance = Inventory->FindFirstItemStackByDefinition(TargetItemDef);
	}
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, Instance && TotalCount > 0 ? Instance->GetCurrentCharges() : 0);
	RefreshChargeIcon();
}

void UWxViewModel_InventoryItem::HandleContentsChanged()
{
	RefreshFromSource();
}
UWxItemInstance* UWxViewModel_InventoryItem::GetTargetInstance() const
{
	return TargetInstance.Get();
}

bool UWxViewModel_InventoryItem::RequestUseConsumable()
{
	if (UWxInventoryComponent* Inventory = CachedInventory.Get())
	{
		return Inventory->HasBegunPlay() && !Inventory->IsBeingDestroyed() && Inventory->RequestUseConsumable();
	}
	return false;
}

void UWxViewModel_InventoryItem::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (ItemDef != TargetItemDef.Get())
	{
		return;
	}

	RefreshFromSource();
}

void UWxViewModel_InventoryItem::HandleSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (Instance != TargetInstance.Get())
	{
		return;
	}

	RefreshFromSource();
}

void UWxViewModel_InventoryItem::HandleChargeChanged(UWxItemInstance* Instance, int32 NewCharges, int32 Delta)
{
	if (!Instance)
	{
		return;
	}

	const UWxItemInstance* TrackedInstance = TargetInstance.Get();
	const bool bMatches = TrackedInstance ? (Instance == TrackedInstance) : (Instance->GetItemDef() == TargetItemDef.Get());
	if (!bMatches)
	{
		return;
	}

	RefreshFromSource();
}

void UWxViewModel_InventoryItem::RefreshChargeIcon()
{
	const UWxItemInstance* Instance = TargetInstance.Get();
	if (!Instance)
	{
		const UWxInventoryComponent* Inventory = CachedInventory.Get();
		Instance = Inventory ? Inventory->FindFirstItemStackByDefinition(TargetItemDef.Get()) : nullptr;
	}

	if (Instance && CachedInventory.IsValid() && TotalCount > 0)
	{
		SetIcon(Instance->GetDisplayIcon());
	}
	else
	{
		SetIcon(TargetItemDef ? TargetItemDef->Icon : TSoftObjectPtr<UObject>());
	}
}

void UWxViewModel_InventoryItem::ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef)
{
	// 슬롯 인스턴스의 기존 약한 참조 수명은 유지하고 표시 원본인 정의를 보관한다.
	SetSourceObject(InItemDef);
	if (!InItemDef)
	{
		return;
	}

	SetIcon(InItemDef->Icon);
	SetDisplayName(InItemDef->DisplayName);

	const UWxItemFragment_Grade* GradeFragment = InItemDef->FindFragmentByClass<UWxItemFragment_Grade>();
	const EWxItemGrade ItemGrade = GradeFragment ? GradeFragment->Grade : EWxItemGrade::Common;
	UE_MVVM_SET_PROPERTY_VALUE(Grade, ItemGrade);
	UE_MVVM_SET_PROPERTY_VALUE(GradeColor, GradeFragment ? GradeFragment->Color : UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade::Common));

	const UWxItemFragment_Charges* Charges = InItemDef->FindFragmentByClass<UWxItemFragment_Charges>();
	UE_MVVM_SET_PROPERTY_VALUE(MaxCharges, Charges ? Charges->MaxCharges : 0);
}

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_InventoryItem::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	UWxViewModel_InventoryItem* ViewModel = NewObject<UWxViewModel_InventoryItem>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
	ViewModel->StartObserving(UserWidget->GetOwningPlayer(), ItemToDisplay);
	return ViewModel;
}

void UWxViewModelResolver_Item::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_InventoryItem* Item = Cast<UWxViewModel_InventoryItem>(ViewModel))
	{
		Item->Deinitialize();
	}
}
