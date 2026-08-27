// Copyright Woogle. All Rights Reserved.

#include "Widget/WxConfirmationPopup.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "CommonRichTextBlock.h"

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

void UWxConfirmationPopup::SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
	Super::SetupPopup(Descriptor, ResultCallback);

	if (!Descriptor)
	{
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

	OnSetupPopup(Descriptor);
}

void UWxConfirmationPopup::KillPopup()
{
	Super::KillPopup();
	HandleResultChosen(EWxPopupResult::Killed);
}

void UWxConfirmationPopup::HandleResultChosen(EWxPopupResult Result)
{
	// 별도 플래그 대신 델리게이트 언바인딩으로 상태를 표현해 연타나 종료 후 재진입에서 중복 실행을 막는다.
	FWxPopupResultDelegate Callback = OnResultCallback;
	OnResultCallback.Unbind();

	DeactivateWidget();
	Callback.ExecuteIfBound(Result);
}
