// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_ReportNoise.generated.h"

/**
 * AI 소음 발생 AnimNotify.
 *
 * 애님의 특정 프레임에서 UAISense_Hearing::ReportNoiseEvent 로 소음을 발생시켜 주변 AI 청각이 감지하게 한다(서버 전용).
 * 발 접지(발소리)·착지 등 AI 에게 들려야 하는 순간에 배치한다.
 * 청취 거리는 HearingDistance(cm)로 직접 지정한다.
 * 단 엔진 구조상 청취 AI 의 HearingRange 를 초과할 수 없어, 실제 청취 거리 = min(HearingDistance, 청취자 HearingRange) 가 된다.
 * 따라서 HearingDistance 를 키우려면 청취 AI 의 HearingRange 도 그만큼 확보해야 한다.
 */
UCLASS(DisplayName = "Wx Report Noise")
class WXAI_API UWxAnimNotify_ReportNoise : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 소음이 들리는 거리(cm). 기본 300 = 3m (청취 AI 의 HearingRange 가 상한) */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	float HearingDistance = 300.f;
};
