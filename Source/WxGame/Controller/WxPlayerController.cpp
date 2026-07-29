// Copyright Woogle. All Rights Reserved.

#include "Controller/WxPlayerController.h"
#include "Cheat/WxCheatManager.h"
#include "CommonActivatableWidget.h"
#include "Components/GameFrameworkComponentManager.h"
#include "System/WxUIDeveloperSettings.h"
#include "WxDialogueSessionComponent.h"
#include "WxGameplayTags.h"
#include "WxUILibrary.h"

AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DialogueSession = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSession"));

	// 개발용 콘솔 치트. 클래스만 지정해 두면 엔진이 Standalone·에디터에서만 실제로 생성한다.
	CheatClass = UWxCheatManager::StaticClass();
}

void AWxPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// ModularGameplay 컴포넌트 수신 opt-in. 활성 주입 요청(Experience 액션이 등록)의 컴포넌트가 여기에 자동 부착된다.
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AWxPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 대화 창 표시는 로컬 어포던스다. 세션 컴포넌트(WxDialogue)는 WxUI 를 모르므로 시작·종료 신호를 PC 가 받아 창만 여닫는다.
	// 카메라 전환은 세션이 직접 한다 — 뷰 타겟은 Engine 이라 플러그인 안에서 닿고, 구도의 재료(대상·시작·종료)도 세션이 이미 들고 있다.
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

void AWxPlayerController::HandleDialogueStarted()
{
	// 대화 위젯은 Game 레이어 스택 top 에 얹혀 HUD 를 잠시 가리고, 닫히면 HUD 가 복귀한다.
	// 띄운 창은 세션이 끝날 때 여기서 닫으므로 인스턴스를 기억해 둔다.
	DialogueScreen = UWxUILibrary::PushSoftContentToLayer(this, WxGameplayTags::UI_Layer_Game, GetDefault<UWxUIDeveloperSettings>()->DialogueScreenClass);
}

void AWxPlayerController::HandleDialogueEnded()
{
	// 창을 띄운 쪽에서 닫는다. 종료 신호를 받아 닫으므로 대화가 어떤 경로로 끝나든 창이 남지 않는다.
	if (UCommonActivatableWidget* Screen = DialogueScreen.Get())
	{
		Screen->DeactivateWidget();
	}
	DialogueScreen.Reset();
}
