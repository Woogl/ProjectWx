// Copyright Woogle. All Rights Reserved.

#include "WxBTService_TargetDistance.h"

#include "WxBlackboardKeys.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

UWxBTService_TargetDistance::UWxBTService_TargetDistance()
{
	NodeName = TEXT("Update Target Distance");

	bNotifyTick = true;
	// 서브트리 진입 첫 평가에서 데코레이터가 신선한 거리를 읽도록 진입 시 즉시 1회 갱신한다.
	bCallTickOnSearchStart = true;

	Interval = 0.1f;
	RandomDeviation = 0.0f;
}

void UWxBTService_TargetDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return;
	}

	const AActor* Self = WxBlackboardKeys::GetSelfActor(Blackboard);
	const AActor* Target = WxBlackboardKeys::GetTargetActor(Blackboard);
	if (!Self || !Target)
	{
		// 타겟이 없으면 거리 키를 비워 stale 값을 남기지 않는다(데코레이터의 IsSet 게이팅 가능).
		WxBlackboardKeys::ClearTargetDistance(Blackboard);
		return;
	}

	const FVector SelfLocation = Self->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();
	const float Distance = FVector::Dist(SelfLocation, TargetLocation);

	WxBlackboardKeys::SetTargetDistance(Blackboard, Distance);
}
