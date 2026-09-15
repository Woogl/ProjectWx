// Copyright Woogle. All Rights Reserved.

#include "Framework/WxGameState.h"

#include "Quest/WxQuestComponent.h"
#include "Cutscene/WxSkillCutsceneComponent.h"

AWxGameState::AWxGameState()
{
	SkillCutsceneComponent = CreateDefaultSubobject<UWxSkillCutsceneComponent>(TEXT("SkillCutsceneComponent"));
	QuestComponent = CreateDefaultSubobject<UWxQuestComponent>(TEXT("QuestComponent"));
}
