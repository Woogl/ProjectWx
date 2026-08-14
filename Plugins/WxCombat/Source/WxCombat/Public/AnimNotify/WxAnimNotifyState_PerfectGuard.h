// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_PerfectGuard.generated.h"

/**
 * 구간 동안 ASC에 Effect.PerfectGuard 태그를 부여하며, 이때 피격되면 ExecCalc가 대미지를 무효화하고 공격자에게 DP를 반사한다.
 *
 * 구간 시작에 그 길이만큼의 GE를 걸어 두고 끝에서 걷어내지 않는다 — 가드가 취소돼도 영구 패링이 되지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_PerfectGuard : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
};
