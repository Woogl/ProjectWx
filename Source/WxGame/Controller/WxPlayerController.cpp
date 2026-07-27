// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "Character/WxPlayerCharacter.h"
#include "Components/GameFrameworkComponentManager.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxActivatableWidget.h"
#include "WxDialogueSessionComponent.h"
#include "WxGameplayTags.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DialogueSession = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSession"));
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

void AWxPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	BindCharacterDeath(InPawn);

	// HUD 위젯의 리졸버가 생성 시점에 빙의 Pawn 의 ASC 를 읽으므로 빙의 완료 후에 푸시해야 한다.
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
