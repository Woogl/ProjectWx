// Copyright Woogle. All Rights Reserved.

#include "Widget/WxHUDLayout.h"

#include "CommonInputModeTypes.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/WxUITags.h"
#include "System/WxUIManagerSubsystem.h"
#include "WxGameplayTags.h"

void UWxHUDLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// HUD는 UI.Layer.Game에 Game 입력 모드로 떠 있다. FBindUIActionArgs.InputMode 기본값(Menu)으로는
	// 게임 플레이 중 발동하지 않으므로, 메뉴 토글 바인딩은 반드시 InputMode = All로 등록한다.
	// CommonUI 매칭 규칙: 액션은 현재 ActiveInputMode가 All이거나 바인딩 InputMode와 정확히 일치할 때만 발동한다.
	// HUD는 GetDesiredInputConfig로 Game 모드를 적용하므로(게임 입력 유지), 바인딩도 Game이어야 게임 플레이 중 매칭된다.
	// (InputMode=All은 'ActiveMode가 정확히 All일 때만' 매칭되어 Game 모드에선 무시된다.)
	// Menu 모드(메뉴 열림)에선 자동으로 매칭되지 않으므로 중첩 열기도 방지된다.
	FBindUIActionArgs InventoryArgs(FWxUITags::Get().UIAction_Inventory, FSimpleDelegate::CreateUObject(this, &UWxHUDLayout::HandleInventoryAction));
	InventoryArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(InventoryArgs);

	FBindUIActionArgs PauseArgs(FWxUITags::Get().UIAction_PauseMenu, FSimpleDelegate::CreateUObject(this, &UWxHUDLayout::HandlePauseMenuAction));
	PauseArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(PauseArgs);
}

void UWxHUDLayout::HandleInventoryAction()
{
	PushMenuWidget(InventoryWidgetClass);
}

void UWxHUDLayout::HandlePauseMenuAction()
{
	PushMenuWidget(PauseMenuWidgetClass);
}

void UWxHUDLayout::PushMenuWidget(TSoftClassPtr<UWxActivatableWidget> WidgetClass)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UWxUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UWxUIManagerSubsystem>();
	if (!UIManager)
	{
		return;
	}

	TSubclassOf<UWxActivatableWidget> ResolvedClass = WidgetClass.LoadSynchronous();
	if (!ResolvedClass)
	{
		return;
	}

	UIManager->PushContentToLayer(WxGameplayTags::UI_Layer_Menu, ResolvedClass);
}
