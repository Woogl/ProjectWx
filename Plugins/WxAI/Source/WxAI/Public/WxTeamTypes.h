// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxTeamTypes.generated.h"

UENUM(BlueprintType)
enum class EWxTeam : uint8
{
	Player = 0,
	Enemy = 1,
	Neutral = 255,
};
