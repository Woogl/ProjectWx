// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Character/WxCharacterBase.h"
#include "Character/WxPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Input/WxControllerInputConfig.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Inventory.h"
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

void AWxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !InputConfig || !InputConfig->MappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		Subsystem->AddMappingContext(InputConfig->MappingContext, 0);
	}
}

void AWxPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputConfig)
	{
		return;
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}

	for (const FWxMenuInputBinding& Binding : InputConfig->MenuInputBindings)
	{
		if (!Binding.InputAction || Binding.WidgetClass.IsNull() || !Binding.LayerTag.IsValid())
		{
			continue;
		}

		EIC->BindAction(Binding.InputAction, ETriggerEvent::Triggered, this,
			&ThisClass::HandleMenuInputTriggered, Binding.LayerTag, Binding.WidgetClass);
	}
}

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(InPawn))
	{
		if (UAbilitySystemComponent* ASC = WxPlayerCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerAbilitySystemViewModel(ASC);
		}
		// Global View Model 초기화가 먼저 되어야함
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::OnUnPossess()
{
	if (IsLocalController())
	{
		DeinitializePlayerAbilitySystemViewModel();
	}

	Super::OnUnPossess();
}

void AWxPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	// 인벤토리는 본 PC 의 서브오브젝트라 PC 가 도착한 시점에 즉시 사용 가능하다.
	// listen server 호스트는 InitPlayerState 시점엔 LocalPlayer 가 아직 지정 전이라 GetLocalPlayer() 가 nullptr —
	// ReceivedPlayer 는 SetPlayer 직후 호출되므로 호스트/standalone/원격 클라 모두에서 안전하다.
	if (IsLocalController())
	{
		InitializeInventoryViewModel();
	}
}

void AWxPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	if (!IsLocalController())
	{
		return;
	}

	// 원격 클라이언트: Pawn 복제 시 HUD Push 및 ViewModel 초기화
	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(GetPawn()))
	{
		if (UAbilitySystemComponent* ASC = WxPlayerCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerAbilitySystemViewModel(ASC);
		}
		// Global View Model 초기화가 먼저 되어야함
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		DeinitializeInventoryViewModel();
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

void AWxPlayerController::InitializePlayerAbilitySystemViewModel(UAbilitySystemComponent* ASC)
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UWxGlobalViewModelSubsystem* GlobalVMSubsystem = LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>();
	if (!GlobalVMSubsystem)
	{
		return;
	}

	UWxViewModel_AbilitySystem* ViewModel = GlobalVMSubsystem->GetPlayerAbilitySystemViewModel();
	if (!ViewModel)
	{
		return;
	}

	ViewModel->Initialize(ASC);

	// WxUI는 WxCombat에 의존할 수 없으므로, WxCombat 측 리소스(AbilityIcon 등)는 WxGame에서 주입한다.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(Spec.Ability);
		if (!WxAbility)
		{
			continue;
		}

		const FGameplayTagContainer& AssetTags = WxAbility->GetAssetTags();
		if (AssetTags.IsEmpty())
		{
			continue;
		}

		UWxViewModel_Ability* AbilityVM = ViewModel->FindAbilityViewModel(AssetTags.First());
		if (!AbilityVM)
		{
			continue;
		}

		AbilityVM->SetIcon(WxAbility->AbilityIcon.LoadSynchronous());
	}
}

void AWxPlayerController::DeinitializePlayerAbilitySystemViewModel()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UWxGlobalViewModelSubsystem* GlobalVMSubsystem = LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>();
	if (!GlobalVMSubsystem)
	{
		return;
	}

	if (UWxViewModel_AbilitySystem* ViewModel = GlobalVMSubsystem->GetPlayerAbilitySystemViewModel())
	{
		ViewModel->Deinitialize();
	}
}

void AWxPlayerController::InitializeInventoryViewModel()
{
	if (!InventoryManager)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UWxGlobalViewModelSubsystem* GlobalVMSubsystem = LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>();
	if (!GlobalVMSubsystem)
	{
		return;
	}

	if (UWxViewModel_Inventory* ViewModel = GlobalVMSubsystem->GetInventoryViewModel())
	{
		ViewModel->Initialize(InventoryManager);
	}
}

void AWxPlayerController::DeinitializeInventoryViewModel()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UWxGlobalViewModelSubsystem* GlobalVMSubsystem = LocalPlayer->GetSubsystem<UWxGlobalViewModelSubsystem>();
	if (!GlobalVMSubsystem)
	{
		return;
	}

	if (UWxViewModel_Inventory* ViewModel = GlobalVMSubsystem->GetInventoryViewModel())
	{
		ViewModel->Deinitialize();
	}
}

void AWxPlayerController::HandleMenuInputTriggered(FGameplayTag LayerTag, TSoftClassPtr<UWxActivatableWidget> WidgetClass)
{
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

	TSubclassOf<UWxActivatableWidget> ResolvedClass = WidgetClass.LoadSynchronous();
	if (!ResolvedClass)
	{
		return;
	}

	UIManager->PushContentToLayer(LayerTag, ResolvedClass);
}
