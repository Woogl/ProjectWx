// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"

#include "Character/WxCharacterBase.h"
#include "Cheat/WxCheatManager.h"
#include "Component/WxPlayerLayoutComponent.h"
#include "Controller/WxNameplateManagerComponent.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "Inventory/WxInventoryComponent.h"
#include "Items/WxItemDefinition.h"
#include "Items/WxItemFragment.h"
#include "Items/WxItemInstance.h"
#include "MVVM/WxGameViewModelUtils.h"
#include "MVVM/WxViewModel_Character.h"
#include "MVVM/WxViewModel_Inventory.h"
#include "MVVM/WxViewModel_Item.h"
#include "MVVM/WxViewModelUtils.h"
#include "Types/MVVMViewModelCollection.h"
#include "WxDialogueSessionComponent.h"
#include "WxGame.h"
#include "WxGameplayTags.h"

namespace
{
	FGameplayTag GetItemCategoryTag(EWxItemCategory Category)
	{
		switch (Category)
		{
		case EWxItemCategory::Equipment:
			return WxGameplayTags::Item_Category_Equipment;
		case EWxItemCategory::Consumable:
			return WxGameplayTags::Item_Category_Consumable;
		case EWxItemCategory::Currency:
			return WxGameplayTags::Item_Category_Currency;
		default:
			return FGameplayTag();
		}
	}
}

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CheatClass = UWxCheatManager::StaticClass();

	InventoryComponent = CreateDefaultSubobject<UWxInventoryComponent>(TEXT("InventoryComponent"));
	InteractionScannerComponent = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScannerComponent"));
	DialogueSessionComponent = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSessionComponent"));
	PlayerLayoutComponent = CreateDefaultSubobject<UWxPlayerLayoutComponent>(TEXT("PlayerLayoutComponent"));
	NameplateManagerComponent = CreateDefaultSubobject<UWxNameplateManagerComponent>(TEXT("NameplateManagerComponent"));
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UMVVMViewModelCollectionObject* Collection = WxViewModel::GetGlobalCollection(this);
	if (PlayerCharacterViewModel)
	{
		if (Collection)
		{
			Collection->RemoveAllViewModelInstance(PlayerCharacterViewModel);
		}
		PlayerCharacterViewModel->Deinitialize();
		PlayerCharacterViewModel = nullptr;
	}
	if (InventoryViewModel)
	{
		if (Collection)
		{
			Collection->RemoveAllViewModelInstance(InventoryViewModel);
		}
		InventoryComponent->OnInventoryStackChanged.RemoveAll(this);
		InventoryComponent->OnInventorySlotChanged.RemoveAll(this);
		InventoryComponent->OnInventoryChargeChanged.RemoveAll(this);
		InventoryComponent->OnInventoryContentsChanged.RemoveAll(this);
		InventoryViewModel = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AWxPlayerController::SetPawn(APawn* InPawn)
{
	const APawn* PreviousPawn = GetPawn();
	Super::SetPawn(InPawn);

	// 빙의 교체 통지(OnPossessedPawnChanged)보다 먼저 불리므로, 그 통지로 다시 뜨는 HUD 가 새 폰으로 채워진 VM 을 받는다.
	if (GetPawn() != PreviousPawn)
	{
		RefreshPlayerCharacterViewModel();
	}
}

UWxViewModel_Item* AWxPlayerController::GetOrCreateItemViewModel(const UWxItemDefinition* ItemDef)
{
	if (!InventoryViewModel || !ItemDef)
	{
		return nullptr;
	}

	if (UWxViewModel_Item* Existing = InventoryViewModel->FindItemViewModel(ItemDef))
	{
		return Existing;
	}

	UWxViewModel_Item* ItemViewModel = CreateItemViewModel(ItemDef);
	InventoryViewModel->AddItemViewModel(ItemViewModel);
	return ItemViewModel;
}

void AWxPlayerController::BeginPlay()
{
	// 컴포넌트가 시작되며 HUD 를 띄우므로, 그보다 먼저 등록해야 위젯이 등록된 VM 을 받는다.
	UMVVMViewModelCollectionObject* Collection = IsLocalController() ? WxViewModel::GetGlobalCollection(this) : nullptr;
	if (Collection)
	{
		// Outer 를 월드 객체로 두면, 제거를 놓쳤을 때 게임 인스턴스 수명의 컬렉션이 옛 월드를 붙잡는다.
		PlayerCharacterViewModel = NewObject<UWxViewModel_Character>(Collection);
		if (!Collection->AddViewModelInstance(UWxViewModel_Character::GetPlayerContext(), PlayerCharacterViewModel))
		{
			UE_LOG(LogWxGame, Warning, TEXT("플레이어 Character 뷰모델을 Global Collection 에 등록하지 못했다. 같은 이름이 이미 등록돼 있다. PC=%s"), *GetName());
		}
		InventoryViewModel = NewObject<UWxViewModel_Inventory>(Collection);
		if (!Collection->AddViewModelInstance(UWxViewModel_Inventory::GetPlayerContext(), InventoryViewModel))
		{
			UE_LOG(LogWxGame, Warning, TEXT("플레이어 Inventory 뷰모델을 Global Collection 에 등록하지 못했다. 같은 이름이 이미 등록돼 있다. PC=%s"), *GetName());
		}
		RefreshPlayerCharacterViewModel();
	}

	Super::BeginPlay();

	// 인벤토리는 이 컨트롤러의 컴포넌트라 수명이 같고, 시작 아이템은 위의 Super 에서 컴포넌트가 지급한다.
	if (InventoryViewModel)
	{
		InventoryComponent->OnInventoryStackChanged.AddUObject(this, &ThisClass::HandleInventoryStackChanged);
		InventoryComponent->OnInventorySlotChanged.AddUObject(this, &ThisClass::HandleInventoryInstanceChanged);
		InventoryComponent->OnInventoryChargeChanged.AddUObject(this, &ThisClass::HandleInventoryInstanceChanged);
		InventoryComponent->OnInventoryContentsChanged.AddUObject(this, &ThisClass::RefreshInventoryItems);
		RefreshInventoryItems();
	}
}

void AWxPlayerController::RefreshPlayerCharacterViewModel()
{
	if (!PlayerCharacterViewModel)
	{
		return;
	}

	if (const AWxCharacterBase* PlayerCharacter = GetPawn<AWxCharacterBase>())
	{
		WxGameViewModel::InitializeCharacter(*PlayerCharacterViewModel, *PlayerCharacter);
	}
	else
	{
		PlayerCharacterViewModel->Deinitialize();
	}
}

void AWxPlayerController::HandleInventoryStackChanged(const UWxItemDefinition* ItemDef, int32 NewCount, int32 Delta)
{
	if (Delta > 0 && ItemDef)
	{
		UWxViewModel_Item* AcquisitionViewModel = CreateItemViewModel(ItemDef);
		AcquisitionViewModel->AcquiredCount = Delta;
		InventoryViewModel->SetLastAcquiredItem(AcquisitionViewModel);
	}

	RefreshInventoryItems();
}

void AWxPlayerController::HandleInventoryInstanceChanged(UWxItemInstance* Instance, int32 NewValue, int32 Delta)
{
	RefreshItemViewModels();
}

void AWxPlayerController::RefreshInventoryItems()
{
	const TArray<TObjectPtr<UWxViewModel_Item>>& CurrentItems = InventoryViewModel->GetItems();
	TArray<TObjectPtr<UWxViewModel_Item>> NewItems;

	for (UWxItemInstance* Instance : InventoryComponent->GetAllItems())
	{
		if (!Instance || !Instance->GetItemDef() || InventoryComponent->GetStackCountByInstance(Instance) <= 0)
		{
			continue;
		}

		UWxViewModel_Item* ItemViewModel = nullptr;
		for (UWxViewModel_Item* Existing : CurrentItems)
		{
			if (Existing && Existing->SourceObject == Instance)
			{
				ItemViewModel = Existing;
				break;
			}
		}

		NewItems.Add(ItemViewModel ? ItemViewModel : CreateItemViewModel(Instance));
	}

	InventoryViewModel->SetItems(MoveTemp(NewItems));
	RefreshItemViewModels();
}

void AWxPlayerController::RefreshItemViewModels()
{
	for (UWxViewModel_Item* ItemViewModel : InventoryViewModel->GetItems())
	{
		if (ItemViewModel)
		{
			RefreshItemViewModel(*ItemViewModel);
		}
	}
	for (UWxViewModel_Item* ItemViewModel : InventoryViewModel->GetItemViewModels())
	{
		if (ItemViewModel)
		{
			RefreshItemViewModel(*ItemViewModel);
		}
	}
}

void AWxPlayerController::RefreshItemViewModel(UWxViewModel_Item& ItemViewModel) const
{
	const UWxItemInstance* Instance = Cast<UWxItemInstance>(ItemViewModel.SourceObject);
	const UWxItemDefinition* ItemDef = Instance ? Instance->GetItemDef() : Cast<UWxItemDefinition>(ItemViewModel.SourceObject);
	if (!ItemDef)
	{
		return;
	}

	ItemViewModel.SetDisplayName(ItemDef->DisplayName);
	ItemViewModel.SetCategory(GetItemCategoryTag(ItemDef->GetItemCategory()));
	const UWxItemFragment_Grade* GradeFragment = ItemDef->FindFragmentByClass<UWxItemFragment_Grade>();
	ItemViewModel.SetGradeColor(GradeFragment ? GradeFragment->Color : UWxItemFragment_Grade::GetDefaultColorForGrade(EWxItemGrade::Common));

	const int32 TotalCount = Instance ? InventoryComponent->GetStackCountByInstance(Instance) : InventoryComponent->GetTotalItemCountByDefinition(ItemDef);
	// 합계 VM 은 첫 스택의 충전량과 아이콘을 대표로 보여 준다.
	const UWxItemInstance* DisplayInstance = Instance ? Instance : InventoryComponent->FindFirstItemStackByDefinition(ItemDef);
	const bool bHasStack = DisplayInstance && TotalCount > 0;

	ItemViewModel.SetTotalCount(TotalCount);
	ItemViewModel.SetCurrentCharges(bHasStack ? DisplayInstance->GetCurrentCharges() : 0);
	ItemViewModel.SetIcon(bHasStack ? DisplayInstance->GetDisplayIcon() : ItemDef->Icon);
}

UWxViewModel_Item* AWxPlayerController::CreateItemViewModel(const UObject* Source)
{
	UWxViewModel_Item* ItemViewModel = NewObject<UWxViewModel_Item>(InventoryViewModel);
	ItemViewModel->SetSourceObject(Source);
	RefreshItemViewModel(*ItemViewModel);
	return ItemViewModel;
}
