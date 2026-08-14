// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_Invincible.generated.h"

/**
 * 구간 동안 ASC에 Effect.Invincible 태그를 부여하며, 그동안 ExecCalc가 대미지를 무시한다.
 *
 * 구간 시작에 그 길이만큼의 GE를 걸어 두고 끝에서 걷어내지 않는다 — 몽타주가 중간에 끊겨도 태그가 남지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_Invincible : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
};
