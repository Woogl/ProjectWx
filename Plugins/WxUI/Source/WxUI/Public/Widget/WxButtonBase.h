// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CommonButtonBase.h"

#include "WxButtonBase.generated.h"

class UCommonTextBlock;

/** 버튼 텍스트가 비면 입력 액션 표시 텍스트로 폴백한다. */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class WXUI_API UWxButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UWxButtonBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void SetButtonText(const FText& InText);

protected:
	// UUserWidget interface
	virtual void NativePreConstruct() override;
	// End of UUserWidget interface

	// UCommonButtonBase interface
	virtual void UpdateInputActionWidget() override;
	virtual void NativeOnCurrentTextStyleChanged() override;
	// End of UCommonButtonBase interface

	void RefreshButtonText();

	void UpdateButtonText(const FText& InText);
	
	UPROPERTY(BlueprintReadOnly, Category = "Button", meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TextBlock;

private:
	UPROPERTY(EditAnywhere, Category = "Wx")
	FText ButtonText;
};
