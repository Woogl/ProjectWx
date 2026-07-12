// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_SnapToTarget.generated.h"

class UTargetingPreset;

/**
 * 모션 워핑 기반 타겟 스냅 AnimNotifyState.
 *
 * NotifyBegin에서 락온 대상을 우선 사용하고, TargetingPreset 쿼리 결과로 "스냅 가능한 타겟팅 범위"를 판정한다.
 * MotionWarpingComponent에 SkewWarp modifier를 직접 등록하여 루트 모션을 보정, 타겟 쪽으로 이동·회전시킨다.
 *
 * bSnapLocation: 타겟 위치로 접근할지 여부 (MinDistance만큼 앞에서 멈춤).
 *                타겟팅 범위 밖이면 회전만 적용하고 이동은 생략한다.
 * bSnapRotation: 타겟 방향으로 회전할지 여부. 거리와 무관하게 적용된다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_SnapToTarget : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	UPROPERTY(EditAnywhere, Category = "Wx|Snap")
	bool bSnapLocation = false;

	UPROPERTY(EditAnywhere, Category = "Wx|Snap")
	bool bSnapRotation = true;

	UPROPERTY(EditAnywhere, Category = "Wx|Snap", meta = (EditCondition = "bSnapLocation", ClampMin = "0.0"))
	float MinDistance = 150.0f;

private:
	static const FName DefaultWarpTargetName;
};
