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

	/**
	 * 현재 목표 정찰 지점(Vector). 컨트롤러(IWxPatrolRouteProvider)가 발행하고 BT 의 MoveTo 가 소비한다.
	 * 정찰 서브트리 게이트도 이 키의 Set 여부로 판단하므로(경로 없음/Once 완료 시 ClearValue), 별도 boolean 키는 두지 않는다.
	 */
	WXAI_API extern const FName PatrolTargetLocation;
}
