// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimNotifyState_MotionWarping.h"
#include "WxAnimNotifyState_TurnAround.generated.h"

class UTargetingPreset;

/**
 * 모션 워핑 기반 타겟 방향 회전 AnimNotifyState.
 *
 * UAnimNotifyState_MotionWarping을 상속하여,
 * NotifyBegin에서 타겟을 탐색해 MotionWarpingComponent에 WarpTarget을 등록하고,
 * 엔진의 모션 워핑 시스템이 루트 모션을 보정하여 타겟 방향으로 회전한다.
 */
UCLASS(DisplayName = "Wx Turn Around")
class WXCOMBAT_API UWxAnimNotifyState_TurnAround : public UAnimNotifyState_MotionWarping
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

private:
	void ApplyDefaultWarpSettings() const;

	bool bSavedOrientRotationToMovement = false;

	static const FName DefaultWarpTargetName;
};
