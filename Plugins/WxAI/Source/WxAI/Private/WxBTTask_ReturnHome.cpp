// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ReturnHome.h"

#include "WxAIPerceptionComponent.h"
#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWxBTTask_ReturnHome::UWxBTTask_ReturnHome()
{
	NodeName = TEXT("Return Home");

	// 기본 이동 목표 키를 HomeLocation 으로 지정한다(에디터에서 별도 선택 없이 동작). 실제 키 해석은 InitializeFromAsset 가 한다.
	BlackboardKey.SelectedKeyName = WxBlackboardKeys::HomeLocation;

	// 종료 시 억제 해제·속도 복원을 위해 종료 콜백을 받는다.
	bNotifyTaskFinished = true;

	// 이동 속도 캐시를 폰별로 보관하기 위해 노드를 인스턴싱한다.
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UWxBTTask_ReturnHome::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	// 퍼셉션에 억제를 지시해 타겟/인식/회전을 원복하고 복귀 중 재감지를 막는다. 상태 변경은 퍼셉션이 단일 지점에서 수행한다.
	if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
	{
		Perception->SetTargetingSuppressed(true);
	}

	// 복귀 이동 동안만 폰의 최대 이동 속도를 배율만큼 낮춘다. 원래 값은 OnTaskFinished 에서 복원한다.
	CachedMaxWalkSpeed = 0.f;
	if (const ACharacter* Character = Cast<ACharacter>(AIController->GetPawn()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			CachedMaxWalkSpeed = Movement->MaxWalkSpeed;
			Movement->MaxWalkSpeed *= MoveSpeedMultiplier;
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

void UWxBTTask_ReturnHome::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

	AAIController* AIController = OwnerComp.GetAIOwner();

	// 이동 동안 낮췄던 최대 이동 속도를 복원한다. 도착·중단·실패 등 어떤 종료 경로에서도 호출된다.
	if (CachedMaxWalkSpeed > 0.f)
	{
		if (const ACharacter* Character = AIController ? Cast<ACharacter>(AIController->GetPawn()) : nullptr)
		{
			if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = CachedMaxWalkSpeed;
			}
		}
		CachedMaxWalkSpeed = 0.f;
	}

	// 복귀가 끝났으니 억제를 해제한다. 리시 안으로 돌아왔으면 다음 자극에서 다시 정상 감지·전투한다.
	if (AIController)
	{
		if (UWxAIPerceptionComponent* Perception = Cast<UWxAIPerceptionComponent>(AIController->GetPerceptionComponent()))
		{
			Perception->SetTargetingSuppressed(false);
		}
	}
}

FString UWxBTTask_ReturnHome::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s\n타겟 해제 후 Home 으로 복귀\nSpeed: x%.2f"), *Super::GetStaticDescription(), MoveSpeedMultiplier);
}
