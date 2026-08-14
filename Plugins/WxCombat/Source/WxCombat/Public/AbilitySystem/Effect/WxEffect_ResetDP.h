// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_ResetDP.generated.h"

/**
 * DP를 0으로 즉시 리셋하는 GameplayEffect.
 *
 * 적용 시 DP가 0으로 떨어지고 그 어트리뷰트 변경을 구독하는 그로기 어빌리티가 스스로 종료하므로, DP 직접 set이 아니라 이 GE로 적용해야 그로기가 정상 해제된다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_ResetDP : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_ResetDP();
};
