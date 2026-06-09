// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxPatrolRouteProvider.generated.h"

UINTERFACE(MinimalAPI)
class UWxPatrolRouteProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 정찰 경로를 BT 에 제공하는 측(보통 AIController)이 구현하는 인터페이스.
 *
 * 경로 데이터와 커서 진행 규칙(Loop/PingPong/Once)은 구현부가 캡슐화하고,
 * BT 태스크는 본 인터페이스로 "다음 지점으로 진행"만 요청한다. 실제 목표 좌표는 Blackboard(PatrolTargetLocation)로 발행된다.
 */
class WXAI_API IWxPatrolRouteProvider
{
	GENERATED_BODY()

public:
	/**
	 * 커서를 다음 지점으로 진행하고 Blackboard 의 PatrolTargetLocation 을 갱신한다.
	 * 반환: 진행 후에도 향할 정찰 지점이 있으면 true, 경로가 없거나 Once 로 종료됐으면 false (IEnumerator.MoveNext 패턴).
	 */
	virtual bool AdvanceToNextPatrolPoint() = 0;
};
