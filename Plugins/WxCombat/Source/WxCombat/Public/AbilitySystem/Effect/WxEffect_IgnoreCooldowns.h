// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_IgnoreCooldowns.generated.h"

/**
 * 모든 어빌리티의 쿨다운을 무시하는 태그를 켜 둔다. 주인의 어빌리티를 따라 쓰는 소환물 등의 AbilitySet이 부여한다.
 * 태그만 세울 뿐이고 쿨다운을 건너뛰는 판정은 UWxAbilityBase가 한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_IgnoreCooldowns : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_IgnoreCooldowns();
};
