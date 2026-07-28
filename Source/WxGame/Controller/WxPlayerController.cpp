// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Character/WxCharacterBase.h"
#include "Character/WxPlayerCharacter.h"
#include "CommonActivatableWidget.h"
#include "Components/GameFrameworkComponentManager.h"
#include "System/WxUIDeveloperSettings.h"
#include "Widget/WxActivatableWidget.h"
#include "WxDialogueSessionComponent.h"
#include "WxGameplayTags.h"
#include "WxUILibrary.h"

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

	// 대화 창 표시도, 대화 카메라 전환도 로컬 어포던스다. 세션 컴포넌트(WxDialogue)는 WxUI 도 뷰 타겟도 모르므로 시작·종료 신호를 PC 가 받아 처리한다.
	if (IsLocalController() && DialogueSession)
	{
		DialogueSession->OnDialogueStarted.AddDynamic(this, &ThisClass::HandleDialogueStarted);
		DialogueSession->OnDialogueEnded.AddDynamic(this, &ThisClass::HandleDialogueEnded);
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
	UWxUILibrary::PushSoftContentToLayer(this, WxGameplayTags::UI_Layer_Game, GetDefault<UWxUIDeveloperSettings>()->GameHUDClass);
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
	if (!IsLocalController())
	{
		return;
	}

	// 사망 화면은 걷어내지 않는다. 부활은 월드 리로드(TravelFromSaveFile)이고, 그때 UI 매니저가 레이아웃을 통째로 재생성한다.
	UWxUILibrary::PushSoftContentToLayer(this, WxGameplayTags::UI_Layer_Menu, GetDefault<UWxUIDeveloperSettings>()->DeathScreenClass);
}

void AWxPlayerController::HandleDialogueStarted()
{
	// 대화 위젯은 Game 레이어 스택 top 에 얹혀 HUD 를 잠시 가리고, 닫히면 HUD 가 복귀한다.
	// 띄운 창은 세션이 끝날 때 여기서 닫으므로 인스턴스를 기억해 둔다.
	DialogueScreen = UWxUILibrary::PushSoftContentToLayer(this, WxGameplayTags::UI_Layer_Game, GetDefault<UWxUIDeveloperSettings>()->DialogueScreenClass);

	// 대화 상대의 카메라로 넘긴다. 액터만 넘기면 AActor::CalcCamera 가 그 액터의 활성 카메라 컴포넌트를 골라 쓰므로 카메라를 직접 꺼내올 필요가 없다.
	// 대상은 세션이 시작을 발행하기 전에 기억해 두므로 여기서 항상 유효하다.
	if (AActor* DialogueTarget = DialogueSession->GetCurrentDialogueTarget())
	{
		SetViewTargetWithBlend(DialogueTarget, DialogueCameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
	}
}

void AWxPlayerController::HandleDialogueEnded()
{
	// 창을 띄운 쪽에서 닫는다. 종료 신호를 받아 닫으므로 대화가 어떤 경로로 끝나든 창이 남지 않는다.
	if (UCommonActivatableWidget* Screen = DialogueScreen.Get())
	{
		Screen->DeactivateWidget();
	}
	DialogueScreen.Reset();

	// 원래 게임플레이 뷰타겟인 내 폰으로 블렌드 복귀한다.
	// bLockOutgoing=true: 블렌드 시작 시점의 출발 POV 를 고정해, 대화가 끝난 NPC 가 움직이기 시작해도 복귀가 튀지 않는다.
	if (APawn* MyPawn = GetPawn())
	{
		SetViewTargetWithBlend(MyPawn, DialogueCameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.f, true);
	}
}
