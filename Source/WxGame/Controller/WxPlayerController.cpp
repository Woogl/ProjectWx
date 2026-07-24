// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "Character/WxPlayerCharacter.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Interaction/WxInteractionScannerComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "MVVM/WxViewModel_Selection.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxActivatableWidget.h"
#include "WxGameplayTags.h"
#include "WxInteractable.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InventoryManager = CreateDefaultSubobject<UWxInventoryManagerComponent>(TEXT("InventoryManager"));
	InteractionScanner = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScanner"));
}

UWxInventoryManagerComponent* AWxPlayerController::GetInventoryManager() const
{
	return InventoryManager;
}

void AWxPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// ModularGameplay 컴포넌트 수신 opt-in. 활성 주입 요청(GameMode 가 등록)의 컴포넌트가 여기에 자동 부착된다.
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 선택 표시는 로컬 어포던스라 소유 클라(리슨호스트 포함)에서만 브리지한다.
	// 스캐너 컴포넌트(WxWorld)는 WxUI 를 참조할 수 없으므로, 선택 변경을 PC 가 받아 전역 선택 VM 에 push 한다.
	if (IsLocalController() && InteractionScanner)
	{
		InteractionScanner->OnListChanged.AddDynamic(this, &ThisClass::HandleInteractionListChanged);
		InteractionScanner->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleInteractionSelectionChanged);

		// 컴포넌트의 초기 스캔 broadcast 는 바인딩 전에 끝났을 수 있으므로 현재 선택으로 1회 시드한다.
		PushSelectionToViewModel();
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);

	Super::EndPlay(EndPlayReason);
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
	PushGameHUD();
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
	PushGameHUD();
}

void AWxPlayerController::PushGameHUD()
{
	if (!GameHUDWidgetClass)
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

	UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Game, GameHUDWidgetClass);
}

void AWxPlayerController::BindCharacterDeath(APawn* InPawn)
{
	AWxCharacterBase* WxCharacter = Cast<AWxCharacterBase>(InPawn);
	if (!WxCharacter)
	{
		return;
	}

	// 같은 Pawn 에 대해 OnRep_Pawn 이 거듭 올 수 있어 중복 바인딩을 막는다.
	WxCharacter->OnDeath.AddUniqueDynamic(this, &ThisClass::HandleCharacterDeath);
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

	// 사망 화면은 걷어내지 않는다. 부활은 월드 리로드(TravelFromSaveFile)이고, 그때 UI 매니저가 레이아웃을 통째로 재생성한다.
	UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Menu, ResolvedClass);
}

void AWxPlayerController::HandleInteractionListChanged(const TArray<FText>& Prompts)
{
	// 목록이 바뀌면 선택 대상도 바뀔 수 있으므로 현재 선택으로 VM 을 다시 push 한다.
	PushSelectionToViewModel();
}

void AWxPlayerController::HandleInteractionSelectionChanged(int32 SelectedIndex)
{
	PushSelectionToViewModel();
}

void AWxPlayerController::PushSelectionToViewModel()
{
	UGameInstance* GameInst = GetGameInstance();
	UWxUIManagerSubsystem* UIManager = GameInst ? GameInst->GetSubsystem<UWxUIManagerSubsystem>() : nullptr;
	UWxViewModel_Selection* ViewModel = UIManager ? UIManager->GetSelectionViewModel() : nullptr;
	if (!ViewModel)
	{
		return;
	}

	// 프롬프트는 선택 대상이 IWxInteractable 로 제공한다(pull). 표시는 프롬프트만(Description/Icon 은 비움).
	const UPrimitiveComponent* Selected = InteractionScanner ? InteractionScanner->GetSelectedMesh() : nullptr;
	if (const IWxInteractable* Target = Selected ? Cast<IWxInteractable>(Selected->GetOwner()) : nullptr)
	{
		ViewModel->SetSelection(Target->GetInteractionPrompt(), FText::GetEmpty(), nullptr);
	}
	else
	{
		ViewModel->ClearSelection();
	}
}
