// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_BeyondLeash.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UWxBTDecorator_BeyondLeash::UWxBTDecorator_BeyondLeash()
{
	NodeName = TEXT("Beyond Leash");

	// TickNode/OnBecomeRelevant 오버라이드를 감지해 알림 플래그(bNotifyTick 등)를 자동 설정한다(엔진 데코 관용).
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();

	// 새 배치의 기본 FlowAbortMode 다.
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
}

FString UWxBTDecorator_BeyondLeash::GetStaticDescription() const
{
	return FString::Printf(TEXT("HomeLocation에서 %.0f 이상 이탈 시 true"), LeashRadius);
}

bool UWxBTDecorator_BeyondLeash::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !Blackboard)
	{
		return false;
	}

	const float DistSquared = FVector::DistSquared(Pawn->GetActorLocation(), WxBlackboardKeys::GetHomeLocation(Blackboard));
	return DistSquared > LeashRadius * LeashRadius;
}

void UWxBTDecorator_BeyondLeash::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// 관찰 시작 시점의 이탈 여부를 시드해, 첫 틱에서 불필요한 재평가 요청이 나가지 않게 한다.
	FWxBeyondLeashMemory* Memory = CastInstanceNodeMemory<FWxBeyondLeashMemory>(NodeMemory);
	Memory->bWasBeyond = CalculateRawConditionValue(OwnerComp, NodeMemory);
}

void UWxBTDecorator_BeyondLeash::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 실제 abort 여부·방향은 엔진이 FlowAbortMode 로 판단한다(Lower Priority 면 하위 전투만 중단).
	FWxBeyondLeashMemory* Memory = CastInstanceNodeMemory<FWxBeyondLeashMemory>(NodeMemory);
	const bool bBeyond = CalculateRawConditionValue(OwnerComp, NodeMemory);
	if (bBeyond != Memory->bWasBeyond)
	{
		Memory->bWasBeyond = bBeyond;
		OwnerComp.RequestExecution(this);
	}
}

uint16 UWxBTDecorator_BeyondLeash::GetInstanceMemorySize() const
{
	return sizeof(FWxBeyondLeashMemory);
}
