// Copyright Woogle. All Rights Reserved.

#include "Component/WxHUDComponent.h"

#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxAsyncAction_PushWidgetToLayer.h"
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

	ClearHUD();

	Super::EndPlay(EndPlayReason);
}

void UWxHUDComponent::ClearHUD()
{
	if (PendingHUDPush)
	{
		PendingHUDPush->Cancel();
		PendingHUDPush = nullptr;
	}
	UWxUIManagerSubsystem* UIManager = UWxUILibrary::GetUIManagerSubsystem(this);
	UWxPrimaryGameLayout* Layout = UIManager ? UIManager->GetPrimaryGameLayout() : nullptr;
	UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game) : nullptr;
	if (UCommonActivatableWidget* Widget = HUDWidget.Get())
	{
		Widget->DeactivateWidget();
		if (Stack)
		{
			Stack->RemoveWidget(*Widget);
		}
	}
	HUDWidget.Reset();
}

void UWxHUDComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// ViewModel은 생성 당시 Pawn의 ASC를 소유자로 삼으므로 빙의 해제·교체 때 함께 걷는다.
	if (OldPawn != NewPawn)
	{
		ClearHUD();
	}
	if (!NewPawn)
	{
		return;
	}

	UWxUIManagerSubsystem* UIManager = UWxUILibrary::GetUIManagerSubsystem(this);
	UWxPrimaryGameLayout* Layout = UIManager ? UIManager->GetPrimaryGameLayout() : nullptr;
	if (!Layout)
	{
		return;
	}

	// 같은 Pawn 알림은 기존 HUD를 유지한다. CommonUI 풀에 남은 참조는 스택 포함 여부로 구분한다.
	const UCommonActivatableWidgetStack* GameStack = Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game);
	if (GameStack && GameStack->GetWidgetList().Contains(HUDWidget.Get()))
	{
		return;
	}
	if (PendingHUDPush)
	{
		return;
	}

	// 빙의는 Experience 로드 완료 뒤라, 이 시점엔 발행된 지정을 읽을 수 있다.
	PendingHUDPush = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(
		this, WxGameplayTags::UI_Layer_Game, UIManager->GetGameHUDClass());
	PendingHUDPush->SetCompletionCallback(
		FWxPushWidgetToLayerNativeDelegate::CreateUObject(this, &ThisClass::HandleHUDPushCompleted));
	PendingHUDPush->Activate();
}

void UWxHUDComponent::HandleHUDPushCompleted(UCommonActivatableWidget* Widget)
{
	PendingHUDPush = nullptr;
	HUDWidget = Widget;
}
