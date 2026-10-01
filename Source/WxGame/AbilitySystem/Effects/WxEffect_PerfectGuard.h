// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_PerfectGuard.generated.h"

/**
 * 정의에 지속시간이 없다 — 몽타주 노티파이 구간이 수명을 정하고, 구간 끝에서 몽타주 소유 어빌리티가 걷어낸다.
 */
UCLASS()
class WXGAME_API UWxEffect_PerfectGuard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_PerfectGuard();
};
