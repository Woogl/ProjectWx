// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_MoveInteractorToTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxGameplayTags.h"
#include "WxWorldModule.h"

EStateTreeRunStatus FWxStateTreeTask_MoveInteractorToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 차단 기록을 비운 상태로 시작한다. 아래 어느 조기 완료 경로로 빠지든 ExitState 가 걸지도 않은 차단을 해제하는 일이 없어야 한다.
	Instance.BlockedController = nullptr;
	Instance.BlockedAbilitySystem = nullptr;

	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 이동 없이 곧바로 완료한다.
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;

	if (!Character || !Owner)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Move Interactor To Target: 오너 기믹의 InteractingCharacter 가 비어 있다 — 완료로 넘어간다."));
		return EStateTreeRunStatus::Succeeded;
	}

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	// 이동/응시 동안 로컬 플레이어의 입력을 막는다(카메라 look 은 별개 게이트라 유지). ExitState 에서 짝 해제한다.
	// 입력이 실제로 생기고 예측이 발동을 게이트하는 로컬 컨트롤 인스턴스에서만 건다(소유 클라가 막으면 서버로 활성화가 전송되지 않아 서버 차단이 불필요). 스냅·이동 두 경로 모두에서 걸어 ExitState 해제와 짝을 맞춘다.
	if (Character->IsLocallyControlled())
	{
		if (AController* Controller = Character->GetController())
		{
			Controller->SetIgnoreMoveInput(true);
			Instance.BlockedController = Controller;
		}
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character))
		{
			ASC->BlockAbilitiesWithTags(FGameplayTagContainer(WxGameplayTags::Ability_Exclusive));
			Instance.BlockedAbilitySystem = ASC;
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

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;
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

	// 도달: yaw 를 목표로 스냅해 마무리한다.
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
	// EnterState 에서 건 입력 차단을 해제한다. SetIgnoreMoveInput·BlockAbilitiesWithTags 모두 스택 카운터라 진입 시의 +1 과 짝을 맞춰야 한다.
	// 진입 때 차단에 성공한 대상 자체를 기록해 두고, 여기서는 그 기록만 근거로 해제한다(기록이 비어 있으면 애초에 걸지 않은 것이다).
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	if (AController* Controller = Instance.BlockedController.Get())
	{
		Controller->SetIgnoreMoveInput(false);
	}
	if (UAbilitySystemComponent* ASC = Instance.BlockedAbilitySystem.Get())
	{
		ASC->UnBlockAbilitiesWithTags(FGameplayTagContainer(WxGameplayTags::Ability_Exclusive));
	}

	Instance.BlockedController = nullptr;
	Instance.BlockedAbilitySystem = nullptr;
}

#if WITH_EDITOR
FText FWxStateTreeTask_MoveInteractorToTarget::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 이동 대상은 오너 기믹에서 읽으므로 표시할 것이 없다. 상태마다 갈리는 건 목표 앵커라 그것을 보인다.
	// 앵커는 보통 바인딩이라 런타임 포인터가 비어 있다. 비우면 오너 트랜스폼 기준이므로 owner 로 폴백.
	FText AnchorText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, AnchorComponent)), Formatting);
	if (AnchorText.IsEmpty())
	{
		AnchorText = InstanceData->AnchorComponent ? FText::FromString(InstanceData->AnchorComponent->GetName()) : INVTEXT("owner");
	}

	return FText::Format(INVTEXT("상호작용자 이동 ({0})"), AnchorText);
}
#endif
