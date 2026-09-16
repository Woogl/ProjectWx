// Copyright Woogle. All Rights Reserved.

#include "WxBTService_MirrorMovement.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	bool HasActiveMirrorMovementAbility(AActor* Actor)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
		if (!ASC) { return false; }
		FScopedAbilityListLock Lock(*ASC);
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.IsActive()) { return true; }
		}
		return false;
	}
}

UWxBTService_MirrorMovement::UWxBTService_MirrorMovement()
{
	NodeName = TEXT("Mirror Movement");
	bCreateNodeInstance = true;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
	Interval = 0.f;
	RandomDeviation = 0.f;
	MirrorTarget.SelectedKeyName = TEXT("Master");
	MirrorTarget.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, MirrorTarget), AActor::StaticClass());
}

void UWxBTService_MirrorMovement::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (Asset.BlackboardAsset) { MirrorTarget.ResolveSelectedKey(*Asset.BlackboardAsset); }
}

FString UWxBTService_MirrorMovement::GetStaticServiceDescription() const
{
	return FString::Printf(TEXT("%s offset %s, arrival %.0f cm, teleport after %.2f s"),
		*MirrorTarget.SelectedKeyName.ToString(), *LocalOffset.ToString(), ArrivalRadius, TeleportDelay);
}

void UWxBTService_MirrorMovement::Release(UBehaviorTreeComponent& OwnerComp)
{
	if (MasterAbilitySystem.IsValid()) { MasterAbilitySystem->OnAbilityEnded.RemoveAll(this); }
	if (FollowerAbilitySystem.IsValid()) { FollowerAbilitySystem->OnAbilityEnded.RemoveAll(this); }
	MasterAbilitySystem.Reset();
	FollowerAbilitySystem.Reset();
	bPendingAbilityEndTeleport = false;
	if (ACharacter* Pawn = Follower.Get())
	{
		UCharacterMovementComponent* Movement = Pawn->GetCharacterMovement();
		Movement->RemoveTickPrerequisiteComponent(&OwnerComp);
		Movement->bOrientRotationToMovement = bOriginalOrientToMovement;
		Movement->bUseControllerDesiredRotation = bOriginalControllerDesiredRotation;
		Movement->MaxWalkSpeed = OriginalMaxWalkSpeed;
		Movement->MaxWalkSpeedCrouched = OriginalCrouchSpeed;
		Movement->GravityScale = OriginalGravity;
		Movement->JumpZVelocity = OriginalJumpVelocity;
		Pawn->JumpMaxCount = OriginalJumpMaxCount;
		Pawn->StopJumping();
		if (Master.IsValid()) { Pawn->GetCapsuleComponent()->IgnoreActorWhenMoving(Master.Get(), false); }
	}
	if (Master.IsValid()) { OwnerComp.RemoveTickPrerequisiteComponent(Master->GetCharacterMovement()); }
	Master.Reset();
	Follower.Reset();
	TravelTime = 0.f;
}

void UWxBTService_MirrorMovement::HandleAbilityEnded(const FAbilityEndedData& Data)
{
	// 콤보 재발동은 종료 통지 직후 다음 단계가 시작된다. 모든 발동이 끝났는지는 BT 틱에서 판단한다.
	bPendingAbilityEndTeleport = true;
}

void UWxBTService_MirrorMovement::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Release(OwnerComp);
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UWxBTService_MirrorMovement::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ACharacter* Pawn = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	ACharacter* Target = BB ? Cast<ACharacter>(BB->GetValueAsObject(MirrorTarget.SelectedKeyName)) : nullptr;
	if (!Pawn || !Pawn->HasAuthority() || !Target || Pawn == Target)
	{
		Release(OwnerComp);
		return;
	}
	UCharacterMovementComponent* Movement = Pawn->GetCharacterMovement();
	const UCharacterMovementComponent* SourceMovement = Target->GetCharacterMovement();
	if (Master != Target || Follower != Pawn)
	{
		Release(OwnerComp);
		Master = Target;
		Follower = Pawn;
		MasterAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		FollowerAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (MasterAbilitySystem.IsValid()) { MasterAbilitySystem->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded); }
		if (FollowerAbilitySystem.IsValid()) { FollowerAbilitySystem->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded); }
		PreviousJumpCount = Target->JumpCurrentCount;
		bOriginalOrientToMovement = Movement->bOrientRotationToMovement;
		bOriginalControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
		OriginalMaxWalkSpeed = Movement->MaxWalkSpeed;
		OriginalCrouchSpeed = Movement->MaxWalkSpeedCrouched;
		OriginalGravity = Movement->GravityScale;
		OriginalJumpVelocity = Movement->JumpZVelocity;
		OriginalJumpMaxCount = Pawn->JumpMaxCount;
		OwnerComp.AddTickPrerequisiteComponent(Target->GetCharacterMovement());
		Movement->AddTickPrerequisiteComponent(&OwnerComp);
		Pawn->GetCapsuleComponent()->IgnoreActorWhenMoving(Target, true);
		if (SourceMovement->IsFalling())
		{
			Movement->SetMovementMode(MOVE_Falling);
			Movement->Velocity = Target->GetVelocity();
		}
	}
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = false;
	Pawn->SetActorRotation(Target->GetActorRotation());
	Controller->SetControlRotation(Target->GetControlRotation());
	Movement->MaxWalkSpeed = SourceMovement->MaxWalkSpeed * 1.25f;
	Movement->MaxWalkSpeedCrouched = SourceMovement->MaxWalkSpeedCrouched * 1.25f;
	Movement->GravityScale = SourceMovement->GravityScale;
	Movement->JumpZVelocity = SourceMovement->JumpZVelocity;
	Pawn->JumpMaxCount = Target->JumpMaxCount;
	if (Target->IsCrouched()) { Pawn->Crouch(); } else { Pawn->UnCrouch(); }
	if (Target->JumpCurrentCount > PreviousJumpCount) { Pawn->StopJumping(); Pawn->Jump(); }
	else if (!Target->bPressedJump) { Pawn->StopJumping(); }
	PreviousJumpCount = Target->JumpCurrentCount;

	const FVector Destination = Target->GetActorLocation() + Target->GetActorRotation().RotateVector(LocalOffset);
	const bool bAbilityActive = HasActiveMirrorMovementAbility(Target) || HasActiveMirrorMovementAbility(Pawn);
	if (bPendingAbilityEndTeleport && !bAbilityActive && Pawn->TeleportTo(Destination, Target->GetActorRotation()))
	{
		bPendingAbilityEndTeleport = false;
		TravelTime = 0.f;
		Movement->StopMovementImmediately();
	}
	FVector Error = Destination - Pawn->GetActorLocation();
	Error.Z = 0.f;
	if (Error.SizeSquared() <= FMath::Square(ArrivalRadius)) { TravelTime = 0.f; Error = FVector::ZeroVector; }
	// Master를 함께 검사해 커밋과 분신의 다음 BT 틱 사이에도 강제 이동하지 않는다.
	else if (bAbilityActive) { TravelTime = 0.f; }
	else if (Movement->IsMovingOnGround() && SourceMovement->IsMovingOnGround())
	{
		TravelTime += DeltaSeconds;
		// 충돌이 있는 목표에는 무조건 겹쳐 넣지 않고 TeleportTo의 배치 검사를 따른다.
		if (TravelTime >= TeleportDelay && Pawn->TeleportTo(Destination, Target->GetActorRotation()))
		{
			TravelTime = 0.f;
			Movement->StopMovementImmediately();
			Error = FVector::ZeroVector;
		}
	}
	else { TravelTime = 0.f; }
	const FVector DesiredVelocity = FVector(Target->GetVelocity().X, Target->GetVelocity().Y, 0.f) + Error * 4.f;
	Pawn->AddMovementInput(DesiredVelocity.GetSafeNormal2D(), FMath::Min(DesiredVelocity.Size2D() / FMath::Max(Movement->GetMaxSpeed(), 1.f), 1.f));
}
