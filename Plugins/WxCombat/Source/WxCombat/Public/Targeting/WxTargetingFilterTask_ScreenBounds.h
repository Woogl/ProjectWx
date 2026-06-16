// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_ScreenBounds.generated.h"

/**
 * 타겟이 현재 카메라 화면(뷰포트) 밖에 있으면 제외하는 타겟팅 필터.
 * 소스 폰의 플레이어 컨트롤러로 타겟 위치를 화면 좌표에 투영해, 카메라 뒤이거나 화면 영역을 벗어나면 해당 타겟을 제외한다.
 * 락온에서 화면에 보이지 않는 대상을 후보에서 빼는 데 쓴다. 플레이어 컨트롤러가 없으면(AI) 아무것도 제외하지 않는다.
 */
UCLASS(Blueprintable)
class WXCOMBAT_API UWxTargetingFilterTask_ScreenBounds : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** 화면 가장자리에서 이 비율(뷰포트 크기 대비)만큼 안쪽으로 좁힌 영역만 보임으로 본다. 0이면 정확히 화면 경계. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "0.49"))
	float ScreenEdgeMargin = 0.f;
};
