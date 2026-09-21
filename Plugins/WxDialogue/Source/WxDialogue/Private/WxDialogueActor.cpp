// Copyright Woogle. All Rights Reserved.

#include "WxDialogueActor.h"

#include "WxDialogueComponent.h"

AWxDialogueActor::AWxDialogueActor()
{
	DialogueComponent = CreateDefaultSubobject<UWxDialogueComponent>(TEXT("DialogueComponent"));
}

void AWxDialogueActor::OnInteracted(AActor* Interactor, int32 OptionValue)
{
	DialogueComponent->StartDialogueWith(Interactor);
}

FText AWxDialogueActor::GetInteractionPrompt() const
{
	return DialogueComponent->GetTalkPrompt();
}

USkeletalMeshComponent* AWxDialogueActor::GetPoseMesh() const
{
	return nullptr;
}
