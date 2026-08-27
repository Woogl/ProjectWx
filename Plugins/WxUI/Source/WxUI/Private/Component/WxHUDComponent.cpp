// Copyright Woogle. All Rights Reserved.

#include "Component/WxHUDComponent.h"

#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxHUDLayout.h"
#include "WxGameplayTags.h"
#include "WxUILibrary.h"

void UWxHUDComponent::BeginPlay()
{
	Super::BeginPlay();

	// 주입 목록에는 사이드 구분이 없다 — 띄울 화면이 없는 원격 사본(데디 서버가 든 PC)은 여기서 걸러낸다.
	APlayerController* OwningController = GetController<APlayerController>();
	if (!OwningController || !OwningController->IsLocalController())
	{
		return;
	}

	OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);

	// 주입이 빙의보다 늦으면 신호가 다시 오지 않으므로, 지금 폰으로 따라잡는다.
	HandlePossessedPawnChanged(nullptr, OwningController->GetPawn());
}

void UWxHUDComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* OwningController = GetController<APlayerController>())
	{
		OwningController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	if (UCommonActivatableWidget* Widget = HUDWidget.Get())
	{
		Widget->DeactivateWidget();
	}
	HUDWidget.Reset();

	Super::EndPlay(EndPlayReason);
}

void UWxHUDComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (!NewPawn)
	{
		return;
	}

	// 폰만 갈아타는 흐름(탈것·연출 폰)에서 다시 push 하면 스택에 인스턴스가 쌓인다.
	if (HUDWidget.IsValid())
	{
		return;
	}

	UWxUIManagerSubsystem* UIManager = UWxUILibrary::GetUIManagerSubsystem(this);
	if (!UIManager)
	{
		return;
	}

	// 빙의는 Experience 로드 완료 뒤라, 이 시점엔 발행된 지정을 읽을 수 있다.
	HUDWidget = UIManager->PushSoftContentToLayer(WxGameplayTags::UI_Layer_Game, UIManager->GetGameHUDClass());
}
