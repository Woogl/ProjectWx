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

	// 에디터에서 별도 선택 없이 동작하도록 기본 키를 채워 둔다. 실제 키 해석은 InitializeFromAsset 가 한다.
	BlackboardKey.SelectedKeyName = WxBlackboardKeys::PatrolTargetLocation;

	// 도착 후 커서를 진행시키기 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;

	// 정찰 커서·감속 GE 핸들을 폰별로 보관하기 위해 노드를 인스턴싱한다.
	bCreateNodeInstance = true;
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

	// Once 로 경로를 마쳤으면 마지막 지점에 그대로 정지한다.
	// Failed 를 반환하면 하위 폴백 분기가 폰을 집/배회로 끌고 가므로, 이동 없이 Succeeded 로 정찰 분기를 점유해 그 자리에 머물게 한다.
	if (bPatrolFinished)
	{
		return EBTNodeResult::Succeeded;
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
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (const UWxPatrolComponent* Patrol = UWxPatrolComponent::FindPatrolComponent(Pawn))
	{
		int32 NextIndex = PatrolCursor;
		if (Patrol->GetNextIndex(PatrolCursor, PatrolDirection, NextIndex))
		{
			PatrolCursor = NextIndex;
		}
		else
		{
			// Once 로 경로 끝에 도달: 이후 ExecuteTask 가 마지막 지점에 정지(Succeeded)하도록 표시한다.
			bPatrolFinished = true;
		}
	}
}
