// Copyright Woogle. All Rights Reserved.

#include "Component/WxPlayerLayoutComponent.h"

#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "System/WxPrimaryGameLayout.h"
#include "System/WxUIManagerSubsystem.h"
#include "Widget/WxAsyncAction_PushWidgetToLayer.h"
#include "Widget/WxHUDLayout.h"
#include "WxGameplayTags.h"
#include "WxUILibrary.h"

void UWxPlayerLayoutComponent::BeginPlay()
{
	Super::BeginPlay();

	// 띄울 화면이 없는 원격 사본(데디 서버가 든 PC)은 여기서 걸러낸다.
	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	if (!OwningController || !OwningController->IsLocalController())
	{
		return;
	}

	OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);

	// BeginPlay 가 빙의보다 늦으면 신호가 다시 오지 않으므로, 지금 폰으로 따라잡는다.
	HandlePossessedPawnChanged(nullptr, OwningController->GetPawn());
}

void UWxPlayerLayoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* OwningController = Cast<APlayerController>(GetOwner()))
	{
		OwningController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}

	ClearLayout();

	Super::EndPlay(EndPlayReason);
}

void UWxPlayerLayoutComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// ViewModel은 생성 당시 Pawn의 ASC를 소유자로 삼으므로 빙의 해제·교체 때 함께 걷는다.
	if (OldPawn != NewPawn)
	{
		ClearLayout();
	}
	if (!NewPawn || LayoutClass.IsNull())
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
	if (GameStack && GameStack->GetWidgetList().Contains(LayoutWidget.Get()))
	{
		return;
	}
	if (PendingLayoutPush)
	{
		return;
	}

	PendingLayoutPush = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(this, WxGameplayTags::UI_Layer_Game, LayoutClass);
	PendingLayoutPush->SetCompletionCallback(
		FWxPushWidgetToLayerNativeDelegate::CreateUObject(this, &ThisClass::HandleLayoutPushCompleted));
	PendingLayoutPush->Activate();
}

void UWxPlayerLayoutComponent::HandleLayoutPushCompleted(UCommonActivatableWidget* Widget)
{
	PendingLayoutPush = nullptr;
	LayoutWidget = Widget;
}

void UWxPlayerLayoutComponent::ClearLayout()
{
	if (PendingLayoutPush)
	{
		PendingLayoutPush->Cancel();
		PendingLayoutPush = nullptr;
	}
	UWxUIManagerSubsystem* UIManager = UWxUILibrary::GetUIManagerSubsystem(this);
	UWxPrimaryGameLayout* Layout = UIManager ? UIManager->GetPrimaryGameLayout() : nullptr;
	UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game) : nullptr;
	if (UCommonActivatableWidget* Widget = LayoutWidget.Get())
	{
		Widget->DeactivateWidget();
		if (Stack)
		{
			Stack->RemoveWidget(*Widget);
		}
	}
	LayoutWidget.Reset();
}
