// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "WxRootMotionModifier_SnapToTarget.generated.h"

class UTargetingPreset;

/**
 * 타겟 스냅용 SkewWarp RootMotionModifier.
 *
 * UWxAnimNotifyState_SnapToTarget 이 생성해 MotionWarpingComponent 에 등록한다.
 * Waiting→Active 진입 시 락온 대상(우선)·TargetingPreset 쿼리로 스냅 타겟을 판정하고,
 * 수평 방향/거리(MinDistance 앞 정지)를 계산해 컴포넌트에 워프 타겟을 등록한다.
 * 부모 SkewWarp 가 그 워프 타겟(WarpTargetName)으로 루트 모션을 보정한다.
 *
 * 타겟 판정은 각 머신 로컬에서 이뤄지며, 플레이어 폰의 위치 워프는 복제되는 락온 대상이 있을 때만
 * 허용해 멀티플레이 위치 디싱크를 막는다(회전 워프는 항상 허용).
 */
UCLASS()
class WXCOMBAT_API UWxRootMotionModifier_SnapToTarget : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	/** 스냅 가능 범위 판정에 쓸 TargetingPreset. 노티파이가 주입한다. */
	UPROPERTY()
	TObjectPtr<UTargetingPreset> TargetingPreset;

	/** 위치 스냅 시 타겟에서 이만큼 앞에 정지한다. 노티파이가 주입한다. */
	UPROPERTY()
	float MinDistance = 150.0f;

protected:
	virtual void OnStateChanged(ERootMotionModifierState LastState) override;
};
