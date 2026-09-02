// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_SelectedTarget.generated.h"

/**
 * 소스 캐릭터가 이미 선택한 대상만 남기는 타겟팅 필터.
 * 플레이어는 락온 대상을, AI는 지정한 Blackboard Object 키의 액터를 사용하며 선택 대상이 없으면 아무것도 제외하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_SelectedTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/** AI가 선택한 대상 액터를 보관하는 Blackboard Object 키 이름. */
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	FName BlackboardTargetKey = TEXT("TargetActor");
};
