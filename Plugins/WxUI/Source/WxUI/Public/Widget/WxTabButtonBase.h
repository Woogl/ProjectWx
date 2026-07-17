// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Widget/WxButtonBase.h"
#include "Widget/WxTabListWidgetBase.h"

#include "WxTabButtonBase.generated.h"

class UCommonLazyImage;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class WXUI_API UWxTabButtonBase : public UWxButtonBase, public IWxTabButtonInterface
{
	GENERATED_BODY()

public:
	void SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject);
	void SetIconBrush(const FSlateBrush& Brush);

protected:
	UFUNCTION()
	virtual void SetTabLabelInfo_Implementation(const FWxTabDescriptor& TabLabelInfo) override;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonLazyImage> LazyImage_Icon;
};
