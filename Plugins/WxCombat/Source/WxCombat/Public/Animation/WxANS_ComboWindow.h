// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxANS_ComboWindow.generated.h"

/**
 * 콤보 입력 수용 구간 AnimNotifyState.
 *
 * 공격 몽타주에 배치하면 NotifyBegin~NotifyEnd 구간 동안
 * 캐릭터 ASC에 ANS.ComboWindow 태그를 부여.
 * 이 구간 내 공격 입력 시 다음 콤보 어빌리티 발동.
 */
UCLASS(DisplayName = "Wx Combo Window")
class WXCOMBAT_API UWxANS_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
