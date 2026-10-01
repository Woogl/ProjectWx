// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_SkillCutscene.h"
#include "LevelSequence.h"
#if WITH_EDITOR
#include "Animation/WxAnimNotifySettings.h"
#endif

#if WITH_EDITOR
FLinearColor UWxAnimNotify_SkillCutscene::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->PresentationColor;
}
#endif

FString UWxAnimNotify_SkillCutscene::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Cutscene: %s"), Sequence ? *Sequence->GetName() : TEXT("None"));
}
