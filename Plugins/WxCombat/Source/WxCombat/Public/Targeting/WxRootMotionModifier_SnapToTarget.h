// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "WxRootMotionModifier_SnapToTarget.generated.h"

class UTargetingPreset;

/**
 * 타겟 스냅용 SkewWarp RootMotionModifier.
 * Active 진입 시 락온 대상(우선)이나 TargetingPreset 쿼리로 스냅 타겟을 정하고 워프 타겟을 등록하면, 부모 SkewWarp가 그 타겟으로 루트 모션을 보정한다.
 * 노티파이가 이동 역할과 회전 역할로 각각 인스턴스화한다.
 *
 * 타겟 판정은 각 머신 로컬에서 이뤄진다.
 * 플레이어 폰의 위치 워프만 복제되는 락온 대상이 있을 때로 제한해 멀티플레이 디싱크를 막고, 회전 워프는 항상 허용한다.
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
	/** 워프 도중 생존을 다시 보기 위해 기억해 두는 확정 대상 */
	UPROPERTY()
	TWeakObjectPtr<AActor> SnapTarget;

	/** ASC가 없으면 판정 근거가 없으므로 살아있는 것으로 본다 */
	bool IsSnapTargetAlive() const;
};
