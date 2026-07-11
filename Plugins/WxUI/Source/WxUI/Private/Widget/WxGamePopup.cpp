// Copyright Woogle. All Rights Reserved.

#include "Widget/WxGamePopup.h"

#define LOCTEXT_NAMESPACE "WxPopup"

UWxGamePopupDescriptor* UWxGamePopupDescriptor::CreateConfirmationOk(const FText& Header, const FText& Body)
{
	UWxGamePopupDescriptor* Descriptor = NewObject<UWxGamePopupDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationPopupAction ConfirmAction;
	ConfirmAction.Result = EWxPopupResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Ok", "확인");

	Descriptor->ButtonActions.Add(ConfirmAction);

	return Descriptor;
}

UWxGamePopupDescriptor* UWxGamePopupDescriptor::CreateConfirmationOkCancel(const FText& Header, const FText& Body)
{
	UWxGamePopupDescriptor* Descriptor = NewObject<UWxGamePopupDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationPopupAction ConfirmAction;
	ConfirmAction.Result = EWxPopupResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Ok", "확인");

	FWxConfirmationPopupAction CancelAction;
	CancelAction.Result = EWxPopupResult::Cancelled;
	CancelAction.OptionalDisplayText = LOCTEXT("Cancel", "취소");

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(CancelAction);

	return Descriptor;
}

UWxGamePopupDescriptor* UWxGamePopupDescriptor::CreateConfirmationYesNo(const FText& Header, const FText& Body)
{
	UWxGamePopupDescriptor* Descriptor = NewObject<UWxGamePopupDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationPopupAction ConfirmAction;
	ConfirmAction.Result = EWxPopupResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Yes", "예");

	FWxConfirmationPopupAction DeclineAction;
	DeclineAction.Result = EWxPopupResult::Declined;
	DeclineAction.OptionalDisplayText = LOCTEXT("No", "아니오");

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(DeclineAction);

	return Descriptor;
}

UWxGamePopupDescriptor* UWxGamePopupDescriptor::CreateConfirmationYesNoCancel(const FText& Header, const FText& Body)
{
	UWxGamePopupDescriptor* Descriptor = NewObject<UWxGamePopupDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationPopupAction ConfirmAction;
	ConfirmAction.Result = EWxPopupResult::Confirmed;
	ConfirmAction.OptionalDisplayText = LOCTEXT("Yes", "예");

	FWxConfirmationPopupAction DeclineAction;
	DeclineAction.Result = EWxPopupResult::Declined;
	DeclineAction.OptionalDisplayText = LOCTEXT("No", "아니오");

	FWxConfirmationPopupAction CancelAction;
	CancelAction.Result = EWxPopupResult::Cancelled;
	CancelAction.OptionalDisplayText = LOCTEXT("Cancel", "취소");

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(DeclineAction);
	Descriptor->ButtonActions.Add(CancelAction);

	return Descriptor;
}

void UWxGamePopup::SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
}

void UWxGamePopup::KillPopup()
{
}

#undef LOCTEXT_NAMESPACE
