// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_Invincible.generated.h"

/**
 * 무적 구간 AnimNotifyState.
 * 구간 동안 ASC에 State.Invincible 태그를 부여하며, 그동안 ExecCalc가 대미지를 무시한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_Invincible : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
