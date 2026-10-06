// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_FullRestore.generated.h"

/** 체크포인트 휴식과 부활이 함께 쓰는 전량 회복이라, 회복할 자원은 여기 한 곳에서 정한다. */
UCLASS()
class WXGAME_API UWxEffect_FullRestore : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_FullRestore();
};
