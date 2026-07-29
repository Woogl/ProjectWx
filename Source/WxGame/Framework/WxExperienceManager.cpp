// Copyright Woogle. All Rights Reserved.

#include "Framework/WxExperienceManager.h"

#include "Engine/Engine.h"

#if WITH_EDITOR
void UWxExperienceManager::OnPlayInEditorBegun()
{
	// 직전 PIE 가 정상 종료됐다면 모든 요청이 해제돼 비어 있어야 한다.
	ensure(GameFeaturePluginRequestCountMap.IsEmpty());
	GameFeaturePluginRequestCountMap.Empty();
}
#endif

void UWxExperienceManager::NotifyOfPluginActivation(const FString& PluginURL)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		UWxExperienceManager* ExperienceManager = GEngine->GetEngineSubsystem<UWxExperienceManager>();
		check(ExperienceManager);

		ExperienceManager->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL)++;
	}
#endif
}

bool UWxExperienceManager::RequestToDeactivatePlugin(const FString& PluginURL)
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		UWxExperienceManager* ExperienceManager = GEngine->GetEngineSubsystem<UWxExperienceManager>();
		check(ExperienceManager);

		int32& Count = ExperienceManager->GameFeaturePluginRequestCountMap.FindChecked(PluginURL);
		--Count;
		if (Count == 0)
		{
			ExperienceManager->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}

		return false;
	}
#endif

	return true;
}
