// Copyright Woogle. All Rights Reserved.

#include "WxDialogueComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "WxDialogueSessionComponent.h"

const FDataTableRowHandle& UWxDialogueComponent::GetStartRow() const
{
	return StartRow;
}

FText UWxDialogueComponent::GetTalkPrompt() const
{
	return FText::Format(NSLOCTEXT("WxDialogueComponent", "TalkPromptFormat", "Talk to {0}"), SpeakerName);
}

void UWxDialogueComponent::StartDialogueWith(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	UWxDialogueSessionComponent* Session = Controller ? Controller->FindComponentByClass<UWxDialogueSessionComponent>() : nullptr;
	if (!Session)
	{
		return;
	}

	Session->StartDialogue(this);
}
