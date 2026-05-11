// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * BehaviorTree 가 사용하는 Blackboard 키 이름 모음.
 *
 * 키 SET/CLEAR 는 AIController 또는 UWxAIPerceptionComponent 가 담당하고, 본 namespace 는 BTTask/BTService/관찰자가 키를 이름으로 참조할 때 사용한다.
 * Blackboard 에셋에 같은 이름의 키가 등록돼 있어야 한다.
 */
namespace WxAIBlackboardKeys
{
	WXAI_API extern const FName SelfActor;

	WXAI_API extern const FName TargetActor;

	WXAI_API extern const FName HomeLocation;

	WXAI_API extern const FName TargetLastKnownLocation;

	WXAI_API extern const FName Phase;
}
