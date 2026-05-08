// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 에너미 BehaviorTree 가 사용하는 Blackboard 키 이름 모음.
 *
 * 키 SET/CLEAR 는 AWxEnemyController 가 담당하고, 본 namespace 는 BTTask/BTService/관찰자가
 * 키를 이름으로 참조할 때 사용한다. BB_Enemy 에셋에 같은 이름의 키가 등록돼 있어야 한다.
 */
namespace WxEnemyBlackboardKeys
{
	WXGAME_API extern const FName SelfActor;

	WXGAME_API extern const FName TargetActor;

	WXGAME_API extern const FName HomeLocation;

	WXGAME_API extern const FName TargetLastKnownLocation;
}
