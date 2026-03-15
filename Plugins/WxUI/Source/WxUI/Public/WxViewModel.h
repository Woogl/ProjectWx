// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel.generated.h"

UCLASS(Abstract)
class WXUI_API UWxViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

protected:
	virtual void Deinitialize();
};
