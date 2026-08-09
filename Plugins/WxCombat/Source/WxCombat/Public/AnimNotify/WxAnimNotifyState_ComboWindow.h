// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_ComboWindow.generated.h"

/**
 * 콤보 입력 수용 구간 AnimNotifyState.
 * 구간 동안 ASC에 ANS.ComboWindow 태그를 부여하며, 이 안에서 들어온 공격 입력이 다음 콤보로 이어진다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
