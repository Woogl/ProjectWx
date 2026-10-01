// Copyright Woogle. All Rights Reserved.

#include "GameModes/WxGameState.h"

#include "Quest/WxQuestComponent.h"
#include "Combat/WxSkillCutsceneComponent.h"

AWxGameState::AWxGameState()
{
	SkillCutsceneComponent = CreateDefaultSubobject<UWxSkillCutsceneComponent>(TEXT("SkillCutsceneComponent"));
	QuestComponent = CreateDefaultSubobject<UWxQuestComponent>(TEXT("QuestComponent"));
}
