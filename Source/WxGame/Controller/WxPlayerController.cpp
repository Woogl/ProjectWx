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
#include "WxDialogueSessionComponent.h"
#include "WxGameplayTags.h"
#include "WxInteractable.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InteractionScanner = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScanner"));
	DialogueSession = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSession"));
}

UWxInventoryManagerComponent* AWxPlayerController::GetInventoryManager() const
{
	// 인벤토리는 GameMode 의 FrameworkComponents 주입(서버) 또는 복제(클라)로 붙으므로 멤버로 들고 있지 않고 조회한다.
	return FindComponentByClass<UWxInventoryManagerComponent>();
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

	// 기본 지급은 권한(서버)에서만 수행하고, 결과는 FastArray 로 소유 클라이언트에 복제된다.
	// 인벤토리는 receiver 등록(PreInitializeComponents) 시점에 주입되고 Super::BeginPlay() 가 그 컴포넌트의 BeginPlay 까지 마친 뒤이므로 여기서 바로 쓸 수 있다.
	// GameMode 에 인벤토리가 등록되지 않았으면 지급 대상 자체가 없으므로 조용히 건너뛴다.
	if (HasAuthority() && !DefaultInventoryItems.IsEmpty())
	{
		if (UWxInventoryManagerComponent* Inventory = GetInventoryManager())
		{
			Inventory->GrantItems(DefaultInventoryItems);
		}
	}

	// 선택 표시는 로컬 어포던스라 소유 클라(리슨호스트 포함)에서만 브리지한다.
	// 스캐너 컴포넌트(WxWorld)는 WxUI 를 참조할 수 없으므로, 선택 변경을 PC 가 받아 전역 선택 VM 에 push 한다.
	if (IsLocalController() && InteractionScanner)
	{
		InteractionScanner->OnListChanged.AddDynamic(this, &ThisClass::HandleInteractionListChanged);
		InteractionScanner->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleInteractionSelectionChanged);

		// 컴포넌트의 초기 스캔 broadcast 는 바인딩 전에 끝났을 수 있으므로 현재 선택으로 1회 시드한다.
		PushSelectionToViewModel();
	}

	// 대화 창 표시도 로컬 어포던스다. 세션 컴포넌트(WxDialogue)는 WxUI 를 참조할 수 없으므로 시작 신호를 PC 가 받아 위젯을 푸시한다.
	if (IsLocalController() && DialogueSession)
	{
		DialogueSession->OnDialogueStarted.AddDynamic(this, &ThisClass::HandleDialogueStarted);
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

void AWxPlayerController::OnSubobjectCreatedFromReplication(UObject* NewSubobject)
{
	Super::OnSubobjectCreatedFromReplication(NewSubobject);

	// 인벤토리가 Pawn 복제보다 늦게 도착하면 OnRep_Pawn 의 HUD 푸시가 위 게이트에 걸려 건너뛰어진다.
	// 도착 시점에 이미 빙의가 끝나 있으면 여기서 한 번 더 시도해 그 순서를 메운다(반대 순서면 이 호출이 걸러지고 OnRep_Pawn 이 푸시한다).
	if (IsLocalController() && GetPawn() && Cast<UWxInventoryManagerComponent>(NewSubobject))
	{
		PushGameHUD();
	}
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

	// 인벤토리는 서버 권한에서만 생성되고(ModularGameplay 는 복제 컴포넌트를 클라에서 만들지 않는다) 클라에는 복제로 도착한다.
	// HUD 위젯의 뷰모델 리졸버는 생성 시점에 인벤토리를 한 번만 조회하므로, 도착 전에 푸시하면 인벤토리 바인딩이 빈 채로 굳는다.
	if (!GetInventoryManager())
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

void AWxPlayerController::HandleDialogueStarted()
{
	if (DialogueWidgetClass.IsNull())
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

	TSubclassOf<UWxActivatableWidget> ResolvedClass = DialogueWidgetClass.LoadSynchronous();
	if (!ResolvedClass)
	{
		return;
	}

	// 대화 위젯은 Game 레이어 스택 top 에 얹혀 HUD 를 잠시 가리고, 닫히면 HUD 가 복귀한다.
	UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Game, ResolvedClass);
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
		ViewModel->SetSelection(Target->GetInteractionPrompt(Selected), FText::GetEmpty(), nullptr);
	}
	else
	{
		ViewModel->ClearSelection();
	}
}
