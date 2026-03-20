// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_RotateToTarget.generated.h"

class UTargetingPreset;

/**
 * 타겟 방향 회전 AnimNotifyState.
 *
 * 공격 몽타주에 배치하면 NotifyBegin에서 TargetingPreset으로 가장 가까운 적을 탐색하고,
 * NotifyTick 구간 동안 해당 적 방향으로 부드럽게 회전한다.
 */
UCLASS(DisplayName = "Wx Rotate To Target")
class WXCOMBAT_API UWxAnimNotifyState_RotateToTarget : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 가장 가까운 적을 탐색하기 위한 타겟팅 프리셋 */
	UPROPERTY(EditAnywhere, Category = "Wx|Targeting")
	TObjectPtr<UTargetingPreset> TargetingPreset;

private:
	/** Owner별 타겟 캐싱. ANS 인스턴스는 공유되므로 Owner를 키로 분리 저장 */
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AActor>> OwnerToTargetMap;

	static constexpr float RotationTolerance = 3.f;
};
