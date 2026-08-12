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

	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 이동 없이 곧바로 완료한다(발동 순간에만 동작; InteractingCharacter 는 비영속이라 복원 시 비어 있음).
	if (!Transition.SourceStateID.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 당사자는 오너 기믹이 권위에서 쓰고 복제하는 값이라 모든 피어가 같은 대상을 본다(에셋 배선 없음).
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr;
	ACharacter* Character = Gimmick ? Gimmick->GetInteractingCharacter() : nullptr;

	// 당사자가 없으면(비캐릭터 상호작용, 비기믹 오너 등) 상태가 갇히지 않게 곧바로 완료한다.
	if (!Character || !Owner)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Move Interactor To Target: 오너 기믹의 InteractingCharacter 가 비어 있다 — 완료로 넘어간다."));
		return EStateTreeRunStatus::Succeeded;
	}

	// 스크립트 이동 동안 CMC 잔여 속도를 제거한다.
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	// 이동/응시 동안 로컬 플레이어의 입력을 막는다(카메라 look 은 별개 게이트라 유지). ExitState 에서 짝 해제한다.
	//  - 이동: AController::SetIgnoreMoveInput 로 AddMovementInput 을 무시.
	//  - 어빌리티+점프: ASC 의 BlockAbilitiesWithTags(Ability.Exclusive) — 액션 어빌리티가 연출 중 서로를 막는 것과 동일한 GAS 순정 관례이며, 캐릭터의 CanJumpInternal 이 같은 태그로 점프를 막으므로 점프도 함께 차단된다.
	// 입력이 실제로 생기고 예측이 발동을 게이트하는 로컬 컨트롤 인스턴스에서만 건다(소유 클라가 막으면 서버로 활성화가 전송되지 않아 서버 차단이 불필요). 스냅·이동 두 경로 모두에서 걸어 ExitState 해제와 짝을 맞춘다.
	// 차단에 성공한 대상은 그때그때 인스턴스에 기록해 둔다 — ExitState 는 이 기록만 보고 해제하므로, 그 사이 캐릭터가 소멸·언포제스돼도 카운터가 새지 않는다.
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

	// 목표 = 앵커(또는 오너) 트랜스폼 ∘ 상대오프셋. 모든 머신에서 동일하게 합성돼 수렴한다.
	const FTransform Anchor = Instance.AnchorComponent ? Instance.AnchorComponent->GetComponentTransform() : Owner->GetActorTransform();
	const FVector TargetLocation = Anchor.TransformPosition(Instance.RelativeLocation);
	const float TargetYaw = (Anchor.GetRotation() * Instance.RelativeRotation.Quaternion()).Rotator().Yaw;

	const FVector StartLocation = Character->GetActorLocation();

	// Duration 0 이하·이미 목표면 즉시 스냅해 곧바로 완료, 아니면 시작→목표 실제 거리/시간으로 등속을 1회 산출하고 Tick 이 슬라이드한다.
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

	// 도달 전까지 EnterState 에서 산출한 등속으로 슬라이드한다.
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

	// 아직 도달 전이면 계속 이동한다.
	if (!NewLocation.Equals(TargetLocation))
	{
		return EStateTreeRunStatus::Running;
	}

	// 도달: yaw 를 목표로 스냅해 마무리하고 상태를 완료시킨다.
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
	// 해제 대상을 오너 기믹의 InteractingCharacter 로 되짚지 않는 이유: 그 값은 권위 측이 언제든 갱신하는 라이브 멤버라 진입 시점의 스냅샷이 아니다.
	// 이동 중 캐릭터가 파괴되거나(Tick 이 Failed 반환) 언포제스되면 그 경로로는 대상을 잃어 해제가 통째로 스킵되고, 컨트롤러에 쌓인 카운터가 리스폰 후에도 남는다.
	// 그래서 진입 때 차단에 성공한 대상 자체를 기록해 두고, 여기서는 그 기록만 근거로 해제한다(기록이 비어 있으면 애초에 걸지 않은 것이다).
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
	// 앵커는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 비우면 오너 트랜스폼 기준이므로 owner 로 폴백.
	FText AnchorText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, AnchorComponent)), Formatting);
	if (AnchorText.IsEmpty())
	{
		AnchorText = InstanceData->AnchorComponent ? FText::FromString(InstanceData->AnchorComponent->GetName()) : INVTEXT("owner");
	}

	return FText::Format(INVTEXT("Move Interactor To Target ({0})"), AnchorText);
}
#endif
