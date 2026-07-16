// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_SnapToTarget.generated.h"

class UTargetingPreset;

/**
 * 모션 워핑 기반 타겟 스냅 AnimNotifyState.
 *
 * NotifyBegin에서 UWxRootMotionModifier_SnapToTarget 을 이동·회전 두 역할로 구성해 MotionWarpingComponent 에 등록한다.
 * 락온 대상 우선·TargetingPreset 범위 판정과 실제 루트 모션 보정은 modifier 가 수행한다.
 *
 * 이동(LocationOffset 적용)과 회전(대상 중심 응시)을 별도 modifier·워프 타겟으로 분리해,
 * LocationOffset 이 응시 방향에 영향을 주지 않게 한다.
 *
 * bSnapLocation: 타겟으로 접근할지 여부 (LocationOffset 앞에서 멈춤). 타겟팅 범위 밖이면 이동을 생략한다.
 * bSnapRotation: 타겟 방향으로 회전할지 여부. 거리·LocationOffset 과 무관하게 항상 대상 중심을 응시한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_SnapToTarget : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	UPROPERTY(EditAnywhere, Category = "Wx|Snap")
	bool bSnapLocation = false;

	UPROPERTY(EditAnywhere, Category = "Wx|Snap")
	bool bSnapRotation = true;

	UPROPERTY(EditAnywhere, Category = "Wx|Snap", meta = (EditCondition = "bSnapLocation"))
	FVector LocationOffset = FVector(100.0f, 0.0f, 0.0f);

};
