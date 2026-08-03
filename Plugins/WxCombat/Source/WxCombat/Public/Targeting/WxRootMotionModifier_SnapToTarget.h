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
 * 수평 방향/거리(LocationOffset 앞 정지)를 계산해 컴포넌트에 워프 타겟을 등록한다.
 * 부모 SkewWarp 가 그 워프 타겟(WarpTargetName)으로 루트 모션을 보정한다.
 *
 * 노티파이가 이동 역할(LocationOffset 적용·bWarpTranslation)과 회전 역할(오프셋 0·대상 중심·bWarpRotation+Facing)로
 * 각각 인스턴스화한다. 회전 역할은 오프셋이 0이라 대상 중심을 응시해 LocationOffset 의 영향을 받지 않는다.
 *
 * 타겟 판정은 각 머신 로컬에서 이뤄지며, 플레이어 폰의 위치 워프는 복제되는 락온 대상이 있을 때만
 * 허용해 멀티플레이 위치 디싱크를 막는다(회전 워프는 항상 허용).
 *
 * 워프 구간 도중 대상이 죽으면 워프 타겟을 거둬 남은 구간을 순정 루트 모션으로 넘긴다.
 */
UCLASS()
class WXCOMBAT_API UWxRootMotionModifier_SnapToTarget : public URootMotionModifier_SkewWarp
{
	GENERATED_BODY()

public:
	/** 스냅 가능 범위 판정에 쓸 TargetingPreset. 노티파이가 주입한다. */
	UPROPERTY()
	TObjectPtr<UTargetingPreset> TargetingPreset;

	/** 위치 스냅 시 타겟 기준 오프셋(X=대상→오너 앞, Y=우, Z=위). 노티파이가 주입한다. */
	UPROPERTY()
	FVector LocationOffset = FVector(150.0f, 0.0f, 0.0f);

	virtual void OnStateChanged(ERootMotionModifierState LastState) override;

	virtual void Update(const FMotionWarpingUpdateContext& Context) override;

private:
	/** 워프 도중 생존을 다시 보기 위해 활성 시 확정한 스냅 대상을 기억한다. */
	UPROPERTY()
	TWeakObjectPtr<AActor> SnapTarget;

	/** 스냅 대상이 아직 살아 있는지 판정한다. ASC 가 없으면 판정 근거가 없으므로 살아있는 것으로 본다. */
	bool IsSnapTargetAlive() const;
};
