// Copyright Woogle. All Rights Reserved.

#include "WxDialogueActor.h"

#include "WxDialogueComponent.h"

AWxDialogueActor::AWxDialogueActor()
{
	DialogueComponent = CreateDefaultSubobject<UWxDialogueComponent>(TEXT("DialogueComponent"));
}

bool AWxDialogueActor::IsInteractionEnabled() const
{
	return bInteractionEnabled;
}

void AWxDialogueActor::SetInteractionEnabled(bool bEnabled)
{
	bInteractionEnabled = bEnabled;
}

void AWxDialogueActor::OnInteracted(AActor* Interactor)
{
	DialogueComponent->StartDialogueWith(Interactor);
}

FText AWxDialogueActor::GetInteractionPrompt() const
{
	return DialogueComponent->GetTalkPrompt();
}
