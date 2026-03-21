// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxTeamTypes.generated.h"

/** 캐릭터의 팀 구분 */
UENUM(BlueprintType)
enum class EWxTeam : uint8
{
	Player = 0,
	Enemy = 1,
	Neutral = 2,
};
