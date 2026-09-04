// Copyright Woogle. All Rights Reserved.

#include "WxBTService_LockOn.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

UWxBTService_LockOn::UWxBTService_LockOn()
{
	NodeName = TEXT("Lock On");

	// TickNode/OnBecomeRelevant/OnCeaseRelevant 오버라이드를 감지해 알림 플래그를 자동 설정한다(엔진 서비스 관용).
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	// bCallTickOnSearchStart 는 쓰지 않는다 — 그 틱은 aux 노드 등록이 커밋되기 전에 돌아, 탐색이 폐기되면 걸어 둔 포커스·회전 모드를 되돌릴 OnCeaseRelevant 가 영영 오지 않는다.
	// 진입 시 적용은 OnBecomeRelevant 가 맡는다.
	Interval = 0.1f;
	RandomDeviation = 0.0f;
}

uint16 UWxBTService_LockOn::GetInstanceMemorySize() const
{
	return sizeof(FWxLockOnMemory);
}

void UWxBTService_LockOn::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FWxLockOnMemory>(NodeMemory, InitType);
}

void UWxBTService_LockOn::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FWxLockOnMemory>(NodeMemory, CleanupType);
}

FString UWxBTService_LockOn::GetStaticServiceDescription() const
{
	return FString::Printf(TEXT("%s, %s"), *WxBlackboardKeys::TargetActor.ToString(), *GetStaticTickIntervalDescription());
}

void UWxBTService_LockOn::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// 첫 틱을 기다리지 않고 브랜치 진입 즉시 대상을 바라본다.
	// 이 통지는 aux 노드 등록이 커밋된 뒤에만 오므로, 여기서 건 상태는 반드시 짝이 되는 OnCeaseRelevant 를 받는다.
	SyncLockOn(OwnerComp, NodeMemory);
}

void UWxBTService_LockOn::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	SyncLockOn(OwnerComp, NodeMemory);
}

void UWxBTService_LockOn::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// StopTree 도 활성 aux 노드에 이 통지를 돌리므로, 사망·빙의 해제·컨트롤러 파괴가 모두 여기로 모인다.
	FWxLockOnMemory* Memory = CastInstanceNodeMemory<FWxLockOnMemory>(NodeMemory);
	ReleaseLockOn(OwnerComp.GetAIOwner(), *Memory);
}

void UWxBTService_LockOn::SyncLockOn(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!AIController || !Blackboard)
	{
		return;
	}

	FWxLockOnMemory* Memory = CastInstanceNodeMemory<FWxLockOnMemory>(NodeMemory);

	AActor* Target = WxBlackboardKeys::GetTargetActor(Blackboard);
	APawn* Pawn = AIController->GetPawn();
	if (!Target || !Pawn)
	{
		ReleaseLockOn(AIController, *Memory);
		return;
	}

	// 걸어 둔 그대로면 손대지 않는다 — 매 틱 같은 값을 다시 쓰면 그 사이 다른 곳이 바꾼 회전 모드를 덮어쓴다.
	if (Memory->LockedOnPawn.Get() == Pawn && AIController->GetFocusActorForPriority(EAIFocusPriority::Gameplay) == Target)
	{
		return;
	}

	// 폰이 바뀔 때만 이전 폰을 되돌린다.
	// 대상만 갈리는 재타겟은 SetFocus 가 같은 우선순위를 먼저 비우므로, 회전 모드를 아키타입 값으로 왕복시킬 이유가 없다.
	if (Memory->LockedOnPawn.Get() != Pawn)
	{
		ReleaseLockOn(AIController, *Memory);
	}

	ApplyLockOn(*AIController, *Pawn, *Target, *Memory);
}

void UWxBTService_LockOn::ApplyLockOn(AAIController& AIController, APawn& Pawn, AActor& Target, FWxLockOnMemory& Memory) const
{
	// 이동 중에는 PathFollowing 이 Move 우선순위로 진행 방향을 응시시키므로, 그보다 높은 Gameplay 로 걸어야 전투 대상을 계속 본다.
	AIController.SetFocus(&Target, EAIFocusPriority::Gameplay);

	// 폰을 기록해 두는 이유: 빙의 해제는 컨트롤러의 폰 참조를 끊은 뒤에야 BT 를 멈추므로, 그 경로의 해제 시점엔 GetPawn() 이 이미 비어 있다.
	Memory.LockedOnPawn = &Pawn;

	const ACharacter* Character = Cast<ACharacter>(&Pawn);
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = true;
}

void UWxBTService_LockOn::ReleaseLockOn(AAIController* AIController, FWxLockOnMemory& Memory) const
{
	if (Memory.LockedOnPawn.IsExplicitlyNull())
	{
		return;
	}

	if (AIController)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// 폰이 파괴된 뒤라면 되돌릴 대상이 없다. 컨트롤러가 이미 사라진 경로에서도 폰만 살아 있으면 여기까지 와서 원복한다.
	APawn* Pawn = Memory.LockedOnPawn.Get();
	Memory.LockedOnPawn.Reset();

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
