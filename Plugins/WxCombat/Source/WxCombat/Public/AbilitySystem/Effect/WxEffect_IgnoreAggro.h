// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_IgnoreAggro.generated.h"

/**
 * 어그로 대상에서 빠지는 태그를 켜 둔다. 소환물 등 노려지지 않을 액터의 AbilitySet 이 부여한다.
 * 태그만 세울 뿐이고 대상으로 삼을지 말지의 판정은 AI 쪽 BT 서비스가 한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_IgnoreAggro : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_IgnoreAggro();
};
