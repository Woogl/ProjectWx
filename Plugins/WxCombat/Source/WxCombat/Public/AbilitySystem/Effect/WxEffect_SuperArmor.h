// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_SuperArmor.generated.h"

/**
 * 슈퍼 아머 상태(Effect.SuperArmor)를 부여한다. 부여하는 어빌리티가 도는 동안 유지돼 고정 지속시간이 없으므로,
 * 수명은 그 어빌리티가 끊는다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_SuperArmor : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_SuperArmor();
};
