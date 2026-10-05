// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_LockOn.h"

#include "AI/WxAIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_LockOn::FWxStateTreeTask_LockOn()
{
	// 자식 상태를 다시 고르느라 이 상태가 재선택될 때마다 풀었다 다시 걸 이유가 없다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_LockOn::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 첫 틱을 기다리지 않고 상태 진입 즉시 대상을 바라본다.
	SyncLockOn(Context);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_LockOn::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	SyncLockOn(Context);

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_LockOn::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 트리 정지도 활성 태스크에 이 통지를 돌리므로, 사망·빙의 해제·컨트롤러 파괴가 모두 여기로 모인다.
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	ReleaseLockOn(Cast<AAIController>(Context.GetOwner()), Instance);
}

void FWxStateTreeTask_LockOn::SyncLockOn(FStateTreeExecutionContext& Context) const
{
	AWxAIController* AIController = Cast<AWxAIController>(Context.GetOwner());
	if (!AIController)
	{
		return;
	}

	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AActor* Target = AIController->GetTargetActor();
	APawn* Pawn = AIController->GetPawn();
	if (!Target || !Pawn)
	{
		ReleaseLockOn(AIController, Instance);
		return;
	}

	// 걸어 둔 그대로면 손대지 않는다 — 매 틱 같은 값을 다시 쓰면 그 사이 다른 곳이 바꾼 회전 모드를 덮어쓴다.
	if (Instance.LockedOnPawn.Get() == Pawn && AIController->GetFocusActorForPriority(EAIFocusPriority::Gameplay) == Target)
	{
		return;
	}

	// 폰이 바뀔 때만 이전 폰을 되돌린다.
	// 대상만 갈리는 재타겟은 SetFocus 가 같은 우선순위를 먼저 비우므로, 회전 모드를 아키타입 값으로 왕복시킬 이유가 없다.
	if (Instance.LockedOnPawn.Get() != Pawn)
	{
		ReleaseLockOn(AIController, Instance);
	}

	ApplyLockOn(*AIController, *Pawn, *Target, Instance);
}

void FWxStateTreeTask_LockOn::ApplyLockOn(AAIController& AIController, APawn& Pawn, AActor& Target, FInstanceDataType& Instance) const
{
	// 이동 중에는 PathFollowing 이 Move 우선순위로 진행 방향을 응시시키므로, 그보다 높은 Gameplay 로 걸어야 전투 대상을 계속 본다.
	AIController.SetFocus(&Target, EAIFocusPriority::Gameplay);

	// 폰을 기록해 두는 이유: 해제는 컨트롤러의 폰이 비었거나 다른 폰으로 바뀐 뒤에도 불리므로, 그 시점엔 GetPawn() 으로 이전 폰을 찾을 수 없다.
	Instance.LockedOnPawn = &Pawn;

	const ACharacter* Character = Cast<ACharacter>(&Pawn);
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = true;
}

void FWxStateTreeTask_LockOn::ReleaseLockOn(AAIController* AIController, FInstanceDataType& Instance) const
{
	if (Instance.LockedOnPawn.IsExplicitlyNull())
	{
		return;
	}

	if (AIController)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	APawn* Pawn = Instance.LockedOnPawn.Get();
	Instance.LockedOnPawn.Reset();

	const ACharacter* Character = Cast<ACharacter>(Pawn);
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	// 평상시 회전 모드는 폰마다 다를 수 있으므로, 상수 대신 컴포넌트 아키타입(폰 BP·C++ 생성자 기본값)에서 읽어 되돌린다.
	if (const UCharacterMovementComponent* MovementDefaults = Cast<UCharacterMovementComponent>(Movement->GetArchetype()))
	{
		Movement->bUseControllerDesiredRotation = MovementDefaults->bUseControllerDesiredRotation;
		Movement->bOrientRotationToMovement = MovementDefaults->bOrientRotationToMovement;
	}
}
