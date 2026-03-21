// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_TeamFilter.generated.h"

/**
 * 팀(아군)을 기준으로 타겟 목록에서 제외하는 타겟팅 필터.
 * IGenericTeamAgentInterface를 구현한 액터에 대해 팀 태도를 비교하여 필터링한다.
 */
UCLASS(Blueprintable)
class WXCOMBAT_API UWxTargetingFilterTask_TeamFilter : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** 아군을 타겟 목록에 포함할지 여부 */
	UPROPERTY(EditAnywhere)
	bool bIncludeFriendly = false;

	/** 적군을 타겟 목록에 포함할지 여부 */
	UPROPERTY(EditAnywhere)
	bool bIncludeHostile = true;

	/** 중립을 타겟 목록에 포함할지 여부 */
	UPROPERTY(EditAnywhere)
	bool bIncludeNeutral = false;
};
