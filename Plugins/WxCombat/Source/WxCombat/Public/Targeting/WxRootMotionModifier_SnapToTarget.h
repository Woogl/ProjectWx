// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RootMotionModifier_SkewWarp.h"
#include "WxRootMotionModifier_SnapToTarget.generated.h"

class UTargetingPreset;

/**
 * Active 진입 시 스냅 타겟을 정하고 워프 타겟을 등록하면, 부모 SkewWarp가 그 타겟으로 루트 모션을 보정한다.
 * 노티파이가 이동 역할과 회전 역할로 각각 인스턴스화한다.
 *
 * 대상은 오너가 겨누는 지정 대상(UWxLockOnComponent)이 1순위이며, 플레이어 락온과 AI 지정 대상이 모두 여기로 모인다.
 * 서버 권위로 복제되는 값이라 전 머신이 같은 액터를 향해 워프한다.
 *
 * 지정 대상이 없으면 TargetingPreset 쿼리 결과를 폴백으로 쓰는데, 이 판정은 머신마다 갈릴 수 있어 회전에만 허용한다.
 * 이동 워프는 지정 대상이 스냅 범위 안에 있을 때만 걸고, 아니면 자기 워프 타겟을 지워 부모가 modifier를 끄게 한다.
 * 지정 대상이 늦게 도착하거나 바뀌면 Update가 회전을 그쪽으로 옮겨 정합을 맞춘다. 이동은 진입 시 정한 대상을 끝까지 쓴다 — 창 도중에 바꾸면 부모가 남은 거리를 메우느라 튄다.
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
	/** 조건이 서지 않으면 워프 타겟을 지워 modifier를 끈다. */
	void ApplySnapTarget();

	/** 워프 도중 생존을 다시 보기 위해 기억해 두는 확정 대상 */
	UPROPERTY()
	TWeakObjectPtr<AActor> SnapTarget;

	/** ASC가 없으면 판정 근거가 없으므로 살아있는 것으로 본다 */
	bool IsSnapTargetAlive() const;
};
