// Copyright Woogle. All Rights Reserved.

#include "Widget/WxConfirmationPopup.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "CommonRichTextBlock.h"

UWxConfirmationPopup::UWxConfirmationPopup()
{
	// 확인 전용 팝업에서도 Back을 소비해 하위 화면으로 전달하지 않는다.
	bIsBackHandler = true;
}

void UWxConfirmationPopup::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked().AddUObject(this, &ThisClass::HandleResultChosen, EWxPopupResult::Confirmed);
	}
	if (Button_Decline)
	{
		Button_Decline->OnClicked().AddUObject(this, &ThisClass::HandleResultChosen, EWxPopupResult::Declined);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked().AddUObject(this, &ThisClass::HandleResultChosen, EWxPopupResult::Cancelled);
	}
}

bool UWxConfirmationPopup::NativeOnHandleBackAction()
{
	if (BackResult != EWxPopupResult::Unknown)
	{
		HandleResultChosen(BackResult);
	}
	return true;
}

void UWxConfirmationPopup::SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
	Super::SetupPopup(Descriptor, ResultCallback);

	if (!Descriptor)
	{
		ResultCallback.ExecuteIfBound(EWxPopupResult::Killed);
		return;
	}

	OnResultCallback = ResultCallback;

	if (Text_Title)
	{
		Text_Title->SetText(Descriptor->Header);
	}
	if (RichText_Description)
	{
		RichText_Description->SetText(Descriptor->Body);
	}

	bool bHasConfirm = false;
	bool bHasDecline = false;
	bool bHasCancel = false;
	for (const FWxConfirmationPopupAction& Action : Descriptor->ButtonActions)
	{
		switch (Action.Result)
		{
		case EWxPopupResult::Confirmed: bHasConfirm = true;
			break;
		case EWxPopupResult::Declined:  bHasDecline = true;
			break;
		case EWxPopupResult::Cancelled: bHasCancel = true;
			break;
		default:
			break;
		}
	}

	if (Button_Confirm)
	{
		Button_Confirm->SetVisibility(bHasConfirm ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_Decline)
	{
		Button_Decline->SetVisibility(bHasDecline ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_Cancel)
	{
		Button_Cancel->SetVisibility(bHasCancel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	BackResult = bHasCancel ? EWxPopupResult::Cancelled :
		(bHasDecline ? EWxPopupResult::Declined : EWxPopupResult::Unknown);
	OnSetupPopup(Descriptor);
}

void UWxConfirmationPopup::HandleResultChosen(EWxPopupResult Result)
{
	// 별도 플래그 대신 델리게이트 언바인딩으로 상태를 표현해 연타나 종료 후 재진입에서 중복 실행을 막는다.
	FWxPopupResultDelegate Callback = OnResultCallback;
	OnResultCallback.Unbind();

	DeactivateWidget();
	Callback.ExecuteIfBound(Result);
}
