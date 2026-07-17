// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_GameplayTag.generated.h"

/**
 * 대상이 보유한 태그 조건으로 필터링.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_GameplayTag : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()
	
protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
	
	UPROPERTY(EditAnywhere)
	FGameplayTagRequirements TagRequirements;
};
