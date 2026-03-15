// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "WxActivatableWidget.generated.h"

class UWxViewModel;

UCLASS(Abstract)
class WXUI_API UWxActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UWxViewModel* InViewModel);
	UWxViewModel* GetViewModel() const;

private:
	UPROPERTY()
	TObjectPtr<UWxViewModel> ViewModel;
};
