// Copyright Woogle. All Rights Reserved.

#include "Widget/WxHUDLayout.h"

#include "CommonInputModeTypes.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/CommonUIInputTypes.h"
#include "UITag.h"
#include "Widget/WxAsyncAction_PushWidgetToLayer.h"
#include "WxGameplayTags.h"

void UWxHUDLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// CommonUI 액션은 ActiveInputMode가 All이거나 바인딩 InputMode와 정확히 일치할 때만 발동한다.
	// HUD의 희망 입력 모드가 Game이므로 바인딩도 Game이어야 하고, 덕분에 메뉴가 열린 상태에선 매칭되지 않는다.
	FBindUIActionArgs InventoryArgs(FUIActionTag::ConvertChecked(WxGameplayTags::UI_Action_Inventory), FSimpleDelegate::CreateUObject(this, &UWxHUDLayout::HandleInventoryAction));
	InventoryArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(InventoryArgs);

	FBindUIActionArgs MainMenuArgs(FUIActionTag::ConvertChecked(WxGameplayTags::UI_Action_MainMenu), FSimpleDelegate::CreateUObject(this, &UWxHUDLayout::HandleMainMenuAction));
	MainMenuArgs.InputMode = ECommonInputMode::Game;
	RegisterUIActionBinding(MainMenuArgs);

	// 홀드 중 입력모드는 All 이라 Released 는 All 와일드카드로 매칭된다.
	FBindUIActionArgs FreeCursorPressedArgs(FUIActionTag::ConvertChecked(WxGameplayTags::UI_Action_FreeCursor), FSimpleDelegate::CreateUObject(this, &UWxHUDLayout::HandleFreeCursorPressed));
	FreeCursorPressedArgs.InputMode = ECommonInputMode::Game;
	FreeCursorPressedArgs.KeyEvent = IE_Pressed;
	RegisterUIActionBinding(FreeCursorPressedArgs);

	FBindUIActionArgs FreeCursorReleasedArgs(FUIActionTag::ConvertChecked(WxGameplayTags::UI_Action_FreeCursor), FSimpleDelegate::CreateUObject(this, &UWxHUDLayout::HandleFreeCursorReleased));
	FreeCursorReleasedArgs.InputMode = ECommonInputMode::Game;
	FreeCursorReleasedArgs.KeyEvent = IE_Released;
	RegisterUIActionBinding(FreeCursorReleasedArgs);
}

void UWxHUDLayout::HandleInventoryAction()
{
	PushMenuWidget(InventoryWidgetClass);
}

void UWxHUDLayout::HandleMainMenuAction()
{
	PushMenuWidget(MainMenuWidgetClass);
}

void UWxHUDLayout::HandleFreeCursorPressed()
{
	UCommonUIActionRouterBase* ActionRouter = UCommonUIActionRouterBase::Get(*this);

	// 희망 설정이 없으면 뗄 때 되돌릴 곳도 없으므로 아예 켜지 않는다.
	if (!ActionRouter || !GetDesiredInputConfig().IsSet())
	{
		return;
	}

	// All: 이동 등 게임 입력 유지, NoCapture: 커서 표시, bIgnoreLookInput: 마우스 카메라만 정지.
	FUIInputConfig Config(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
	Config.bIgnoreLookInput = true;
	ActionRouter->SetActiveUIInputConfig(Config, this);
}

void UWxHUDLayout::HandleFreeCursorReleased()
{
	UCommonUIActionRouterBase* ActionRouter = UCommonUIActionRouterBase::Get(*this);
	if (!ActionRouter)
	{
		return;
	}

	// 복원 값은 HUD 자신의 희망 설정을 단일 출처로 삼는다.
	const TOptional<FUIInputConfig> DesiredConfig = GetDesiredInputConfig();
	if (DesiredConfig.IsSet())
	{
		ActionRouter->SetActiveUIInputConfig(DesiredConfig.GetValue(), this);
	}
}

void UWxHUDLayout::PushMenuWidget(TSoftClassPtr<UWxActivatableWidget> WidgetClass)
{
	if (WidgetClass.IsNull() || PendingMenuPush)
	{
		return;
	}

	PendingMenuPush = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(this, WxGameplayTags::UI_Layer_Menu, WidgetClass);
	PendingMenuPush->SetCompletionCallback(
		FWxPushWidgetToLayerNativeDelegate::CreateUObject(this, &ThisClass::HandleMenuPushCompleted));
	PendingMenuPush->Activate();
}

void UWxHUDLayout::HandleMenuPushCompleted(UCommonActivatableWidget* Widget)
{
	PendingMenuPush = nullptr;
}
