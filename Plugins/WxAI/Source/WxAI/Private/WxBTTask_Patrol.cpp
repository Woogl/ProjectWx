// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_Patrol.h"

#include "WxPatrolComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "WxBlackboardKeys.h"
#include "WxGameplayTags.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UWxBTTask_Patrol::UWxBTTask_Patrol()
{
	NodeName = TEXT("Patrol");

	// 도착 후 커서를 진행시키기 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;

	bCreateNodeInstance = true;
}

void UWxBTTask_Patrol::InitializeFromAsset(UBehaviorTree& Asset)
{
	// Super 가 이 이름으로 이동 목표 키를 해석하므로, 에셋에 다른 값이 남아 있어도 여기서 덮어써 고정한다.
	BlackboardKey.SelectedKeyName = WxBlackboardKeys::PatrolTargetLocation;

	Super::InitializeFromAsset(Asset);
}

EBTNodeResult::Type UWxBTTask_Patrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;

	// 정찰 경로가 없는 적(정찰 안 함)은 실패시켜 Selector 가 다음 행동(배회 등)으로 넘어가게 한다.
	const UWxPatrolComponent* Patrol = UWxPatrolComponent::FindPatrolComponent(Pawn);
	if (!Patrol || Patrol->GetNumPoints() == 0)
	{
		return EBTNodeResult::Failed;
	}

	// 갈 지점이 더 없으면 움직이지 않는다(Once 는 전투 뒤 재진입에서도 마지막 지점이 아니라 지금 서 있는 자리다).
	// Failed 를 반환하면 하위 폴백 분기가 폰을 집/배회로 끌고 가고, 즉시 Succeeded 는 브랜치를 놓아 주어 상위가 되감기며 재탐색을 되풀이한다.
	if (bPatrolFinished)
	{
		return EBTNodeResult::InProgress;
	}

	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		WxBlackboardKeys::SetPatrolTargetLocation(Blackboard, Patrol->GetPointLocation(PatrolCursor));
	}

	// MaxWalkSpeed 를 직접 쓰면 SPD 어트리뷰트 콜백과 주인이 겹쳐, 정찰 중 버프가 걸리거나 정찰이 끝날 때 서로의 값을 덮어쓴다.
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (ASC && MoveSpeedEffect)
	{
		const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(MoveSpeedEffect, 1.f, ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_MoveSpeedScale, MoveSpeedMultiplier);
			MoveSpeedEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

FString UWxBTTask_Patrol::GetStaticDescription() const
{
	const FString SpeedLine = MoveSpeedEffect ? FString::Printf(TEXT("Speed: x%.2f"), MoveSpeedMultiplier) : FString(TEXT("Speed: 감속 GE 미지정"));

	return FString::Printf(TEXT("%s\n%s\n"), *Super::GetStaticDescription(), *SpeedLine);
}

void UWxBTTask_Patrol::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	// 이동 중인 Patrol 과 완주해 눌러앉은 Patrol 은 디버거에서 똑같이 Running 으로만 보인다.
	if (bPatrolFinished)
	{
		Values.Add(TEXT("정찰 완주 — 제자리 대기"));
		return;
	}

	Values.Add(FString::Printf(TEXT("Cursor: %d"), PatrolCursor));
}

void UWxBTTask_Patrol::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	// 도착·중단·실패 등 어떤 종료 경로에서도 호출되므로, 감속 GE 제거는 여기서 한다.
	if (UAbilitySystemComponent* ASC = MoveSpeedEffectHandle.GetOwningAbilitySystemComponent())
	{
		ASC->RemoveActiveGameplayEffect(MoveSpeedEffectHandle);
	}
	MoveSpeedEffectHandle = FActiveGameplayEffectHandle();

	const AAIController* AIController = OwnerComp.GetAIOwner();
	const UWxPatrolComponent* Patrol = UWxPatrolComponent::FindPatrolComponent(AIController ? AIController->GetPawn() : nullptr);
	if (!Patrol)
	{
		return;
	}

	// 중단·실패 시엔 커서를 보존해 재개 시 이어서 정찰한다.
	// Once 가 아니면 정찰 종료도 함께 되돌린다 — 그래야 전투로 밀려났던 폰이 다시 정찰 지점으로 돌아온다(지점이 하나뿐인 경로가 여기 걸린다).
	if (TaskResult != EBTNodeResult::Succeeded)
	{
		if (Patrol->GetMoveMode() != EWxPatrolMoveMode::Once)
		{
			bPatrolFinished = false;
		}
		return;
	}

	int32 NextIndex = PatrolCursor;
	if (Patrol->GetNextIndex(PatrolCursor, PatrolDirection, NextIndex))
	{
		PatrolCursor = NextIndex;
	}
	else
	{
		bPatrolFinished = true;
	}
}
