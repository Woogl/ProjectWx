// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CommonButtonBase.h"

#include "WxButtonBase.generated.h"

class UCommonTextBlock;

/**
 * WxUI 공용 버튼 베이스.
 * 텍스트 렌더는 UpdateButtonText 이벤트로 위임하고, 버튼 텍스트가 비면 입력 액션 표시 텍스트로 폴백한다.
 */
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

	/** 버튼 텍스트를 바인딩된 TextBlock에 세팅한다. */
	void UpdateButtonText(const FText& InText);
	
	/** 있으면 UpdateButtonText 기본 구현이 여기에 텍스트를 세팅한다. */
	UPROPERTY(BlueprintReadOnly, Category = "Button", meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> TextBlock;

private:
	UPROPERTY(EditAnywhere, Category = "Button")
	FText ButtonText;
};
