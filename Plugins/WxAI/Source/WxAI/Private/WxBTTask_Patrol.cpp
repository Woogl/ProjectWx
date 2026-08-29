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
	// ExecuteTask 가 쓰는 키와 Super 가 읽는 키는 반드시 같아야 한다.
	// Super 가 이 이름으로 키를 해석하므로, 에셋에 다른 값이 남아 있어도 여기서 덮어써 고정한다.
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

	// Once 로 경로를 마쳤으면 더 이상 움직이지 않는다(전투 뒤 재진입이면 마지막 지점이 아니라 지금 서 있는 자리다).
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
	if (!MoveSpeedEffect)
	{
		return FString::Printf(TEXT("%s\nSpeed: 감속 GE 미지정"), *Super::GetStaticDescription());
	}

	return FString::Printf(TEXT("%s\nSpeed: x%.2f"), *Super::GetStaticDescription(), MoveSpeedMultiplier);
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

	// 중단·실패 시엔 커서를 보존해 재개 시 이어서 정찰한다.
	if (TaskResult != EBTNodeResult::Succeeded)
	{
		return;
	}

	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (const UWxPatrolComponent* Patrol = UWxPatrolComponent::FindPatrolComponent(AIController ? AIController->GetPawn() : nullptr))
	{
		int32 NextIndex = PatrolCursor;
		if (Patrol->GetNextIndex(PatrolCursor, PatrolDirection, NextIndex))
		{
			PatrolCursor = NextIndex;
		}
		else
		{
			// 다음 지점이 없다는 건 Once 로 경로 끝에 도달했다는 뜻이다.
			bPatrolFinished = true;
		}
	}
}
