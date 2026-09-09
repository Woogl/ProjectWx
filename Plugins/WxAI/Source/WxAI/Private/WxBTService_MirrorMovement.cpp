// Copyright Woogle. All Rights Reserved.

#include "WxBTService_MirrorMovement.h"

#include "WxBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

UWxBTService_MirrorMovement::UWxBTService_MirrorMovement()
{
	NodeName = TEXT("Mirror Movement");

	// TickNode/OnCeaseRelevant 오버라이드를 감지해 알림 플래그를 자동 설정한다(엔진 서비스 관용).
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	// 이동 입력은 매 프레임 넣어야 한다. 주기를 두면 그 사이 프레임에 입력이 비어 가속이 끊긴다.
	Interval = 0.f;
	RandomDeviation = 0.f;

	// 진입 즉시 적용은 필요 없다 — 첫 틱이 표본 하나를 기록해야 재생할 것이 생긴다.

	MirrorTarget.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UWxBTService_MirrorMovement, MirrorTarget), AActor::StaticClass());
	MirrorTarget.SelectedKeyName = WxBlackboardKeys::Master;
}

void UWxBTService_MirrorMovement::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		MirrorTarget.ResolveSelectedKey(*BlackboardAsset);
	}
	else
	{
		MirrorTarget.InvalidateResolvedKey();
	}
}

uint16 UWxBTService_MirrorMovement::GetInstanceMemorySize() const
{
	return sizeof(FWxMirrorMovementMemory);
}

void UWxBTService_MirrorMovement::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FWxMirrorMovementMemory>(NodeMemory, InitType);
}

void UWxBTService_MirrorMovement::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FWxMirrorMovementMemory>(NodeMemory, CleanupType);
}

FString UWxBTService_MirrorMovement::GetStaticServiceDescription() const
{
	return FString::Printf(TEXT("%s 를 %.2f 초 뒤에 따라간다\nOffset: %s\n%s"),
		*MirrorTarget.SelectedKeyName.ToString(),
		Delay,
		*LocalOffset.ToCompactString(),
		*GetStaticTickIntervalDescription());
}

void UWxBTService_MirrorMovement::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	const FWxMirrorMovementMemory* Memory = CastInstanceNodeMemory<FWxMirrorMovementMemory>(NodeMemory);
	Values.Add(FString::Printf(TEXT("표본: %d"), Memory->Trail.Num()));
}

void UWxBTService_MirrorMovement::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FWxMirrorMovementMemory* Memory = CastInstanceNodeMemory<FWxMirrorMovementMemory>(NodeMemory);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	const AActor* Target = FindMirrorTarget(OwnerComp);
	const UWorld* World = OwnerComp.GetWorld();
	if (!Pawn || !Target || !World)
	{
		// 대상이 사라지면 그 자리에 선다. 자취를 남겨 두면 대상이 돌아왔을 때 낡은 자리로 달려간다.
		Memory->Trail.Reset();
		ReleaseFacing(AIController, *Memory);
		return;
	}

	const double Now = World->GetTimeSeconds();
	RecordSample(*Target, Now, *Memory);

	// 재생 시점을 지난 표본은 버린다. 한 틱에 여러 개가 버려져도 그 사이의 점프는 수거해 넘긴다.
	const double ReplayTime = Now - Delay;
	bool bShouldJump = false;
	while (Memory->Trail.Num() > 1 && Memory->Trail[1].Time <= ReplayTime)
	{
		bShouldJump |= Memory->Trail[0].bJumped;
		Memory->Trail.RemoveAt(0, EAllowShrinking::No);
	}

	// 맨 앞 표본은 여러 틱 동안 재생되므로, 점프를 한 번만 내보내려면 여기서 지운다.
	FWxTrailSample& Sample = Memory->Trail[0];
	bShouldJump |= Sample.bJumped;
	Sample.bJumped = false;

	MirrorSample(*Pawn, Sample, bShouldJump);

	if (bMirrorFacing)
	{
		ApplyFacing(*AIController, *Pawn, Sample.Yaw, *Memory);
	}
}

void UWxBTService_MirrorMovement::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// StopTree 도 활성 aux 노드에 이 통지를 돌리므로, 사망·빙의 해제·컨트롤러 파괴가 모두 여기로 모인다.
	FWxMirrorMovementMemory* Memory = CastInstanceNodeMemory<FWxMirrorMovementMemory>(NodeMemory);
	Memory->Trail.Reset();
	ReleaseFacing(OwnerComp.GetAIOwner(), *Memory);
}

AActor* UWxBTService_MirrorMovement::FindMirrorTarget(const UBehaviorTreeComponent& OwnerComp) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return nullptr;
	}

	return Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(MirrorTarget.GetSelectedKeyID()));
}

void UWxBTService_MirrorMovement::RecordSample(const AActor& Target, double Now, FWxMirrorMovementMemory& Memory) const
{
	const ACharacter* TargetCharacter = Cast<ACharacter>(&Target);
	const UCharacterMovementComponent* TargetMovement = TargetCharacter ? TargetCharacter->GetCharacterMovement() : nullptr;
	const FVector Velocity = Target.GetVelocity();

	FWxTrailSample Sample;
	Sample.Time = Now;
	Sample.Location = Target.GetActorLocation();
	Sample.Yaw = Target.GetActorRotation().Yaw;
	Sample.Speed = Velocity.Size2D();
	Sample.bCrouched = TargetCharacter && TargetCharacter->bIsCrouched;
	Sample.bAirborne = TargetMovement && TargetMovement->IsFalling();

	// 낭떠러지에서 걸어 떨어진 것은 점프가 아니다. 지상에서 뜨는 순간에 상승 중이어야 스스로 뛴 것으로 본다.
	Sample.bJumped = Sample.bAirborne && Velocity.Z > 0.f && (Memory.Trail.IsEmpty() || !Memory.Trail.Last().bAirborne);

	Memory.Trail.Add(Sample);
}

void UWxBTService_MirrorMovement::MirrorSample(APawn& Pawn, const FWxTrailSample& Sample, bool bShouldJump) const
{
	ACharacter* Character = Cast<ACharacter>(&Pawn);
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	// 매 틱 같은 값을 다시 넣어도 앉기 의사만 세울 뿐이라, 표본을 그대로 따라 여닫는다.
	if (Sample.bCrouched)
	{
		Character->Crouch();
	}
	else
	{
		Character->UnCrouch();
	}

	if (bShouldJump && !Movement->IsFalling())
	{
		Character->Jump();
	}

	FVector ToTarget = Sample.Location + FRotator(0.f, Sample.Yaw, 0.f).RotateVector(LocalOffset) - Pawn.GetActorLocation();
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();
	if (Distance <= ArrivalRadius)
	{
		return;
	}

	// 기록된 속력을 그대로 내는 입력 스케일은 상한 대비 비율이다. 폰의 상한이 대상보다 낮으면 그만큼 뒤처진다.
	const float MaxSpeed = Movement->GetMaxSpeed();
	const float Scale = (Distance > CatchUpDistance || MaxSpeed <= 0.f)
		? 1.f
		: FMath::Clamp(Sample.Speed / MaxSpeed, 0.f, 1.f);

	Pawn.AddMovementInput(ToTarget / Distance, Scale);
}

void UWxBTService_MirrorMovement::ApplyFacing(AAIController& AIController, APawn& Pawn, float Yaw, FWxMirrorMovementMemory& Memory) const
{
	// 폰 교체 처리가 포커스보다 먼저다. 뒤에 두면 ReleaseFacing 의 ClearFocus 가 방금 세운 포커스를 지운다.
	if (Memory.FacingPawn.Get() != &Pawn)
	{
		ReleaseFacing(&AIController, Memory);

		// 폰을 기록해 두는 이유: 빙의 해제는 컨트롤러의 폰 참조를 끊은 뒤에야 BT 를 멈추므로, 그 경로의 해제 시점엔 GetPawn() 이 이미 비어 있다.
		Memory.FacingPawn = &Pawn;

		const ACharacter* Character = Cast<ACharacter>(&Pawn);
		if (UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
		{
			Movement->bOrientRotationToMovement = false;
			Movement->bUseControllerDesiredRotation = true;
		}
	}

	// 이동 중에는 PathFollowing 이 Move 우선순위로 진행 방향을 응시시키므로, 그보다 높은 Gameplay 로 걸어야 대상의 시선이 이긴다.
	// 폰 자신을 기준으로 밀어야 거리와 무관하게 표본의 Yaw 가 그대로 나온다.
	AIController.SetFocalPoint(Pawn.GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * 10000.f, EAIFocusPriority::Gameplay);
}

void UWxBTService_MirrorMovement::ReleaseFacing(AAIController* AIController, FWxMirrorMovementMemory& Memory) const
{
	if (Memory.FacingPawn.IsExplicitlyNull())
	{
		return;
	}

	if (AIController)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// 폰이 파괴된 뒤라면 되돌릴 대상이 없다. 컨트롤러가 이미 사라진 경로에서도 폰만 살아 있으면 여기까지 와서 원복한다.
	APawn* Pawn = Memory.FacingPawn.Get();
	Memory.FacingPawn.Reset();

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
