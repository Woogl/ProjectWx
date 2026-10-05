// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/WxAbilityTask_Rush.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Tasks/WxAbilityTask_LockMovementRotation.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"

UWxAbilityTask_Rush* UWxAbilityTask_Rush::CreateTask(UGameplayAbility* OwningAbility, int32 MontageInstanceID, AActor& Other, float InDuration, float StopDistance, const TArray<TEnumAsByte<EObjectTypeQuery>>& InIgnoreCollisions)
{
	ACharacter* Avatar = OwningAbility ? Cast<ACharacter>(OwningAbility->GetAvatarActorFromActorInfo()) : nullptr;
	const UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Other);
	if (!Avatar || InDuration <= 0.f || Other.IsActorBeingDestroyed() || (OtherASC && OtherASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)))
	{
		return nullptr;
	}
	if (UWxAbilityTask_Rush* Previous = FindRush(*Avatar))
	{
		Previous->EndTask();
	}

	const FVector Source = Avatar->GetActorLocation();
	FVector Destination = Other.GetActorLocation();
	UWxAbilityTask_Rush* Partner = FindRush(Other);
	if (Partner && Partner->Target.Get() == Avatar)
	{
		// 상대가 먼저 움직였어도 교차 목적지는 상대의 출발점으로 고정한다.
		Destination = Partner->StartLocation;
	}
	Destination.Z = Source.Z;
	const FVector Direction = (Destination - Source).GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		return nullptr;
	}
	Destination -= Direction * FMath::Clamp(StopDistance, 0.f, FVector::Dist2D(Source, Destination));

	UWxAbilityTask_Rush* Task = NewAbilityTask<UWxAbilityTask_Rush>(OwningAbility, TEXT("WxRush"));
	Task->ForceName = TEXT("WxRush");
	Task->StartLocation = Source;
	Task->TargetLocation = Destination;
	Task->Duration = InDuration;
	Task->bRestrictSpeedToExpected = true;
	Task->FinishVelocityMode = ERootMotionFinishVelocityMode::SetVelocity;
	Task->FinishSetVelocity = FVector::ZeroVector;
	Task->IgnoreCollisions = InIgnoreCollisions;
	Task->Target = &Other;
	Task->AbilityHandle = OwningAbility->GetCurrentAbilitySpecHandle();
	Task->SourceMontageInstanceID = MontageInstanceID;
	Task->TrackPartner();
	Task->SharedInitAndApply();
	return Task;
}

UWxAbilityTask_Rush* UWxAbilityTask_Rush::FindRush(AActor& Actor)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);
	if (ASC)
	{
		for (auto It = ASC->GetKnownTaskIterator(); It; ++It)
		{
			UWxAbilityTask_Rush* Task = Cast<UWxAbilityTask_Rush>(*It);
			if (Task && Task->bOwnsState)
			{
				return Task;
			}
		}
	}
	return nullptr;
}

void UWxAbilityTask_Rush::SharedInitAndApply()
{
	Super::SharedInitAndApply();
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActor());
	UCharacterMovementComponent* Movement = MovementComponent.Get();
	if (!Avatar || !Movement)
	{
		return;
	}
	if (const TSharedPtr<FRootMotionSource> Source = Movement->GetRootMotionSourceByID(RootMotionSourceID))
	{
		// 수평 이동만 덮어쓰고 단차와 낙하의 Z 처리는 CMC에 맡긴다.
		Source->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
	}
	bOwnsState = true;
	UAnimInstance* AnimInstance = Avatar->GetMesh() ? Avatar->GetMesh()->GetAnimInstance() : nullptr;
	// 몽타주 ID는 머신마다 다르므로 로컬 노티파이가 지정한 인스턴스만 잠근다.
	if (FAnimMontageInstance* Instance = AnimInstance ? AnimInstance->GetMontageInstanceForID(SourceMontageInstanceID) : nullptr)
	{
		// 노티파이 끝이 먼저 처리되는 마지막 프레임에도 이동 소스가 마지막 이동분을 적용해야 한다.
		Instance->PushDisableRootMotion();
		SourceAnimInstance = AnimInstance;
	}
	// 회전 모드 플래그는 락온이 소유하므로 건드리지 않고, 중첩을 처리하는 회전 속도 잠금으로 CMC 회전을 막는다.
	if (Ability)
	{
		UWxAbilityTask_LockMovementRotation* Lock = UWxAbilityTask_LockMovementRotation::CreateTask(Ability);
		Lock->ReadyForActivation();
		RotationLock = Lock;
	}
	const FVector Direction = TargetLocation - StartLocation;
	if (!Direction.IsNearlyZero())
	{
		Avatar->SetActorRotation(Direction.Rotation());
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
	if (AActor* Other = Target.Get())
	{
		Other->OnEndPlay.AddDynamic(this, &ThisClass::HandleTargetEndPlay);
		if (UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other))
		{
			OtherASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleTargetAbilityEnded);
		}
	}
}

void UWxAbilityTask_Rush::TrackPartner()
{
	AActor* Other = Target.Get();
	UWxAbilityTask_Rush* Partner = Other ? FindRush(*Other) : nullptr;
	if (Partner && Partner->Target.Get() == GetAvatarActor())
	{
		PartnerAbilityHandle = Partner->AbilityHandle;
		Partner->PartnerAbilityHandle = AbilityHandle;
	}
}

void UWxAbilityTask_Rush::TickTask(float DeltaTime)
{
	if (!bIsSimulating && !bIsFinished)
	{
		AActor* Other = Target.Get();
		const UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
		if (!Other || Other->IsActorBeingDestroyed() || (OtherASC && OtherASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)))
		{
			EndTask();
			return;
		}
		TrackPartner();
	}
	Super::TickTask(DeltaTime);
	if (bIsFinished)
	{
		// 모의 프록시 태스크는 복제 삭제까지 남지만 충돌 설정은 이동 종료 시점에 되돌린다.
		ReleaseState();
	}
}

void UWxAbilityTask_Rush::HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	EndTask();
}

void UWxAbilityTask_Rush::HandleTargetAbilityEnded(const FAbilityEndedData& Data)
{
	if (Data.bWasCancelled && PartnerAbilityHandle.IsValid() && Data.AbilitySpecHandle == PartnerAbilityHandle)
	{
		EndTask();
	}
}

void UWxAbilityTask_Rush::OnDestroy(bool AbilityIsEnding)
{
	if (!bIsFinished && MovementComponent.IsValid())
	{
		MovementComponent->StopMovementImmediately();
	}
	ReleaseState();
	Super::OnDestroy(AbilityIsEnding);
}

void UWxAbilityTask_Rush::ReleaseState()
{
	if (!bOwnsState)
	{
		return;
	}
	bOwnsState = false;
	if (UAnimInstance* AnimInstance = SourceAnimInstance.Get())
	{
		if (FAnimMontageInstance* Instance = AnimInstance->GetMontageInstanceForID(SourceMontageInstanceID))
		{
			Instance->PopDisableRootMotion();
		}
	}
	SourceAnimInstance.Reset();
	SourceMontageInstanceID = INDEX_NONE;
	if (UWxAbilityTask_LockMovementRotation* Lock = RotationLock.Get())
	{
		Lock->EndTask();
	}
	RotationLock.Reset();
	if (AActor* Other = Target.Get())
	{
		Other->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleTargetEndPlay);
		if (UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other))
		{
			OtherASC->OnAbilityEnded.RemoveAll(this);
		}
	}
	if (ACharacter* Avatar = Cast<ACharacter>(GetAvatarActor()); Avatar && bChangedCollisionResponses)
	{
		Avatar->GetCapsuleComponent()->SetCollisionResponseToChannels(SavedCollisionResponses);
		bChangedCollisionResponses = false;
	}
}

void UWxAbilityTask_Rush::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWxAbilityTask_Rush, IgnoreCollisions);
}
