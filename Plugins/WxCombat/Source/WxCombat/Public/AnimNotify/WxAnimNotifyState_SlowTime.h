// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_SlowTime.generated.h"

/**
 * 구간이 열려 있는 동안 전역 시간을 느리게 흘린다. 퍼펙트 회피·퍼펙트 가드의 슬로우 모션이 이걸로 열린다.
 *
 * 배율을 직접 만지지 않고 구간의 시작과 길이를 어빌리티 태스크에 넘긴다 — 되돌리는 책임이 어빌리티 수명에 묶여야 몽타주가 어떻게 끝나든 배율이 남지 않는다.
 * 애니메이팅 어빌리티가 없는 Persona 프리뷰에서는 아예 걸리지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_SlowTime : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	/** 엔진이 Min/MaxGlobalTimeDilation으로 클램프한다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0.01"))
	float TimeDilation = 0.4f;
};
