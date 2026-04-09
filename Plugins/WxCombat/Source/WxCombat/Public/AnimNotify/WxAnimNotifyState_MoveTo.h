// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_MotionWarping.h"
#include "WxAnimNotifyState_MoveTo.generated.h"

class UTargetingPreset;

/**
 * 모션 워핑 기반 타겟 방향 이동 AnimNotifyState.
 *
 * UAnimNotifyState_MotionWarping을 상속하여,
 * NotifyBegin에서 타겟을 탐색해 MotionWarpingComponent에 WarpTarget을 등록하고,
 * 엔진의 모션 워핑 시스템이 루트 모션을 보정하여 타겟 방향으로 이동한다.
 * MinDistance로 타겟에 얼마나 가까이 접근할지 설정할 수 있다.
 */
UCLASS(DisplayName = "Wx Move To")
class WXCOMBAT_API UWxAnimNotifyState_MoveTo : public UAnimNotifyState_MotionWarping
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual URootMotionModifier* AddRootMotionModifier_Implementation(UMotionWarpingComponent* MotionWarpingComp, const UAnimSequenceBase* Animation, float StartTime, float EndTime) const override;

	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif

protected:
	/** 가장 가까운 적을 탐색하기 위한 타겟팅 프리셋 */
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	TObjectPtr<UTargetingPreset> TargetingPreset;

	/** 타겟과의 최소 거리. 이 거리 이내로는 접근하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|MoveTo", meta = (ClampMin = "0.0"))
	float MinDistance = 100.0f;

private:
	void ApplyDefaultWarpSettings() const;

	bool bSavedOrientRotationToMovement = false;

	static const FName DefaultWarpTargetName;
};
