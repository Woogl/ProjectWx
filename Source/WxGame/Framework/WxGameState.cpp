// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Quest/WxQuestComponent.h"

AWxGameState::AWxGameState()
{
	QuestComponent = CreateDefaultSubobject<UWxQuestComponent>(TEXT("QuestComponent"));
}
