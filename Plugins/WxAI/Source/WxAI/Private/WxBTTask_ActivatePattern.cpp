// Copyright Woogle. All Rights Reserved.

#include "WxBTTask_ActivatePattern.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UWxBTTask_ActivatePattern::UWxBTTask_ActivatePattern()
{
	NodeName = TEXT("Activate Pattern");

	// 이동/발동 상태를 폰별로 보관하기 위해 노드를 인스턴싱한다.
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UWxBTTask_ActivatePattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	bMovePhaseActive = false;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	CachedOwnerComp = &OwnerComp;
	CachedAIController = AIController;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* Target = Blackboard ? WxBlackboardKeys::GetTargetActor(Blackboard) : nullptr;

	// 타겟이 없거나 접근 거리가 유효하지 않으면 이동을 생략하고 제자리에서 발동한다.
	if (!Target || ApproachDistance <= 0.f)
	{
		return BeginActivateAbility(Pawn);
	}

	AIController->ReceiveMoveCompleted.AddDynamic(this, &UWxBTTask_ActivatePattern::HandleMoveCompleted);

	// MoveId 로 본 Task 의 이동 완료만 걸러내기 위해, 코드+MoveId 를 함께 돌려주는 MoveTo(FAIMoveRequest) 를 쓴다.
	FAIMoveRequest MoveRequest(Target);
	MoveRequest.SetAcceptanceRadius(ApproachDistance);

	const FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveRequest);
	switch (MoveResult.Code)
	{
	case EPathFollowingRequestResult::AlreadyAtGoal:
		// 이미 사거리 안이면 이동 콜백이 오지 않으므로 바로 발동한다.
		AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UWxBTTask_ActivatePattern::HandleMoveCompleted);
		return BeginActivateAbility(Pawn);

	case EPathFollowingRequestResult::RequestSuccessful:
	{
		// MoveTo 가 같은 콜스택에서 완료 콜백을 이미 브로드캐스트했다면(즉시 완료),
		// bMovePhaseActive 세팅 전이라 HandleMoveCompleted 가 콜백을 버려 Task 가 InProgress 로 영구 정지한다.
		// 그 경우 PathFollowing 상태가 이미 Idle 이므로 여기서 직접 발동한다.
		const UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent();
		if (PathComp && PathComp->GetStatus() == EPathFollowingStatus::Idle)
		{
			AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UWxBTTask_ActivatePattern::HandleMoveCompleted);
			return BeginActivateAbility(Pawn);
		}

		MoveRequestID = MoveResult.MoveId;
		bMovePhaseActive = true;
		return EBTNodeResult::InProgress;
	}

	default:
		// 경로 실패 등으로 이동을 시작조차 못하면 실패로 마감한다.
		AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UWxBTTask_ActivatePattern::HandleMoveCompleted);
		return EBTNodeResult::Failed;
	}
}

FString UWxBTTask_ActivatePattern::GetStaticDescription() const
{
	return FString::Printf(TEXT("AbilityTag: %s\nApproach: %.0f"), *AbilityTag.ToString(), ApproachDistance);
}

EBTNodeResult::Type UWxBTTask_ActivatePattern::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 이동 페이즈 중단: 이동 콜백을 끊고 이동을 멈춘다.
	if (bMovePhaseActive)
	{
		bMovePhaseActive = false;
		if (AAIController* AIController = CachedAIController.Get())
		{
			AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UWxBTTask_ActivatePattern::HandleMoveCompleted);
			AIController->StopMovement();
		}
		MoveRequestID = FAIRequestID::InvalidRequest;
		return Super::AbortTask(OwnerComp, NodeMemory);
	}

	// 발동 페이즈 중단: CleanUp 이 ActivatedHandle 을 리셋하므로 취소할 핸들을 먼저 캡처한다.
	const FGameplayAbilitySpecHandle HandleToCancel = ActivatedHandle;

	// 델리게이트를 먼저 해제하여 CancelAbilityHandle 이 트리거하는 OnAbilityEnded 콜백이 FinishLatentTask 를 호출하지 않도록 한다.
	CleanUp();

	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->CancelAbilityHandle(HandleToCancel);
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

EBTNodeResult::Type UWxBTTask_ActivatePattern::BeginActivateAbility(APawn* Pawn)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC || !AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// 동일 태그 어빌리티가 여러 개일 수 있으므로, 발동에 성공하는 첫 후보를 채택한다.
	FGameplayAbilitySpecHandle ActivatedSpecHandle;
	for (const FGameplayAbilitySpec& IterSpec : ASC->GetActivatableAbilities())
	{
		if (IterSpec.Ability && IterSpec.Ability->GetAssetTags().HasTag(AbilityTag))
		{
			// 핸들은 값 타입이라 TryActivateAbility 가 배열을 재할당해도 안전하다. 호출 전에 캡처한다.
			const FGameplayAbilitySpecHandle CandidateHandle = IterSpec.Handle;
			if (ASC->TryActivateAbility(CandidateHandle))
			{
				// 성공 시 즉시 break — 활성화로 배열이 재할당됐어도 이후 IterSpec 을 건드리지 않는다.
				ActivatedSpecHandle = CandidateHandle;
				break;
			}
			// 실패(CanActivate 실패)는 ActivatableAbilities 를 바꾸지 않으므로 다음 후보로 계속 진행해도 안전하다.
		}
	}

	if (!ActivatedSpecHandle.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// TryActivateAbility 는 활성화 도중 어빌리티 부여/제거로 ActivatableAbilities 배열을 재할당할 수 있어,
	// 활성화 이전에 잡아둔 Spec 포인터는 무효가 될 수 있다. 반드시 핸들로 다시 조회한다.
	// 또한 TryActivateAbility 내부에서 어빌리티가 동기적으로 종료될 수 있다 (CommitAbility 실패 등).
	// 이 경우 OnAbilityEnded가 이미 브로드캐스트된 후이므로, 델리게이트를 등록해도 콜백이 발생하지 않아 BT가 InProgress 상태로 영구 정지한다.
	const FGameplayAbilitySpec* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActivatedSpecHandle);
	if (!ActiveSpec || !ActiveSpec->IsActive())
	{
		return EBTNodeResult::Failed;
	}

	CachedASC = ASC;
	ActivatedHandle = ActivatedSpecHandle;

	AbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(
		this, &UWxBTTask_ActivatePattern::HandleAbilityEnded);

	return EBTNodeResult::InProgress;
}

void UWxBTTask_ActivatePattern::HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// 본 Task 가 요청한 이동만 처리한다(컨트롤러의 다른 이동 완료 통지 무시).
	if (!bMovePhaseActive || RequestID != MoveRequestID)
	{
		return;
	}
	bMovePhaseActive = false;

	AAIController* AIController = CachedAIController.Get();
	if (AIController)
	{
		AIController->ReceiveMoveCompleted.RemoveDynamic(this, &UWxBTTask_ActivatePattern::HandleMoveCompleted);
	}

	UBehaviorTreeComponent* BTComp = CachedOwnerComp.Get();
	if (!BTComp)
	{
		return;
	}

	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (Result != EPathFollowingResult::Success || !Pawn)
	{
		FinishLatentTask(*BTComp, EBTNodeResult::Failed);
		return;
	}

	// 접근 완료 → 발동. 발동이 InProgress 가 아니면(즉시 실패/성공) 여기서 마감하고, InProgress 면 OnAbilityEnded 가 마감한다.
	const EBTNodeResult::Type ActivateResult = BeginActivateAbility(Pawn);
	if (ActivateResult != EBTNodeResult::InProgress)
	{
		FinishLatentTask(*BTComp, ActivateResult);
	}
}

void UWxBTTask_ActivatePattern::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (AbilityEndedData.AbilitySpecHandle != ActivatedHandle)
	{
		return;
	}

	CleanUp();

	UBehaviorTreeComponent* BTComp = CachedOwnerComp.Get();
	if (!BTComp)
	{
		return;
	}

	const EBTNodeResult::Type Result = AbilityEndedData.bWasCancelled
		? EBTNodeResult::Failed
		: EBTNodeResult::Succeeded;
	FinishLatentTask(*BTComp, Result);
}

void UWxBTTask_ActivatePattern::CleanUp()
{
	if (UAbilitySystemComponent* ASC = CachedASC.Get())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedDelegateHandle);
	}
	AbilityEndedDelegateHandle.Reset();
	ActivatedHandle = FGameplayAbilitySpecHandle();
}
