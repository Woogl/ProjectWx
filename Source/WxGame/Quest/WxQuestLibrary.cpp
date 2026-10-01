// Copyright Woogle. All Rights Reserved.

#include "Quest/WxQuestLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Quest/WxQuestComponent.h"

void UWxQuestLibrary::StartQuest(const UObject* WorldContextObject, UStateTree* QuestAsset)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (UWxQuestComponent* QuestComponent = GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr)
	{
		QuestComponent->ActivateQuest(QuestAsset);
	}
}
