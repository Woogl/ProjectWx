// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "WxTargetingSorterTask_InputDirection.generated.h"

/**
 * 소스 폰의 이동 입력 방향(수평)과 타겟 방향의 사이각이 작을수록 앞에 온다.
 * 입력이 없으면(스틱 중립) 모두 같은 점수를 줘 순서에 관여하지 않는다.
 *
 * 입력 방향은 CharacterMovementComponent의 Acceleration에서 읽는다 — 서버가 ServerMove로 받은 클라이언트 값을 그대로 넣어 주므로 머신 간 판정이 일치한다.
 */
UCLASS()
class WXCOMBAT_API UWxTargetingSorterTask_InputDirection : public UTargetingSortTask_Base
{
	GENERATED_BODY()

protected:
	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
