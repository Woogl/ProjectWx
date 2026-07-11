// Copyright Woogle. All Rights Reserved.

#include "Widget/WxGameDialog.h"

#define LOCTEXT_NAMESPACE "WxDialog"

UWxGameDialogDescriptor* UWxGameDialogDescriptor::CreateConfirmationOk(const FText& Header, const FText& Body)
{
	UWxGameDialogDescriptor* Descriptor = NewObject<UWxGameDialogDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationDialogAction ConfirmAction;
	ConfirmAction.Result = EWxMessagingResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Ok", "확인");

	Descriptor->ButtonActions.Add(ConfirmAction);

	return Descriptor;
}

UWxGameDialogDescriptor* UWxGameDialogDescriptor::CreateConfirmationOkCancel(const FText& Header, const FText& Body)
{
	UWxGameDialogDescriptor* Descriptor = NewObject<UWxGameDialogDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationDialogAction ConfirmAction;
	ConfirmAction.Result = EWxMessagingResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Ok", "확인");

	FWxConfirmationDialogAction CancelAction;
	CancelAction.Result = EWxMessagingResult::Cancelled;
	CancelAction.OptionalDisplayText = LOCTEXT("Cancel", "취소");

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(CancelAction);

	return Descriptor;
}

UWxGameDialogDescriptor* UWxGameDialogDescriptor::CreateConfirmationYesNo(const FText& Header, const FText& Body)
{
	UWxGameDialogDescriptor* Descriptor = NewObject<UWxGameDialogDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationDialogAction ConfirmAction;
	ConfirmAction.Result = EWxMessagingResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Yes", "예");

	FWxConfirmationDialogAction DeclineAction;
	DeclineAction.Result = EWxMessagingResult::Declined;
	DeclineAction.OptionalDisplayText = LOCTEXT("No", "아니오");

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(DeclineAction);

	return Descriptor;
}

UWxGameDialogDescriptor* UWxGameDialogDescriptor::CreateConfirmationYesNoCancel(const FText& Header, const FText& Body)
{
	UWxGameDialogDescriptor* Descriptor = NewObject<UWxGameDialogDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationDialogAction ConfirmAction;
	ConfirmAction.Result = EWxMessagingResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Yes", "예");

	FWxConfirmationDialogAction DeclineAction;
	DeclineAction.Result = EWxMessagingResult::Declined;
	DeclineAction.OptionalDisplayText = LOCTEXT("No", "아니오");

	FWxConfirmationDialogAction CancelAction;
	CancelAction.Result = EWxMessagingResult::Cancelled;
	CancelAction.OptionalDisplayText = LOCTEXT("Cancel", "취소");

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(DeclineAction);
	Descriptor->ButtonActions.Add(CancelAction);

	return Descriptor;
}

void UWxGameDialog::SetupDialog(UWxGameDialogDescriptor* Descriptor, FWxMessagingResultDelegate ResultCallback)
{
}

void UWxGameDialog::KillDialog()
{
}

#undef LOCTEXT_NAMESPACE
