// Copyright Woogle. All Rights Reserved.

#pragma once

#include "FrontEnd/WxFrontEndLibrary.h"
#include "Widget/WxHUDLayout.h"
#include "Widget/WxGamePopup.h"
#include "WxFrontEndWidget.generated.h"

class UCommonButtonBase;
class UTextBlock;
class UWidgetSwitcher;
class UWxGameFlowSubsystem;

/** 선택 데이터와 디자인은 WBP에 두고, 화면 동작은 게임 흐름의 상태 변경을 따라간다. */
UCLASS(Abstract, meta = (DisableNativeTick))
class WXGAME_API UWxFrontEndWidget : public UWxHUDLayout
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|FrontEnd")
	TArray<FWxFrontEndOption> CharacterOptions;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|FrontEnd")
	TArray<FWxFrontEndOption> DestinationOptions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> NewGame;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> QuitGame;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> TemplatePlayerButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> BP_HGTest;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> CombatButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> OpenWorldButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

private:
	void HandleNewGame();
	void HandleQuitGame();
	void HandleSelectCharacter(int32 Index);
	void HandleSelectDestination(int32 Index);
	void HandleStartResult(EWxPopupResult Result);
	void HandleFrontEndChanged();

	UPROPERTY(Transient)
	TObjectPtr<UWxGameFlowSubsystem> GameFlow;

	int32 DisplayedPage = INDEX_NONE;
};
