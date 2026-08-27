// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "WxBTService_TargetDistance.generated.h"

/**
 * BT Service: SelfActor 와 TargetActor 사이 거리를 Blackboard 의 TargetDistance(Float 키) 에 주기적으로 기록한다.
 *
 * 거리 비교 자체는 엔진 기본 Blackboard 데코레이터(arithmetic 비교: Less/Greater 등) 가 TargetDistance 키를 읽어 처리한다.
 *
 * TargetActor 가 없으면 stale 거리 대신 NoTargetDistance 를 기록해, 근거리 비교가 타겟 부재 시 통과하지 않게 한다.
 */
UCLASS()
class WXAI_API UWxBTService_TargetDistance : public UBTService
{
	GENERATED_BODY()

public:
	UWxBTService_TargetDistance();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
