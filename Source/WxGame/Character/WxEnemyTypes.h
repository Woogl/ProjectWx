// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxEnemyTypes.generated.h"

/** 적의 게임플레이 등급. 역할 판별의 단일 기준으로 사용한다. */
UENUM(BlueprintType)
enum class EWxEnemyRank : uint8
{
	Normal,
	Elite,
	Boss,
};
