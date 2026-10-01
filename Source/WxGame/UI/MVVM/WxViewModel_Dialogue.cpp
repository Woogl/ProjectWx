// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Dialogue.h"
#include "Dialogue/WxDialogueSessionComponent.h"

void UWxViewModel_Dialogue::SetSession(UWxDialogueSessionComponent* InSession)
{
	Session = InSession;
}

void UWxViewModel_Dialogue::SetLine(const FText& InSpeaker, const FText& InLine)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(Speaker, InSpeaker))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HasSpeaker);
	}
	UE_MVVM_SET_PROPERTY_VALUE(LineText, InLine);
}

void UWxViewModel_Dialogue::RequestAdvance()
{
	if (UWxDialogueSessionComponent* DialogueSession = Session.Get())
	{
		DialogueSession->Advance();
	}
}

bool UWxViewModel_Dialogue::HasSpeaker() const
{
	return !Speaker.IsEmpty();
}
