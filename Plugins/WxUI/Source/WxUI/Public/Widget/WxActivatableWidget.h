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
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Input")
	ECommonInputMode InputMode = ECommonInputMode::Game;

	/** 이 위젯이 활성화된 동안 시간 흐름 속도. 멀티플레이 환경에서는 적용되지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Time", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float TimeDilation = 1.0f;

private:
	void ApplyGlobalTimeDilation(float Dilation);
};
