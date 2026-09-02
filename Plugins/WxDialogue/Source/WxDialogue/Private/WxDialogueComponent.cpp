// Copyright Woogle. All Rights Reserved.

#include "WxDialogueComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "WxDialogueModule.h"
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
		// 세션 컴포넌트는 Experience 주입으로 붙으므로, 주입이 빠지면 이 갈래로 떨어진다.
		UE_LOG(LogWxDialogue, Warning, TEXT("StartDialogueWith: 대화 세션 컴포넌트를 찾지 못함(대상 %s / Interactor %s)."),
			*GetNameSafe(GetOwner()), *GetNameSafe(Interactor));
		return;
	}

	Session->StartDialogue(this);
}
