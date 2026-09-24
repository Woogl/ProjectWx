// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_SkillCutscene.h"
#include "LevelSequence.h"
#include "System/WxCombatDeveloperSettings.h"

FLinearColor UWxAnimNotify_SkillCutscene::GetEditorColor()
{
	return GetDefault<UWxCombatDeveloperSettings>()->CombatAnimNotifyColor;
}

FString UWxAnimNotify_SkillCutscene::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Cutscene: %s"), Sequence ? *Sequence->GetName() : TEXT("None"));
}
