// Copyright Woogle. All Rights Reserved.

#include "Framework/WxWorldSettings.h"

#include "Engine/AssetManager.h"
#include "WxGame.h"

FPrimaryAssetId AWxWorldSettings::GetDefaultGameplayExperience() const
{
	if (DefaultGameplayExperience.IsNull())
	{
		return FPrimaryAssetId();
	}

	// 스캔된 에셋만 ID 로 해석된다 — 스캔 폴더 밖의 에셋을 지정한 실수를 조기에 드러낸다.
	const FPrimaryAssetId Result = UAssetManager::Get().GetPrimaryAssetIdForPath(DefaultGameplayExperience.ToSoftObjectPath());
	if (!Result.IsValid())
	{
		UE_LOG(LogWxGame, Error, TEXT("GetDefaultGameplayExperience: '%s' 가 AssetManager 에 스캔돼 있지 않음. 스캔 폴더(/Game/Framework) 확인 필요."), *DefaultGameplayExperience.ToString());
	}

	return Result;
}
