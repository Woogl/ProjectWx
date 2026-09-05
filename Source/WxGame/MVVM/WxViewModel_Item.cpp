// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Item.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"

void UWxViewModel_Item::StartObserving(APlayerController* PC, const UWxItemDefinition* InItemDef)
{
	Deinitialize();
	ObservedController = PC;
	TargetItemDef = InItemDef;
	ApplyStaticDataFromDef(InItemDef);
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_Item::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_Item::HandleInventoryEnded);
	BindSource(PC ? PC->FindComponentByClass<UWxInventoryComponent>() : nullptr);
}

void UWxViewModel_Item::Initialize(UWxInventoryComponent* InInventory, UWxItemInstance* InInstance)
{
	Deinitialize();
	TargetInstance = InInstance;
	TargetItemDef = InInstance ? InInstance->GetItemDef() : nullptr;
	ApplyStaticDataFromDef(TargetItemDef);
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_Item::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_Item::HandleInventoryEnded);
	BindSource(InInventory);
}

void UWxViewModel_Item::Initialize(UWxInventoryComponent* InInventory, const UWxItemDefinition* InItemDef)
{
	Deinitialize();
	TargetItemDef = InItemDef;
	ApplyStaticDataFromDef(InItemDef);
	ReadyHandle = UWxInventoryComponent::OnAnyInventoryReady.AddUObject(this, &UWxViewModel_Item::HandleInventoryReady);
	EndedHandle = UWxInventoryComponent::OnAnyInventoryEnded.AddUObject(this, &UWxViewModel_Item::HandleInventoryEnded);
	BindSource(InInventory);
}

void UWxViewModel_Item::BindSource(UWxInventoryComponent* Inventory)
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
	StackChangedHandle = Inventory->OnInventoryStackChanged.AddUObject(this, &UWxViewModel_Item::HandleStackChanged);
	SlotChangedHandle = Inventory->OnInventorySlotChanged.AddUObject(this, &UWxViewModel_Item::HandleSlotChanged);
	ChargeChangedHandle = Inventory->OnInventoryChargeChanged.AddUObject(this, &UWxViewModel_Item::HandleChargeChanged);
	ContentsChangedHandle = Inventory->OnInventoryContentsChanged.AddUObject(this, &UWxViewModel_Item::HandleContentsChanged);
	RefreshFromSource();
	UE_MVVM_SET_PROPERTY_VALUE(bIsInventoryAvailable, true);
}

void UWxViewModel_Item::UnbindSource()
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
	Super::Deinitialize();
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_SET_PROPERTY_VALUE(bIsInventoryAvailable, false);
		UE_MVVM_SET_PROPERTY_VALUE(TotalCount, 0);
		UE_MVVM_SET_PROPERTY_VALUE(CurrentCharges, 0);
		RequestImageAsync(TEXT("Icon"), TargetItemDef ? TargetItemDef->Icon : TSoftObjectPtr<UObject>());
	}
}

void UWxViewModel_Item::StopObserving()
{
	UWxInventoryComponent::OnAnyInventoryReady.Remove(ReadyHandle);
	UWxInventoryComponent::OnAnyInventoryEnded.Remove(EndedHandle);
	ReadyHandle.Reset();
	EndedHandle.Reset();
	ObservedController.Reset();
}

void UWxViewModel_Item::HandleInventoryReady(UWxInventoryComponent* Inventory)
{
	if (ObservedController.IsValid() && Inventory && Inventory->GetOwner() == ObservedController.Get())
	{
		BindSource(Inventory);
	}
}

void UWxViewModel_Item::HandleInventoryEnded(UWxInventoryComponent* Inventory)
{
	if (Inventory == CachedInventory.Get())
	{
		UnbindSource();
	}
}
void UWxViewModel_Item::Deinitialize()
{
	StopObserving();
	TargetItemDef = nullptr;
	UnbindSource();
	TargetInstance.Reset();
	AcquiredCount = 0;
	if (!HasAnyFlags(RF_BeginDestroyed))
	{
		UE_MVVM_SET_PROPERTY_VALUE(MaxCharges, 0);
		UE_MVVM_SET_PROPERTY_VALUE(DisplayName, FText::GetEmpty());
		UE_MVVM_SET_PROPERTY_VALUE(Grade, EWxItemGrade::Common);
		UE_MVVM_SET_PROPERTY_VALUE(GradeColor, FLinearColor::White);
	}
	Super::Deinitialize();
}

void UWxViewModel_Item::RefreshFromSource()
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

void UWxViewModel_Item::HandleContentsChanged()
{
	RefreshFromSource();
}
UWxItemInstance* UWxViewModel_Item::GetTargetInstance() const
{
	return TargetInstance.Get();
}

bool UWxViewModel_Item::RequestUseConsumable()
{
	if (UWxInventoryComponent* Inventory = CachedInventory.Get())
	{
		return Inventory->HasBegunPlay() && !Inventory->IsBeingDestroyed() && Inventory->RequestUseConsumable();
	}
	return false;
}

void UWxViewModel_Item::HandleStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (ItemDef != TargetItemDef.Get())
	{
		return;
	}

	RefreshFromSource();
}

void UWxViewModel_Item::HandleSlotChanged(UWxItemInstance* Instance, int32 NewStackCount, int32 Delta)
{
	if (Instance != TargetInstance.Get())
	{
		return;
	}

	RefreshFromSource();
}

void UWxViewModel_Item::HandleChargeChanged(UWxItemInstance* Instance, int32 NewCharges, int32 Delta)
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

void UWxViewModel_Item::RefreshChargeIcon()
{
	const UWxItemInstance* Instance = TargetInstance.Get();
	if (!Instance)
	{
		const UWxInventoryComponent* Inventory = CachedInventory.Get();
		Instance = Inventory ? Inventory->FindFirstItemStackByDefinition(TargetItemDef.Get()) : nullptr;
	}

	if (Instance && CachedInventory.IsValid() && TotalCount > 0)
	{
		RequestImageAsync(TEXT("Icon"), Instance->GetDisplayIcon());
	}
	else
	{
		RequestImageAsync(TEXT("Icon"), TargetItemDef ? TargetItemDef->Icon : TSoftObjectPtr<UObject>());
	}
}

void UWxViewModel_Item::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, LoadedImage);
}

void UWxViewModel_Item::ApplyStaticDataFromDef(const UWxItemDefinition* InItemDef)
{
	if (!InItemDef)
	{
		return;
	}

	RequestImageAsync(TEXT("Icon"), InItemDef->Icon);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InItemDef->DisplayName);

	const UWxItemFragment_Grade* GradeFragment = InItemDef->FindFragmentByClass<UWxItemFragment_Grade>();
	const EWxItemGrade ItemGrade = GradeFragment ? GradeFragment->Grade : EWxItemGrade::Common;
	UE_MVVM_SET_PROPERTY_VALUE(Grade, ItemGrade);
	UE_MVVM_SET_PROPERTY_VALUE(GradeColor, GradeFragment ? GradeFragment->Color : UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade::Common));

	const UWxItemFragment_Charges* Charges = InItemDef->FindFragmentByClass<UWxItemFragment_Charges>();
	UE_MVVM_SET_PROPERTY_VALUE(MaxCharges, Charges ? Charges->MaxCharges : 0);
}

UObject* UWxViewModelResolver_Item::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (!UserWidget || !ExpectedType || !ExpectedType->IsChildOf(UWxViewModel_Item::StaticClass()) || ExpectedType->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	UWxViewModel_Item* ViewModel = NewObject<UWxViewModel_Item>(const_cast<UUserWidget*>(UserWidget), ExpectedType);
	ViewModel->StartObserving(UserWidget->GetOwningPlayer(), ItemToDisplay);
	return ViewModel;
}

void UWxViewModelResolver_Item::DestroyInstance(UObject* ViewModel, const UMVVMView* View) const
{
	if (UWxViewModel_Item* Item = Cast<UWxViewModel_Item>(ViewModel))
	{
		Item->Deinitialize();
	}
}
