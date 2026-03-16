// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_Guard.generated.h"

/**
 * 가드 판정 구간 AnimNotifyState.
 *
 * 몽타주에 배치하면 NotifyBegin~NotifyEnd 구간 동안
 * 캐릭터 ASC에 ANS.Guard 태그를 부여.
 * 이 태그가 있는 동안 데미지 감소 및 가드 히트 리액션이 발동한다.
 */
UCLASS(DisplayName = "Wx Guard")
class WXCOMBAT_API UWxAnimNotifyState_Guard : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
