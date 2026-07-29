// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "WxWorldSettings.generated.h"

class UWxExperienceDefinition;

/**
 * 프로젝트 공용 WorldSettings.
 * 맵이 자기 기본 Experience 를 지정하는 자리다 — GameMode 는 진입 URL 다음, 자체 폴백 이전에 이 값을 본다.
 */
UCLASS()
class AWxWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	/** 이 맵의 기본 Experience 의 프라이머리 에셋 ID. 미지정·미스캔이면 무효 ID 를 반환해 GameMode 가 다음 폴백으로 넘어간다. */
	FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	/** 이 맵을 그냥 열었을 때 쓸 기본 Experience. */
	UPROPERTY(EditAnywhere, Category = "GameMode")
	TSoftObjectPtr<UWxExperienceDefinition> DefaultGameplayExperience;
};
