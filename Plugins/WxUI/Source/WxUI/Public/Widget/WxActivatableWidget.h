// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "WxActivatableWidget.generated.h"

UCLASS(Abstract, meta = (DisableNativeTick))
class WXUI_API UWxActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** 실제 정지 적용은 UWxUIManagerSubsystem 이 전 레이어를 재평가해 결정한다. */
	bool ShouldPauseGame() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Input")
	ECommonInputMode InputMode = ECommonInputMode::Game;

	/** 멀티플레이 환경에서는 적용되지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Time")
	bool bPauseGame = false;
};
