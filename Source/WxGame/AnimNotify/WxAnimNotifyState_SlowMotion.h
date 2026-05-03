// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_SlowMotion.generated.h"

/**
 * 전역 슬로우모션 AnimNotifyState.
 *
 * NotifyBegin~NotifyEnd 구간 동안 World의 Global TimeDilation을 TargetDilation으로 설정.
 * GameState의 UWxTimeDilationComponent를 통해 서버 권위로 적용되며, 모든 클라이언트에 복제된다.
 * 서버가 아닌 머신에서 호출되면 컴포넌트 내부에서 무시되므로 안전.
 */
UCLASS(DisplayName = "Wx Slow Motion")
class WXGAME_API UWxAnimNotifyState_SlowMotion : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 구간 동안 적용할 Global TimeDilation 값 (1.0 = 평상시, 0.2 = 20% 속도) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|SlowMotion", meta = (ClampMin = "0.01", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float TargetDilation = 0.2f;
};
