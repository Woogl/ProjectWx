// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_SnapToTarget.generated.h"

class UTargetingPreset;

/**
 * 모션 워핑 기반 타겟 스냅 AnimNotifyState.
 * UWxRootMotionModifier_SnapToTarget을 이동·회전 두 역할로 구성해 MotionWarpingComponent에 등록한다.
 * 대상 선택(락온 우선)·범위 판정·실제 루트 모션 보정은 전부 modifier가 수행한다.
 *
 * bSnapLocation은 LocationOffset 앞까지 접근하며, 타겟팅 범위 밖이면 이동을 생략한다.
 * bSnapRotation은 거리·LocationOffset과 무관하게 항상 대상 중심을 응시한다.
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
