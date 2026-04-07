// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_LineTrace.generated.h"

/**
 * 지정된 콜리전 채널로 라인 트레이스를 수행하여 타겟을 필터링하는 타겟팅 필터.
 * 소스 액터에서 타겟 액터로의 트레이스가 차단되면 해당 타겟을 제외한다.
 * 예: ECC_Visibility 채널로 시야가 차단된 타겟을 제거.
 */
UCLASS(Blueprintable)
class WXCOMBAT_API UWxTargetingFilterTask_LineTrace : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** 트레이스에 사용할 콜리전 채널 */
	UPROPERTY(EditAnywhere)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** 소스 액터의 트레이스 시작 위치 오프셋 (로컬 스페이스) */
	UPROPERTY(EditAnywhere)
	FVector SourceOffset = FVector::ZeroVector;

	/** 타겟 액터의 트레이스 끝 위치 오프셋 (로컬 스페이스) */
	UPROPERTY(EditAnywhere)
	FVector TargetOffset = FVector::ZeroVector;
};
