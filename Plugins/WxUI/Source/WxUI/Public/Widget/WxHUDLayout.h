// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widget/WxActivatableWidget.h"
#include "WxHUDLayout.generated.h"

/**
 * 게임 플레이 중 항상 활성화되는 HUD 루트 위젯 (UI.Layer.Game).
 * 메뉴 토글 입력을 CommonUI 액션으로 수신해 해당 메뉴를 Menu 레이어에 푸시한다.
 * Lyra의 ULyraHUDLayout 대응.
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
};
