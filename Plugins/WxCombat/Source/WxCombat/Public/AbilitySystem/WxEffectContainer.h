// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UGameplayEffect;

/**
 * GameplayEffect 클래스와 SetByCaller 값을 묶어 전달하는 컨테이너.
 * 어빌리티·무기·투사체 등에서 GE 적용 정보를 기술할 때 사용한다.
 */
struct FWxEffectContainer
{
	TSubclassOf<UGameplayEffect> EffectClass;
	TMap<FGameplayTag, float> SetByCallers;
};
