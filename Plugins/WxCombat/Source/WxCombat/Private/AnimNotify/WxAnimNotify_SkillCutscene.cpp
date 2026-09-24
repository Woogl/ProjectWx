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
	if (!Sequence)
	{
		return Super::GetNotifyName_Implementation();
	}

	return Sequence->GetName();
}
