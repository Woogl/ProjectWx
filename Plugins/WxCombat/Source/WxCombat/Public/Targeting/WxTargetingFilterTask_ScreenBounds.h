// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_ScreenBounds.generated.h"

/**
 * 타겟이 카메라 뒤이거나 화면(뷰포트) 밖에 있으면 제외하는 타겟팅 필터.
 * 플레이어 컨트롤러가 없으면(AI) 아무것도 제외하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_ScreenBounds : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** 화면 가장자리에서 이 비율(뷰포트 크기 대비)만큼 안쪽으로 좁힌 영역만 보임으로 본다. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "0.49"))
	float ScreenEdgeMargin = 0.f;
};
