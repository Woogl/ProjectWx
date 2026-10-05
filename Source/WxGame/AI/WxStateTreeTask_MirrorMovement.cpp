// Copyright Woogle. All Rights Reserved.

#include "AI/WxStateTreeTask_MirrorMovement.h"

#include "AbilitySystem/Effects/WxEffect_MoveSpeedOverride.h"
#include "Minion/WxMinionComponent.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "StateTreeExecutionContext.h"

FWxStateTreeTask_MirrorMovement::FWxStateTreeTask_MirrorMovement()
{
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_MirrorMovement::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);

	AAIController* Controller = Cast<AAIController>(Context.GetOwner());
	UBrainComponent* Brain = Controller ? Controller->GetBrainComponent() : nullptr;
	ACharacter* Pawn = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	ACharacter* Target = Pawn ? Cast<ACharacter>(UWxMinionComponent::GetMaster(*Pawn)) : nullptr;
	if (!Brain || !Pawn || !Target || Pawn == Target)
	{
		Release(Controller, Instance);
		return EStateTreeRunStatus::Running;
	}

	UCharacterMovementComponent* Movement = Pawn->GetCharacterMovement();
	const UCharacterMovementComponent* SourceMovement = Target->GetCharacterMovement();
	if (Instance.Master != Target || Instance.Follower != Pawn)
	{
		Release(Controller, Instance);
		Instance.Master = Target;
		Instance.Follower = Pawn;
		Instance.FollowerAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (UAbilitySystemComponent* FollowerASC = Instance.FollowerAbilitySystem.Get())
		{
			// 통지 안에서는 인스턴스 데이터에 닿지 않는다. 따라 걷던 시간은 다음 틱의 마주 보기 분기가 0으로 되돌린다.
			Instance.AbilityActivatedHandle = FollowerASC->AbilityActivatedCallbacks.AddLambda(
				[WeakFollower = Instance.Follower, WeakMaster = Instance.Master, FaceMasterAbilityTags = Instance.FaceMasterAbilityTags, AbilityTeleportDistance = Instance.AbilityTeleportDistance](UGameplayAbility* Ability)
				{
					ACharacter* Follower = WeakFollower.Get();
					const ACharacter* Master = WeakMaster.Get();
					if (!Follower || !Master || !Ability || !Ability->GetAssetTags().HasAnyExact(FaceMasterAbilityTags))
					{
						return;
					}

					// GAS의 PreActivate 알림이므로 몽타주 시작 전에 위치와 방향이 정해진다. 거절된 발동은 이 알림에 닿지 않는다.
					const FVector Forward = FRotator(0.f, Master->GetActorRotation().Yaw, 0.f).Vector();
					const FVector Destination = Master->GetActorLocation() + Forward * AbilityTeleportDistance;
					Follower->TeleportTo(Destination, (-Forward).Rotation());
					const FRotator Facing(0.f, (Master->GetActorLocation() - Follower->GetActorLocation()).Rotation().Yaw, 0.f);
					Follower->SetActorRotation(Facing);
					if (AController* FollowerController = Follower->GetController())
					{
						FollowerController->SetControlRotation(Facing);
					}
					Follower->ConsumeMovementInputVector();
					Follower->GetCharacterMovement()->StopMovementImmediately();
				});
		}

		// 주인이 움직인 뒤에 목표를 재고, 그 입력으로 분신이 움직이도록 틱 순서를 건다.
		Brain->AddTickPrerequisiteComponent(Target->GetCharacterMovement());
		Movement->AddTickPrerequisiteComponent(Brain);
	}

	UAbilitySystemComponent* FollowerASC = Instance.FollowerAbilitySystem.Get();

	// 몽타주 종료 통지는 다이내믹 델리게이트라 구조체 태스크가 받을 수 없어, 지난 틱의 몽타주 인스턴스가 사라졌는지로 종료를 안다.
	// 콤보 다음 단계에 끊긴 몽타주도 인스턴스가 따로라 매 단계 보정한다.
	if (const UAnimInstance* AnimInstance = FollowerASC && FollowerASC->AbilityActorInfo.IsValid() ? FollowerASC->AbilityActorInfo->GetAnimInstance() : nullptr)
	{
		TArray<int32> CurrentMontageInstanceIDs;
		for (const FAnimMontageInstance* MontageInstance : AnimInstance->MontageInstances)
		{
			if (MontageInstance)
			{
				CurrentMontageInstanceIDs.Add(MontageInstance->GetInstanceID());
			}
		}

		for (const int32 MontageInstanceID : Instance.MontageInstanceIDs)
		{
			if (!CurrentMontageInstanceIDs.Contains(MontageInstanceID))
			{
				Instance.bPendingMontageEndTeleport = true;
				break;
			}
		}
		Instance.MontageInstanceIDs = MoveTemp(CurrentMontageInstanceIDs);
	}

	// 콜리전이 없어 바닥 판정을 못 하므로, 높이는 직접 맞추고 공중 자세만 주인을 따른다(중력 0).
	const EMovementMode DesiredMode = SourceMovement->IsFalling() ? MOVE_Falling : MOVE_Flying;
	if (Movement->MovementMode != DesiredMode)
	{
		Movement->SetMovementMode(DesiredMode);
		if (DesiredMode == MOVE_Falling)
		{
			Movement->Velocity.Z = 0.f;
		}
	}

	const FVector Destination = Target->GetActorLocation() + Target->GetActorRotation().RotateVector(Instance.LocalOffset);
	Pawn->SetActorLocation(FVector(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, Destination.Z));

	if (FollowerASC && FollowerASC->HasAnyMatchingGameplayTags(Instance.FaceMasterAbilityTags))
	{
		const FRotator Facing(0.f, (Target->GetActorLocation() - Pawn->GetActorLocation()).Rotation().Yaw, 0.f);
		Pawn->SetActorRotation(Facing);
		Controller->SetControlRotation(Facing);
		Pawn->ConsumeMovementInputVector();
		Instance.TravelTime = 0.f;
		return EStateTreeRunStatus::Running;
	}

	Pawn->SetActorRotation(Target->GetActorRotation());
	Controller->SetControlRotation(Target->GetControlRotation());

	// MaxWalkSpeed 는 캐릭터가 MOV 로 정하므로, 직접 쓰지 않고 MOV 를 덮어써 주인 속도를 따른다.
	// 덮어쓰기라 따라 쓴 질주 같은 자기 MOV 효과는 주인 속도에 이미 들어 있어 무시된다.
	const float FollowSpeed = SourceMovement->MaxWalkSpeed * 1.25f;
	if (const FActiveGameplayEffect* SpeedEffect = FollowerASC ? FollowerASC->GetActiveGameplayEffect(Instance.MoveSpeedEffectHandle) : nullptr)
	{
		if (!FMath::IsNearlyEqual(SpeedEffect->Spec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Magnitude, false), FollowSpeed))
		{
			FollowerASC->UpdateActiveGameplayEffectSetByCallerMagnitude(Instance.MoveSpeedEffectHandle, WxGameplayTags::SetByCaller_Magnitude, FollowSpeed);
		}
	}
	else if (FollowerASC)
	{
		const FGameplayEffectSpecHandle SpecHandle = FollowerASC->MakeOutgoingSpec(UWxEffect_MoveSpeedOverride::StaticClass(), 1.f, FollowerASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Magnitude, FollowSpeed);
			Instance.MoveSpeedEffectHandle = FollowerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}
	}
	Movement->MaxFlySpeed = FollowSpeed;

	// 몽타주 없이 켜져 있는 락온·질주는 몸을 쥐지 않으므로 보정을 막지 않는다.
	const bool bAbilityActive = FollowerASC && FollowerASC->GetAnimatingAbility();
	if (Instance.bPendingMontageEndTeleport && Pawn->TeleportTo(Destination, Target->GetActorRotation()))
	{
		Instance.bPendingMontageEndTeleport = false;
		Instance.TravelTime = 0.f;
		Movement->StopMovementImmediately();
	}

	FVector Error = Destination - Pawn->GetActorLocation();
	Error.Z = 0.f;
	if (Error.SizeSquared() <= FMath::Square(Instance.ArrivalRadius))
	{
		Instance.TravelTime = 0.f;
		Error = FVector::ZeroVector;
	}
	else if (bAbilityActive)
	{
		Instance.TravelTime = 0.f;
	}
	else if (SourceMovement->IsMovingOnGround())
	{
		Instance.TravelTime += DeltaTime;

		// 충돌이 있는 목표에는 무조건 겹쳐 넣지 않고 TeleportTo의 배치 검사를 따른다.
		if (Instance.TravelTime >= Instance.TeleportDelay && Pawn->TeleportTo(Destination, Target->GetActorRotation()))
		{
			Instance.TravelTime = 0.f;
			Movement->StopMovementImmediately();
			Error = FVector::ZeroVector;
		}
	}
	else
	{
		Instance.TravelTime = 0.f;
	}

	const FVector DesiredVelocity = FVector(Target->GetVelocity().X, Target->GetVelocity().Y, 0.f) + Error * 4.f;
	Pawn->AddMovementInput(DesiredVelocity.GetSafeNormal2D(), FMath::Min(DesiredVelocity.Size2D() / FMath::Max(Movement->GetMaxSpeed(), 1.f), 1.f));

	return EStateTreeRunStatus::Running;
}

void FWxStateTreeTask_MirrorMovement::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Release(Cast<AAIController>(Context.GetOwner()), Instance);
}

void FWxStateTreeTask_MirrorMovement::Release(AAIController* Controller, FInstanceDataType& Instance) const
{
	if (UAbilitySystemComponent* FollowerASC = Instance.FollowerAbilitySystem.Get())
	{
		FollowerASC->AbilityActivatedCallbacks.Remove(Instance.AbilityActivatedHandle);

		// MOV 가 자기 값으로 다시 계산되면 캐릭터가 MaxWalkSpeed 를 되돌린다.
		FollowerASC->RemoveActiveGameplayEffect(Instance.MoveSpeedEffectHandle);
	}
	Instance.AbilityActivatedHandle.Reset();
	Instance.MoveSpeedEffectHandle.Invalidate();
	Instance.FollowerAbilitySystem.Reset();
	Instance.MontageInstanceIDs.Reset();
	Instance.bPendingMontageEndTeleport = false;

	UBrainComponent* Brain = Controller ? Controller->GetBrainComponent() : nullptr;
	if (ACharacter* Pawn = Instance.Follower.Get())
	{
		// 비행 속도는 MOV 가 다루지 않아 클래스 기본값이 주인이다.
		Pawn->GetCharacterMovement()->MaxFlySpeed = Pawn->GetClass()->GetDefaultObject<ACharacter>()->GetCharacterMovement()->MaxFlySpeed;
		if (Brain)
		{
			Pawn->GetCharacterMovement()->RemoveTickPrerequisiteComponent(Brain);
		}
	}
	if (Brain && Instance.Master.IsValid())
	{
		Brain->RemoveTickPrerequisiteComponent(Instance.Master->GetCharacterMovement());
	}
	Instance.Master.Reset();
	Instance.Follower.Reset();
	Instance.TravelTime = 0.f;
}
