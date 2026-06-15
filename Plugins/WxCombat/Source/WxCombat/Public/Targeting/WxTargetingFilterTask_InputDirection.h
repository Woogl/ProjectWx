// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_InputDirection.generated.h"

/**
 * 이동 입력(스틱) 방향 기준으로 일정 각도를 벗어난 타겟을 제외하는 타겟팅 필터.
 * 소스 폰의 이동 입력 방향(수평)에서 타겟 방향이 MaxInputAngle 을 넘으면 제외한다.
 * 입력이 없으면(스틱 중립) 아무것도 제외하지 않는다. 락온에서 미는 방향의 대상만 추리는 데 쓴다.
 */
UCLASS(Blueprintable)
class WXCOMBAT_API UWxTargetingFilterTask_InputDirection : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** 입력 방향에서 이 각도(도)를 넘어선 타겟을 제외한다. 작을수록 미는 방향의 좁은 콘만 허용. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "180"))
	float MaxInputAngle = 90.f;
};
