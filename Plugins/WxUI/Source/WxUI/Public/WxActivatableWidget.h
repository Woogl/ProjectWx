// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "WxActivatableWidget.generated.h"

UCLASS(Abstract)
class WXUI_API UWxActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Input")
	ECommonInputMode InputMode = ECommonInputMode::Game;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Input")
	EMouseCaptureMode MouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
