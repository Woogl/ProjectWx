// Copyright Woogle. All Rights Reserved.

#include "WxDialogueActor.h"

#include "WxDialogueComponent.h"

AWxDialogueActor::AWxDialogueActor()
{
	DialogueComponent = CreateDefaultSubobject<UWxDialogueComponent>(TEXT("DialogueComponent"));
}

void AWxDialogueActor::OnInteracted(AActor* Interactor)
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
