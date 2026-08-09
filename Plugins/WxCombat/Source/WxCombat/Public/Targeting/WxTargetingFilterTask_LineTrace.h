// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_LineTrace.generated.h"

/**
 * 소스 액터에서 타겟 액터로의 TraceChannel 트레이스가 차단되면 해당 타겟을 제외하는 타겟팅 필터.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_LineTrace : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	UPROPERTY(EditAnywhere)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** 소스 액터의 트레이스 시작 위치 오프셋 (로컬 스페이스) */
	UPROPERTY(EditAnywhere)
	FVector SourceOffset = FVector::ZeroVector;

	/** 타겟 액터의 트레이스 끝 위치 오프셋 (로컬 스페이스) */
	UPROPERTY(EditAnywhere)
	FVector TargetOffset = FVector::ZeroVector;
};
