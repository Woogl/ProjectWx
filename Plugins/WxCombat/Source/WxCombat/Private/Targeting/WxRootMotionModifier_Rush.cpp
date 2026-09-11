// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxRootMotionModifier_Rush.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "WxGameplayTags.h"

bool UWxRootMotionModifier_Rush::InitializeRush(AActor& Other, const UAnimNotifyState& Notify, float StopDistance, const TArray<TEnumAsByte<EObjectTypeQuery>>& IgnoreCollisions)
{
	ACharacter* Avatar = Cast<ACharacter>(GetActorOwner());
	UMotionWarpingComponent* Warping = GetOwnerComponent();
	UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Other);
	if (!Avatar || !Warping || Other.IsActorBeingDestroyed() || (OtherASC && OtherASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)))
	{
		return false;
	}
	SourceLocation = Avatar->GetActorLocation();
	FVector Destination = Other.GetActorLocation();
	if (const UMotionWarpingComponent* OtherWarping = Other.FindComponentByClass<UMotionWarpingComponent>())
	{
		for (const URootMotionModifier* Modifier : OtherWarping->GetModifiers())
		{
			const UWxRootMotionModifier_Rush* Rush = Cast<UWxRootMotionModifier_Rush>(Modifier);
			if (Rush && Rush->bOwnsState && Rush->Target.Get() == Avatar)
			{
				// 상대의 첫 애님 틱이 늦어도 이미 이동한 위치를 쫓지 않는다.
				Destination = Rush->SourceLocation;
				break;
			}
		}
	}
	Destination.Z = SourceLocation.Z;
	const FVector Direction = (Destination - SourceLocation).GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		return false;
	}
	Destination -= Direction * FMath::Clamp(StopDistance, 0.f, FVector::Dist2D(SourceLocation, Destination));
	Target = &Other;
	SourceNotify = &Notify;
	OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
	if (OwnerASC.IsValid() && OwnerASC->GetAnimatingAbility())
	{
		AbilityHandle = OwnerASC->GetAnimatingAbility()->GetCurrentAbilitySpecHandle();
	}
	Other.OnEndPlay.AddDynamic(this, &ThisClass::HandleTargetEndPlay);
	if (OtherASC)
	{
		OtherASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleTargetAbilityEnded);
	}
	bOwnsState = true;
	bSavedControllerYaw = Avatar->bUseControllerRotationYaw;
	bSavedPhysicsRotation = Avatar->GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion;
	Avatar->bUseControllerRotationYaw = false;
	Avatar->GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = false;
	Avatar->SetActorRotation(Direction.Rotation());
	if (AAIController* AI = Cast<AAIController>(Avatar->GetController()))
	{
		if (UBrainComponent* Brain = AI->GetBrainComponent(); Brain && Brain->IsRunning() && !Brain->IsPaused())
		{
			PausedBrain = Brain;
			Brain->PauseLogic(TEXT("Rush"));
		}
	}
	UCapsuleComponent* Capsule = Avatar->GetCapsuleComponent();
	SavedCollisionResponses = Capsule->GetCollisionResponseToChannels();
	for (const TEnumAsByte<EObjectTypeQuery> ObjectType : IgnoreCollisions)
	{
		const ECollisionChannel Channel = UEngineTypes::ConvertToCollisionChannel(ObjectType);
		if (Channel != ECC_MAX)
		{
			Capsule->SetCollisionResponseToChannel(Channel, ECR_Ignore);
			bChangedCollisionResponses = true;
		}
	}
	Warping->AddOrUpdateWarpTargetFromLocationAndRotation(WarpTargetName, Destination, Direction.Rotation());
	return true;
}

bool UWxRootMotionModifier_Rush::MatchesNotify(const UAnimNotifyState& Notify) const
{
	return SourceNotify.Get() == &Notify && bOwnsState;
}

void UWxRootMotionModifier_Rush::CancelRush()
{
	if (!bOwnsState)
	{
		return;
	}
	SetState(ERootMotionModifierState::Disabled);
}

void UWxRootMotionModifier_Rush::HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	CancelRush();
}

void UWxRootMotionModifier_Rush::HandleTargetAbilityEnded(const FAbilityEndedData& Data)
{
	const UMotionWarpingComponent* Warping = Target.IsValid() ? Target->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (Data.bWasCancelled && Warping && Data.AbilityThatEnded)
	{
		for (const URootMotionModifier* Modifier : Warping->GetModifiers())
		{
			const UWxRootMotionModifier_Rush* Rush = Cast<UWxRootMotionModifier_Rush>(Modifier);
			if (Rush && Rush->AbilityHandle == Data.AbilitySpecHandle)
			{
				CancelRush();
				return;
			}
		}
	}
}

void UWxRootMotionModifier_Rush::Update(const FMotionWarpingUpdateContext& Context)
{
	const UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target.Get());
	if (bOwnsState && (!Target.IsValid() || (OtherASC && OtherASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))))
	{
		CancelRush();
		return;
	}
	Super::Update(Context);
}

void UWxRootMotionModifier_Rush::OnStateChanged(ERootMotionModifierState LastState)
{
	Super::OnStateChanged(LastState);
	if (GetState() != ERootMotionModifierState::Waiting && GetState() != ERootMotionModifierState::Active)
	{
		ReleaseState();
	}
}

void UWxRootMotionModifier_Rush::ReleaseState()
{
	if (!bOwnsState)
	{
		return;
	}
	bOwnsState = false;
	AActor* Other = Target.Get();
	if (Other)
	{
		Other->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleTargetEndPlay);
		if (UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other))
		{
			OtherASC->OnAbilityEnded.RemoveAll(this);
		}
	}
	if (ACharacter* Avatar = Cast<ACharacter>(GetActorOwner()))
	{
		if (GetState() == ERootMotionModifierState::Disabled)
		{
			// 루트 모션에서 계산한 고속 Velocity가 보행 감속으로 넘어가지 않게 한다.
			Avatar->GetCharacterMovement()->StopMovementImmediately();
		}
		Avatar->bUseControllerRotationYaw = bSavedControllerYaw;
		Avatar->GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = bSavedPhysicsRotation;
		if (bChangedCollisionResponses)
		{
			Avatar->GetCapsuleComponent()->SetCollisionResponseToChannels(SavedCollisionResponses);
			bChangedCollisionResponses = false;
		}
	}
	if (UMotionWarpingComponent* Warping = GetOwnerComponent())
	{
		Warping->RemoveWarpTarget(WarpTargetName);
	}
	if (UBrainComponent* Brain = PausedBrain.Get(); Brain && Brain->IsPaused())
	{
		Brain->ResumeLogic(TEXT("RushEnded"));
	}
	PausedBrain.Reset();
}
