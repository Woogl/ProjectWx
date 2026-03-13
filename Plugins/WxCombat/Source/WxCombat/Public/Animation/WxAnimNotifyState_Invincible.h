// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_Invincible.generated.h"

/**
 * 무적 구간 AnimNotifyState.
 *
 * 몽타주에 배치하면 NotifyBegin~NotifyEnd 구간 동안
 * 캐릭터 ASC에 ANS.Invincible 태그를 부여.
 * 이 태그가 있는 동안 대미지 계산(ExecCalc)에서 대미지를 무시한다.
 */
UCLASS(DisplayName = "Wx Invincible")
class WXCOMBAT_API UWxAnimNotifyState_Invincible : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
