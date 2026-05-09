// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_Exceed.generated.h"

/**
 * Exceed 버프 GameplayEffect.
 *
 * 6초 동안 ATK와 ASPD를 각각 20% 증가시킨다.
 * 대상당 1스택만 유지하며 재적용 시 Duration을 갱신한다 (덮어쓰기).
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxEffect_Exceed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_Exceed();
};
