// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_Team.generated.h"

/**
 * 소스 액터가 본 팀 태도(아군·적군·중립)별로 포함 여부를 가리는 타겟팅 필터.
 * 소스 액터가 IGenericTeamAgentInterface 를 구현하지 않으면 아무것도 제외하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_Team : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere)
	bool bIncludeFriendly = false;

	UPROPERTY(EditAnywhere)
	bool bIncludeHostile = true;

	UPROPERTY(EditAnywhere)
	bool bIncludeNeutral = false;
};
