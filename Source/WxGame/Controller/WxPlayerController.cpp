// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "Character/WxPlayerCharacter.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxActivatableWidget.h"
#include "WxGameplayTags.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InventoryManager = CreateDefaultSubobject<UWxInventoryManagerComponent>(TEXT("InventoryManager"));
}

UWxInventoryManagerComponent* AWxPlayerController::GetInventoryManager() const
{
	return InventoryManager;
}

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	BindCharacterDeath(InPawn);

	// HUD 위젯의 리졸버가 생성 시점에 빙의 Pawn 의 ASC/인벤토리를 읽으므로 빙의 완료 후에 푸시해야 한다.
	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(InPawn))
	{
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::OnUnPossess()
{
	if (IsLocalController())
	{
		UnbindCharacterDeath();
	}

	Super::OnUnPossess();
}

void AWxPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (!IsLocalController())
	{
		return;
	}

	BindCharacterDeath(GetPawn());

	// 원격 클라이언트: Pawn 복제 시 HUD Push.
	// 리졸버가 생성 시점에 Pawn 의 ASC/인벤토리를 읽으므로 이 시점이어야 한다.
	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(GetPawn()))
	{
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		UnbindCharacterDeath();
		DismissDeathScreen();
	}

	Super::EndPlay(EndPlayReason);
}

void AWxPlayerController::PushGameHUD(AWxPlayerCharacter* PlayerCharacter)
{
	TSubclassOf<UWxActivatableWidget> HUDClass = PlayerCharacter->GetGameHUDClass();
	if (!HUDClass)
	{
		return;
	}

	UGameInstance* GameInst = GetGameInstance();
	if (!GameInst)
	{
		return;
	}

	UWxUIManagerSubsystem* UIManager = GameInst->GetSubsystem<UWxUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	GameHUD = Cast<UWxActivatableWidget>(UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Game, HUDClass));
}

void AWxPlayerController::BindCharacterDeath(APawn* InPawn)
{
	// 이전 캐릭터 정리 및 이전 사망 화면 제거 (부활/Pawn 교체 시 자동 정리)
	UnbindCharacterDeath();
	DismissDeathScreen();

	AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(InPawn);
	if (!WxCharacter)
	{
		return;
	}

	WxCharacter->OnDeath.AddDynamic(this, &ThisClass::HandleCharacterDeath);
	BoundCharacter = WxCharacter;
}

void AWxPlayerController::UnbindCharacterDeath()
{
	if (AWxCharacterBase* WxCharacter = BoundCharacter.Get())
	{
		WxCharacter->OnDeath.RemoveDynamic(this, &ThisClass::HandleCharacterDeath);
	}
	BoundCharacter.Reset();
}

void AWxPlayerController::DismissDeathScreen()
{
	if (DeathScreen)
	{
		DeathScreen->DeactivateWidget();
		DeathScreen = nullptr;
	}
}

void AWxPlayerController::HandleCharacterDeath(AWxCharacterBase* DeadCharacter)
{
	if (!IsLocalController() || DeathScreenWidgetClass.IsNull())
	{
		return;
	}

	UGameInstance* GameInst = GetGameInstance();
	if (!GameInst)
	{
		return;
	}

	UWxUIManagerSubsystem* UIManager = GameInst->GetSubsystem<UWxUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	TSubclassOf<UWxActivatableWidget> ResolvedClass = DeathScreenWidgetClass.LoadSynchronous();
	if (!ResolvedClass)
	{
		return;
	}

	DeathScreen = Cast<UWxActivatableWidget>(UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Menu, ResolvedClass));
}
