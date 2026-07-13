// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_Footstep.generated.h"

/**
 * 발소리 AnimNotify.
 *
 * 보행/달리기 애니메이션의 발 접지 프레임에 배치한다.
 * 발 접지 시 AI 소음(서버 전용 fire-and-forget 신호)을 직접 발생시키고, 표면별 코스메틱(발소리/VFX)은 소유 액터의 UWxContextEffectsComponent 에 위임한다.
 * 청취 거리는 HearingDistance(cm)로 직접 지정한다.
 * 단 엔진 구조상 청취 AI 의 HearingRange 를 초과할 수 없어, 실제 청취 거리 = min(HearingDistance, 청취자 HearingRange) 가 된다.
 * 따라서 HearingDistance 를 키우려면 청취 AI 의 HearingRange 도 그만큼 확보해야 한다.
 */
UCLASS(DisplayName = "Wx Footstep")
class WXGAME_API UWxAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 발소리가 들리는 거리(cm). 기본 300 = 3m (청취 AI 의 HearingRange 가 상한) */
	UPROPERTY(EditAnywhere, Category = "Wx|Footstep")
	float HearingDistance = 300.f;
};
