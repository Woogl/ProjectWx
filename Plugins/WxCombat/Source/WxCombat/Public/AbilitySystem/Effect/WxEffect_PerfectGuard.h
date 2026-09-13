// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_PerfectGuard.generated.h"

/**
 * 정의에 지속시간이 없다 — 구간을 여는 노티파이가 수명을 쥐고 구간 끝에서 걷어낸다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_PerfectGuard : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_PerfectGuard();
};
