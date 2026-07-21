// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_PerfectGuard.generated.h"

/**
 * 퍼펙트 가드 판정 구간 AnimNotifyState.
 *
 * 몽타주에 배치하면 NotifyBegin~NotifyEnd 구간 동안 캐릭터 ASC에 State.PerfectGuard 태그를 부여.
 * 이 구간에서 피격 시 WxExecCalc_Damage가 대미지를 무효화하고 공격자에게 DP를 반사한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_PerfectGuard : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
