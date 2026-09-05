// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widget/WxActivatableWidget.h"
#include "WxHUDLayout.generated.h"

class UWxAsyncAction_PushWidgetToLayer;

/**
 * 게임 플레이 중 항상 활성화되는 HUD 루트 위젯 (UI.Layer.Game).
 * 메뉴 토글 입력을 CommonUI 액션으로 수신해 해당 메뉴를 Menu 레이어에 푸시한다.
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class WXUI_API UWxHUDLayout : public UWxActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	void HandleInventoryAction();
	void HandleMainMenuAction();
	void HandleFreeCursorPressed();
	void HandleFreeCursorReleased();

	/** UI.Action.Inventory 입력 시 Menu 레이어에 푸시할 위젯 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSoftClassPtr<UWxActivatableWidget> InventoryWidgetClass;

	/** UI.Action.MainMenu 입력 시 Menu 레이어에 푸시할 위젯 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|UI")
	TSoftClassPtr<UWxActivatableWidget> MainMenuWidgetClass;

private:
	void PushMenuWidget(TSoftClassPtr<UWxActivatableWidget> WidgetClass);

	void HandleMenuPushCompleted(UCommonActivatableWidget* Widget);

	/** 스트리밍이 끝나기 전까지 토글 입력이 계속 살아 있어, 진행 중인 요청을 기억해 메뉴가 겹쳐 쌓이는 것을 막는다. */
	UPROPERTY()
	TObjectPtr<UWxAsyncAction_PushWidgetToLayer> PendingMenuPush;
};
