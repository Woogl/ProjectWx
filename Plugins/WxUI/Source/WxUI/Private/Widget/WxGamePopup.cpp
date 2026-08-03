// Copyright Woogle. All Rights Reserved.

#include "Widget/WxGamePopup.h"

bool FWxConfirmationPopupAction::operator==(const FWxConfirmationPopupAction& Other) const
{
	return Result == Other.Result && OptionalDisplayText.EqualTo(Other.OptionalDisplayText);
}

UWxGamePopupDescriptor* UWxGamePopupDescriptor::CreateConfirmationOk(const FText& Header, const FText& Body)
{
	UWxGamePopupDescriptor* Descriptor = NewObject<UWxGamePopupDescriptor>();
	Descriptor->Header = Header;
	Descriptor->Body = Body;

	FWxConfirmationPopupAction ConfirmAction;
	ConfirmAction.Result = EWxPopupResult::Confirmed;
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

	FWxConfirmationPopupAction CancelAction;
	CancelAction.Result = EWxPopupResult::Cancelled;

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

	FWxConfirmationPopupAction DeclineAction;
	DeclineAction.Result = EWxPopupResult::Declined;

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

	FWxConfirmationPopupAction DeclineAction;
	DeclineAction.Result = EWxPopupResult::Declined;

	FWxConfirmationPopupAction CancelAction;
	CancelAction.Result = EWxPopupResult::Cancelled;

	Descriptor->ButtonActions.Add(ConfirmAction);
	Descriptor->ButtonActions.Add(DeclineAction);
	Descriptor->ButtonActions.Add(CancelAction);

	return Descriptor;
}

UWxGamePopup::UWxGamePopup()
{
	InputMode = ECommonInputMode::Menu;
	bPauseGame = true;
}

void UWxGamePopup::SetupPopup(UWxGamePopupDescriptor* Descriptor, FWxPopupResultDelegate ResultCallback)
{
}

void UWxGamePopup::KillPopup()
{
}
