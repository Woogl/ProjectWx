// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotifyState_RotateToTarget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"

void UWxAnimNotifyState_RotateToTarget::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || !TargetingPreset)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(Owner->GetWorld());
	if (!TargetingSubsystem)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = Owner;
	SourceContext.InstigatorActor = Owner;

	FTargetingRequestHandle Handle = TargetingSubsystem->MakeTargetRequestHandle(TargetingPreset, SourceContext);
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(Handle);

	TArray<AActor*> Targets;
	TargetingSubsystem->GetTargetingResultsActors(Handle, Targets);
	TargetingSubsystem->ReleaseTargetRequestHandle(Handle);

	if (Targets.Num() > 0)
	{
		OwnerToTargetMap.Add(Owner, Targets[0]);
	}
	else
	{
		OwnerToTargetMap.Remove(Owner);
	}
}

void UWxAnimNotifyState_RotateToTarget::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	TWeakObjectPtr<AActor>* FoundTarget = OwnerToTargetMap.Find(Owner);
	if (!FoundTarget || !FoundTarget->IsValid())
	{
		return;
	}

	AActor* Target = FoundTarget->Get();

	FVector Direction = Target->GetActorLocation() - Owner->GetActorLocation();
	Direction.Z = 0.0;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation = Direction.Rotation();
	const FRotator CurrentRotation = Owner->GetActorRotation();

	if (FMath::Abs(FRotator::NormalizeAxis(CurrentRotation.Yaw - DesiredRotation.Yaw)) <= RotationTolerance)
	{
		return;
	}

	float RotationRateYaw = 360.f;
	if (const ACharacter* Character = Cast<ACharacter>(Owner))
	{
		RotationRateYaw = Character->GetCharacterMovement()->RotationRate.Yaw;
	}

	const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, DesiredRotation, FrameDeltaTime, RotationRateYaw);
	Owner->SetActorRotation(NewRotation);
}

void UWxAnimNotifyState_RotateToTarget::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		OwnerToTargetMap.Remove(MeshComp->GetOwner());
	}
}

FString UWxAnimNotifyState_RotateToTarget::GetNotifyName_Implementation() const
{
	return TEXT("Rotate To Target");
}
