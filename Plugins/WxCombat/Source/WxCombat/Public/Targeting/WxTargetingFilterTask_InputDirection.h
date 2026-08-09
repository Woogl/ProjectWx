// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_InputDirection.generated.h"

class AActor;
class APawn;

/**
 * 소스 폰의 이동 입력 방향(수평)에서 타겟 방향이 MaxInputAngle 을 넘으면 제외하는 타겟팅 필터.
 * 락온에서 미는 방향의 대상만 추리는 데 쓴다.
 * 입력이 없으면(스틱 중립) 아무것도 제외하지 않는다.
 * 콘 안에 드는 후보가 하나도 없을 때는 bKeepAllWhenNoMatch 에 따라 전체를 유지하거나 전부 제외한다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_InputDirection : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/**
	 * 입력 방향에서 이 각도(도)를 넘어선 타겟을 제외한다.
	 * 작을수록 미는 방향의 좁은 콘만 허용.
	 */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "180"))
	float MaxInputAngle = 90.f;

	/**
	 * 켜지면(기본) 입력 콘 안에 드는 후보가 하나도 없을 때 아무도 제외하지 않는다(전부 유지).
	 * 꺼지면 콘 밖 후보는 항상 제외한다.
	 */
	UPROPERTY(EditAnywhere)
	bool bKeepAllWhenNoMatch = true;

private:
	/** InputDirNormalized 는 수평으로 평탄화해 정규화한 입력 방향이어야 한다. */
	bool PassesInputCone(const APawn& SourcePawn, const FVector& InputDirNormalized, const AActor& TargetActor) const;
};
