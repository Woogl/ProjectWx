// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Widget/WxGamePopup.h"
#include "WxConfirmationPopup.generated.h"

class UCommonTextBlock;
class UCommonRichTextBlock;
class UCommonButtonBase;

/**
 * 헤더/본문 + 확인/거절/취소 버튼을 갖춘 확인 팝업 위젯.
 * 결과 3종(Confirmed/Declined/Cancelled)에 1:1 대응하는 고정 버튼을 사용하며, 서술자의 버튼 구성에 따라 필요한 버튼만 표시한다.
 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class WXUI_API UWxConfirmationPopup : public UWxGamePopup
{
	GENERATED_BODY()

public:
	virtual void SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback) override;
	virtual void KillPopup() override;

protected:
	virtual void NativeOnInitialized() override;

	/**
	 * 텍스트/버튼 표시가 끝난 뒤, WBP가 버튼 라벨(OptionalDisplayText)이나 부가 비주얼을 구성하도록 호출된다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wx|Popup")
	void OnSetupPopup(UWxGamePopupDescriptor* Descriptor);

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Popup", meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> Button_Confirm;

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Popup", meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> Button_Decline;

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Popup", meta = (BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> Button_Cancel;

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Popup", meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, Category = "Wx|Popup", meta = (BindWidgetOptional))
	TObjectPtr<UCommonRichTextBlock> RichText_Description;

private:
	/** 버튼 클릭/강제 종료 공통 진입점. 결과 콜백은 최초 1회만 실행하고 위젯을 닫는다. */
	void HandleResultChosen(EWxPopupResult Result);

	FWxPopupResultDelegate OnResultCallback;
};
