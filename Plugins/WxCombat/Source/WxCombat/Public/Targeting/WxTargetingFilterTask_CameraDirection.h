// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_CameraDirection.generated.h"

/**
 * 카메라(시점) 정면 기준으로 일정 각도를 벗어난 타겟을 제외하는 타겟팅 필터.
 * 소스 폰의 컨트롤러 시점에서 타겟으로의 방향이 정면과 이루는 각이 MaxViewAngle 을 넘으면 해당 타겟을 제외한다.
 * 락온에서 카메라가 향하는 반대 방향(후방)의 대상을 후보에서 빼는 데 쓴다.
 */
UCLASS(Blueprintable)
class WXCOMBAT_API UWxTargetingFilterTask_CameraDirection : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** 카메라 정면에서 이 각도(도)를 넘어선 타겟을 제외한다. 90이면 카메라 뒤(후방)를 제외, 작을수록 정면 좁은 콘만 허용. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "180"))
	float MaxViewAngle = 90.f;
};
