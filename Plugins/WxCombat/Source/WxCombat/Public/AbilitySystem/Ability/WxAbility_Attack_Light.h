// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbility_Attack.h"
#include "WxAbility_Attack_Light.generated.h"

/**
 * 약공격 어빌리티.
 * WxAbility_Attack의 콤보 시스템을 약공격 태그로 특수화한다.
 */
UCLASS(Abstract)
class WXCOMBAT_API UWxAbility_Attack_Light : public UWxAbility_Attack
{
	GENERATED_BODY()

public:
	UWxAbility_Attack_Light();
};
