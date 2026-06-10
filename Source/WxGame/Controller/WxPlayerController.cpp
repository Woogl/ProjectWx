// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "Character/WxCharacterBase.h"
#include "Character/WxPlayerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "MVVM/WxGlobalViewModelSubsystem.h"
#include "MVVM/WxViewModel_Ability.h"
#include "MVVM/WxViewModel_AbilitySystem.h"
#include "MVVM/WxViewModel_Character.h"
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

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	BindCharacterDeath(InPawn);

	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(InPawn))
	{
		if (UAbilitySystemComponent* ASC = WxPlayerCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerCharacterViewModel(ASC, WxPlayerCharacter->GetCharacterUIData());
		}
		// Global View Model 초기화가 먼저 되어야함
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::OnUnPossess()
{
	if (IsLocalController())
	{
		UnbindCharacterDeath();
		DeinitializePlayerCharacterViewModel();
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

	BindCharacterDeath(GetPawn());

	// 원격 클라이언트: Pawn 복제 시 HUD Push 및 ViewModel 초기화
	if (AWxPlayerCharacter* WxPlayerCharacter = Cast<AWxPlayerCharacter>(GetPawn()))
	{
		if (UAbilitySystemComponent* ASC = WxPlayerCharacter->GetAbilitySystemComponent())
		{
			InitializePlayerCharacterViewModel(ASC, WxPlayerCharacter->GetCharacterUIData());
		}
		// Global View Model 초기화가 먼저 되어야함
		PushGameHUD(WxPlayerCharacter);
	}
}

void AWxPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		UnbindCharacterDeath();
		DismissDeathScreen();
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

void AWxPlayerController::InitializePlayerCharacterViewModel(UAbilitySystemComponent* ASC, const FWxCharacterUIData& UIData)
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

	UWxViewModel_Character* ViewModel = GlobalVMSubsystem->GetPlayerCharacterViewModel();
	if (!ViewModel)
	{
		return;
	}

	ViewModel->Initialize(ASC, UIData);

	UWxViewModel_AbilitySystem* AbilitySystemViewModel = ViewModel->AbilitySystem;
	if (!AbilitySystemViewModel)
	{
		return;
	}

	// WxUI는 WxCombat에 의존할 수 없으므로, WxCombat 측 리소스(AbilityIcon 등)는 WxGame에서 주입한다.
	// 어빌리티 VM 은 지연 생성되므로, 아이콘이 있는 어빌리티는 여기서 GetOrCreate 로 만들어 주입한다. 바인딩이 먼저 만들었다면 그 VM 을 재사용한다.
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UWxAbilityBase* WxAbility = Cast<UWxAbilityBase>(Spec.Ability);
		if (!WxAbility || WxAbility->AbilityIcon.IsNull())
		{
			continue;
		}

		const FGameplayTagContainer& AssetTags = WxAbility->GetAssetTags();
		if (AssetTags.IsEmpty())
		{
			continue;
		}

		UWxViewModel_Ability* AbilityVM = AbilitySystemViewModel->GetOrCreateAbilityViewModel(AssetTags);
		if (!AbilityVM)
		{
			continue;
		}

		AbilityVM->SetIcon(WxAbility->AbilityIcon.LoadSynchronous());
	}
}

void AWxPlayerController::DeinitializePlayerCharacterViewModel()
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

	if (UWxViewModel_Character* ViewModel = GlobalVMSubsystem->GetPlayerCharacterViewModel())
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
