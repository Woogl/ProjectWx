// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_ResetDP.generated.h"

/**
 * DP를 0으로 즉시 리셋하는 GameplayEffect.
 *
 * 적용 시 PostAttributeChange의 캐스케이드가 State.Groggy를 제거하므로, DP 직접 set이 아니라 이 GE로 적용해야 그로기가 정상 해제된다.
 * 앞잡 몽타주가 정상 완료되면 UWxAbility_Finisher가 대상에 적용한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_ResetDP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_ResetDP();
};
