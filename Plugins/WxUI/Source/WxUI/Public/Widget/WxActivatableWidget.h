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

	/** 이 위젯이 활성화된 동안 게임 정지를 원하는지. 정지 적용은 UWxUIManagerSubsystem 이 전 레이어를 재평가해 결정한다. */
	bool ShouldPauseGame() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Input")
	ECommonInputMode InputMode = ECommonInputMode::Game;

	/** 이 위젯이 활성화된 동안 게임을 일시정지한다. 멀티플레이 환경에서는 적용되지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Time")
	bool bPauseGame = false;
};
