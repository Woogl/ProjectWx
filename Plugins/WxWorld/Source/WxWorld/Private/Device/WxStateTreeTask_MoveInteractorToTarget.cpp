// Copyright Woogle. All Rights Reserved.

#include "Device/WxStateTreeTask_MoveInteractorToTarget.h"

#include "Components/SceneComponent.h"
#include "Device/WxDevice.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxWorldModule.h"

EStateTreeRunStatus FWxStateTreeTask_MoveInteractorToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 아래 어느 조기 완료 경로로 빠지든 ExitState 가 걸지도 않은 차단을 해제하는 일이 없어야 한다.
	Instance.BlockedController = nullptr;

	// 전이로 들어온 것이 아니면 StateTree 시작·세이브 복원·레이트조인이다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const AWxDevice* Owner = Cast<AWxDevice>(Context.GetOwner());
	ACharacter* Character = Owner ? Owner->GetInteractingCharacter() : nullptr;

	if (!Character || !Owner)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Move Interactor To Target: 오너 장치의 InteractingCharacter 가 비어 있다 — 완료로 넘어간다."));
		return EStateTreeRunStatus::Succeeded;
	}

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	// 이동 입력이 실제로 생기는 로컬 컨트롤 인스턴스에서만 건다.
	// 스냅·이동 두 경로 모두에서 걸어 ExitState 해제와 짝을 맞춘다.
	if (Character->IsLocallyControlled())
	{
		if (AController* Controller = Character->GetController())
		{
			Controller->SetIgnoreMoveInput(true);
			Instance.BlockedController = Controller;
		}
	}

	const FTransform Anchor = Instance.AnchorComponent ? Instance.AnchorComponent->GetComponentTransform() : Owner->GetActorTransform();
	const FVector TargetLocation = Anchor.TransformPosition(Instance.RelativeLocation);
	const float TargetYaw = (Anchor.GetRotation() * Instance.RelativeRotation.Quaternion()).Rotator().Yaw;

	const FVector StartLocation = Character->GetActorLocation();

	const bool bReachNow = Instance.Duration <= 0.f || StartLocation.Equals(TargetLocation);
	if (bReachNow)
	{
		Character->SetActorLocation(TargetLocation);
		if (Instance.bAlignRotation)
		{
			FRotator Rotation = Character->GetActorRotation();
			Rotation.Yaw = TargetYaw;
			Character->SetActorRotation(Rotation);
		}
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.MoveSpeed = (TargetLocation - StartLocation).Size() / Instance.Duration;
	if (Instance.bAlignRotation)
	{
		const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(Character->GetActorRotation().Yaw, TargetYaw));
		Instance.TurnSpeed = YawDelta / Instance.Duration;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_MoveInteractorToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	const AWxDevice* Owner = Cast<AWxDevice>(Context.GetOwner());
	ACharacter* Character = Owner ? Owner->GetInteractingCharacter() : nullptr;
	if (!Character || !Owner)
	{
		return EStateTreeRunStatus::Failed;
	}

	const FTransform Anchor = Instance.AnchorComponent ? Instance.AnchorComponent->GetComponentTransform() : Owner->GetActorTransform();
	const FVector TargetLocation = Anchor.TransformPosition(Instance.RelativeLocation);
	const float TargetYaw = (Anchor.GetRotation() * Instance.RelativeRotation.Quaternion()).Rotator().Yaw;

	FVector NewLocation = Character->GetActorLocation();
	if (!NewLocation.Equals(TargetLocation))
	{
		NewLocation = FMath::VInterpConstantTo(NewLocation, TargetLocation, DeltaTime, Instance.MoveSpeed);
		Character->SetActorLocation(NewLocation);
	}

	if (Instance.bAlignRotation)
	{
		FRotator Rotation = Character->GetActorRotation();
		Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, TargetYaw, Instance.TurnSpeed * DeltaTime);
		Character->SetActorRotation(Rotation);
	}

	if (!NewLocation.Equals(TargetLocation))
	{
		return EStateTreeRunStatus::Running;
	}

	if (Instance.bAlignRotation)
	{
		FRotator Rotation = Character->GetActorRotation();
		Rotation.Yaw = TargetYaw;
		Character->SetActorRotation(Rotation);
	}
	return EStateTreeRunStatus::Succeeded;
}

void FWxStateTreeTask_MoveInteractorToTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// SetIgnoreMoveInput 은 스택 카운터라 진입 시의 +1 과 짝을 맞춰야 한다.
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AController* Controller = Instance.BlockedController.Get())
	{
		Controller->SetIgnoreMoveInput(false);
	}

	Instance.BlockedController = nullptr;
}

#if WITH_EDITOR
FText FWxStateTreeTask_MoveInteractorToTarget::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 이동 대상은 오너 장치에서 읽으므로 표시할 것이 없다. 상태마다 갈리는 건 목표 앵커라 그것을 보인다.
	// 앵커는 보통 바인딩이라 런타임 포인터가 비어 있다.
	FText AnchorText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, AnchorComponent)), Formatting);
	if (AnchorText.IsEmpty())
	{
		AnchorText = InstanceData->AnchorComponent ? FText::FromString(InstanceData->AnchorComponent->GetName()) : INVTEXT("owner");
	}

	return FText::Format(INVTEXT("상호작용자 이동 ({0})"), AnchorText);
}
#endif
