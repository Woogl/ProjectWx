// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_HealByMaxHPPercent.generated.h"

/**
 * HP를 MaxHP의 40%만큼 즉시 회복시키는 GameplayEffect.
 * 포션(다크소울 에스트병 방식) 등 비율 회복 소비 아이템의 회복 효과로 사용한다.
 * HP는 어트리뷰트 세트에서 [0, MaxHP]로 클램프되므로 오버힐은 발생하지 않는다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_HealByMaxHPPercent : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_HealByMaxHPPercent();
};
