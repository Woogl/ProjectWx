// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "WxWorldSettings.generated.h"

class UWxExperienceDefinition;

/**
 * GameMode 는 진입 URL 다음으로 이 값을 보며, 확정의 마지막 단계다.
 */
UCLASS()
class AWxWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	/** 미지정·미스캔이면 무효 ID 를 반환한다. 진입 URL 도 비어 있었다면 그대로 Experience 미확정이 된다. */
	FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	UPROPERTY(EditAnywhere, Category = "GameMode")
	TSoftObjectPtr<UWxExperienceDefinition> GameplayExperience;
};
