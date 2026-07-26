// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widget/WxActivatableWidget.h"
#include "WxHUDLayout.generated.h"

/**
 * 게임 플레이 중 항상 활성화되는 HUD 루트 위젯 (UI.Layer.Game).
 * 메뉴 토글 입력을 CommonUI 액션으로 수신해 해당 메뉴를 Menu 레이어에 푸시한다.
 * 화면 인디케이터 캔버스를 HUD 내용 아래에 깔아 준다 — 위젯 트리에 배치하지 않아도 모든 HUD 레이아웃이 인디케이터를 갖는다.
 * Lyra의 ULyraHUDLayout 대응.
 */
UCLASS(Abstract)
class WXUI_API UWxHUDLayout : public UWxActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	/**
	 * HUD 내용 아래에 인디케이터 캔버스를 깔아 준다.
	 * 캔버스는 화면 전체를 좌표계로 쓰므로 HUD 루트와 같은 지오메트리여야 하고, HUD 의 자식이라 표시 여부가 HUD 를 따라간다.
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;

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
