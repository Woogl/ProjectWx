// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotify_AbilityEvent.generated.h"

/** 실행 상태를 보관하지 않고 몽타주 소유자에게 로컬 타이밍 신호만 전달한다. */
UCLASS(Abstract)
class WXGAME_API UWxAnimNotify_AbilityEvent : public UAnimNotify
{

	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};

/** Queued와 BranchingPoint 모두 원본 몽타주 인스턴스와 구간 식별자를 보존한다. */
UCLASS(Abstract)
class WXGAME_API UWxAnimNotifyState_AbilityEvent : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;
};
