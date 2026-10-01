// Copyright Woogle. All Rights Reserved.

#include "Player/WxPlayerController.h"

#include "Character/WxCharacterBase.h"
#include "Development/WxCheatManager.h"
#include "UI/WxPlayerLayoutComponent.h"
#include "UI/WxNameplateManagerComponent.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "Inventory/WxInventoryComponent.h"
#include "UI/MVVM/WxViewModel_Character.h"
#include "UI/MVVM/WxViewModel_Inventory.h"
#include "UI/MVVM/WxViewModelUtils.h"
#include "Types/MVVMViewModelCollection.h"
#include "Dialogue/WxDialogueSessionComponent.h"
#include "WxGame.h"

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
		InventoryViewModel->Deinitialize();
		InventoryViewModel = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AWxPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	// 복제 경로는 Pawn 을 먼저 대입한 뒤 부르므로 전후 폰 비교로는 교체를 알 수 없어, 매번 갱신하고 같은 ASC 판정은 VM 에 맡긴다.
	// 빙의 교체 통지(OnPossessedPawnChanged)보다 먼저 불리므로, 그 통지로 다시 뜨는 HUD 가 새 폰으로 채워진 VM 을 받는다.
	RefreshPlayerCharacterViewModel();
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
		InventoryViewModel->Initialize(InventoryComponent);
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
		PlayerCharacterViewModel->Initialize(PlayerCharacter->GetAbilitySystemComponent(), PlayerCharacter->GetTitle());
	}
	else
	{
		PlayerCharacterViewModel->Deinitialize();
	}
}
