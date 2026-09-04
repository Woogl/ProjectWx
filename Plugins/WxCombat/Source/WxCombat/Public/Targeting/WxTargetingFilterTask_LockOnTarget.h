// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "WxTargetingFilterTask_LockOnTarget.generated.h"

/**
 * 소스 캐릭터가 겨누는 락온 대상(UWxLockOnComponent)만 남기는 타겟팅 필터.
 * 플레이어 락온과 AI 지정 대상이 같은 복제 값이라 머신끼리 갈리지 않는다 — 다만 복제가 아직 닿지 않은 원격은 그 몇 프레임 동안 대상 없음으로 읽는다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingFilterTask_LockOnTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;

	/**
	 * 겨누는 대상이 없을 때 이 필터를 건너뛴다(=아무것도 제외하지 않는다). 끄면 대신 결과를 통째로 비운다.
	 * 끈 프리셋을 스냅 워프(UWxRootMotionModifier_SnapToTarget)가 쓰면 락온하지 않은 동안 폴백 후보까지 사라져 회전 스냅도 함께 꺼진다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	bool bSkipIfEmpty = true;
};
