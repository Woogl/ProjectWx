// Copyright Woogle. All Rights Reserved.

#include "WxBTDecorator_BeyondLeash.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UWxBTDecorator_BeyondLeash::UWxBTDecorator_BeyondLeash()
{
	NodeName = TEXT("Beyond Leash");

	// TickNode/OnBecomeRelevant 오버라이드를 감지해 알림 플래그(bNotifyTick 등)를 자동 설정한다(엔진 데코 관용).
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();

	Anchor.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UWxBTDecorator_BeyondLeash, Anchor), AActor::StaticClass());
	Anchor.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UWxBTDecorator_BeyondLeash, Anchor));
	Anchor.SelectedKeyName = WxBlackboardKeys::HomeLocation;

	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
}

void UWxBTDecorator_BeyondLeash::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		Anchor.ResolveSelectedKey(*BlackboardAsset);
	}
	else
	{
		Anchor.InvalidateResolvedKey();
	}
}

uint16 UWxBTDecorator_BeyondLeash::GetInstanceMemorySize() const
{
	return sizeof(FWxBeyondLeashMemory);
}

FString UWxBTDecorator_BeyondLeash::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s로부터 %.0f m 이상 이탈 시 true"), *Anchor.SelectedKeyName.ToString(), LeashRadius / 100);
}

bool UWxBTDecorator_BeyondLeash::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (OwnerComp.IsExecutingBranch(this, GetChildIndex()))
	{
		return true;
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !Blackboard)
	{
		return false;
	}

	// 미설정 Vector 키는 FAISystem::InvalidLocation 이라 거리로 재면 무조건 이탈이 된다. 엔진 조회가 그 검사까지 해 주므로 실패를 그대로 "이탈 아님" 으로 쓴다.
	FVector AnchorLocation;
	if (!Blackboard->GetLocationFromEntry(Anchor.GetSelectedKeyID(), AnchorLocation))
	{
		return false;
	}

	const float DistSquared = FVector::DistSquared(Pawn->GetActorLocation(), AnchorLocation);
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
