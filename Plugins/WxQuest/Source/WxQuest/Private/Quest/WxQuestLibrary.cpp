// Copyright Woogle. All Rights Reserved.

#include "Quest/WxQuestLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Quest/WxQuestComponent.h"

namespace
{
	UWxQuestComponent* ResolveQuestComponent(const UObject* WorldContextObject)
	{
		const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		return GameState ? GameState->FindComponentByClass<UWxQuestComponent>() : nullptr;
	}
}

void UWxQuestLibrary::StartQuest(const UObject* WorldContextObject, UWxQuestStateTree* QuestAsset)
{
	if (UWxQuestComponent* QuestComponent = ResolveQuestComponent(WorldContextObject))
	{
		QuestComponent->StartQuest(QuestAsset);
	}
}

void UWxQuestLibrary::SendQuestEvent(const UObject* WorldContextObject, FGameplayTag EventTag)
{
	if (UWxQuestComponent* QuestComponent = ResolveQuestComponent(WorldContextObject))
	{
		QuestComponent->SendQuestEvent(EventTag);
	}
}
