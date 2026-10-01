// Copyright Woogle. All Rights Reserved.

#include "Dialogue/WxDialogueActor.h"

#include "Dialogue/WxDialogueComponent.h"

AWxDialogueActor::AWxDialogueActor()
{
	DialogueComponent = CreateDefaultSubobject<UWxDialogueComponent>(TEXT("DialogueComponent"));
}

void AWxDialogueActor::GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const
{
	OutOptions.Add({DialogueComponent->GetTalkPrompt()});
}

void AWxDialogueActor::OnInteracted(AActor* Interactor, int32 OptionValue)
{
	DialogueComponent->StartDialogueWith(Interactor);
}

USkeletalMeshComponent* AWxDialogueActor::GetPoseMesh() const
{
	return nullptr;
}
