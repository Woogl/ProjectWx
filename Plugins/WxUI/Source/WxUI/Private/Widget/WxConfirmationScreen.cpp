// Copyright Woogle. All Rights Reserved.

#include "Widget/WxConfirmationScreen.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "CommonRichTextBlock.h"

void UWxConfirmationScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 버튼→결과는 고정이므로 최초 초기화 시 1회만 바인딩한다.
	if (Button_Confirm)
	{
		Button_Confirm->OnClicked().AddUObject(this, &ThisClass::HandleResultChosen, EWxMessagingResult::Confirmed);
	}
	if (Button_Decline)
	{
		Button_Decline->OnClicked().AddUObject(this, &ThisClass::HandleResultChosen, EWxMessagingResult::Declined);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked().AddUObject(this, &ThisClass::HandleResultChosen, EWxMessagingResult::Cancelled);
	}
}

void UWxConfirmationScreen::SetupDialog(UWxGameDialogDescriptor* Descriptor, FWxMessagingResultDelegate ResultCallback)
{
	Super::SetupDialog(Descriptor, ResultCallback);

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

	// 서술자에 포함된 결과에 해당하는 버튼만 표시한다.
	bool bHasConfirm = false;
	bool bHasDecline = false;
	bool bHasCancel = false;
	for (const FWxConfirmationDialogAction& Action : Descriptor->ButtonActions)
	{
		switch (Action.Result)
		{
		case EWxMessagingResult::Confirmed: bHasConfirm = true; break;
		case EWxMessagingResult::Declined:  bHasDecline = true; break;
		case EWxMessagingResult::Cancelled: bHasCancel = true; break;
		default: break;
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

	OnSetupDialog(Descriptor);
}

void UWxConfirmationScreen::KillDialog()
{
	Super::KillDialog();
	HandleResultChosen(EWxMessagingResult::Killed);
}

void UWxConfirmationScreen::HandleResultChosen(EWxMessagingResult Result)
{
	// 결과 콜백은 최초 1회만 실행한다. 별도 플래그 대신 델리게이트 언바인딩으로 상태를 표현해
	// 연타나 종료 후 재진입에서 중복 실행을 막는다.
	FWxMessagingResultDelegate Callback = OnResultCallback;
	OnResultCallback.Unbind();

	DeactivateWidget();
	Callback.ExecuteIfBound(Result);
}
