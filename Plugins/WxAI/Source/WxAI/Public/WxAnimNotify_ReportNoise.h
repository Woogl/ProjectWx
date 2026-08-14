// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_ReportNoise.generated.h"

/**
 * 애님의 특정 프레임에서 UAISense_Hearing::ReportNoiseEvent 로 소음을 발생시켜 주변 AI 청각이 감지하게 한다(서버 전용).
 *
 * 엔진 구조상 청취 거리는 청취 AI 의 HearingRange 를 초과할 수 없어, 실제 청취 거리 = min(HearingDistance, 청취자 HearingRange) 가 된다.
 */
UCLASS(DisplayName = "Wx Report Noise")
class WXAI_API UWxAnimNotify_ReportNoise : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 소음이 들리는 거리(cm). 청취 AI 의 HearingRange 가 상한이다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	float HearingDistance = 300.f;
};
