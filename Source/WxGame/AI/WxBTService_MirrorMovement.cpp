// Copyright Woogle. All Rights Reserved.

#include "AI/WxBTService_MirrorMovement.h"
#include "AI/WxBlackboardKeys.h"
#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Effects/WxEffect_MoveSpeedOverride.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "WxGameplayTags.h"

UWxBTService_MirrorMovement::UWxBTService_MirrorMovement()
{
	NodeName = TEXT("Mirror Movement");
	bCreateNodeInstance = true;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
	Interval = 0.f;
	RandomDeviation = 0.f;
	MirrorTarget.SelectedKeyName = WxBlackboardKeys::Master;
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
	if (FollowerAbilitySystem.IsValid())
	{
		FollowerAbilitySystem->AbilityActivatedCallbacks.RemoveAll(this);
		if (UAnimInstance* AnimInstance = FollowerAbilitySystem->AbilityActorInfo.IsValid() ? FollowerAbilitySystem->AbilityActorInfo->GetAnimInstance() : nullptr)
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &ThisClass::HandleMontageEnded);
		}
		// MOV 가 자기 값으로 다시 계산되면 캐릭터가 MaxWalkSpeed 를 되돌린다.
		FollowerAbilitySystem->RemoveActiveGameplayEffect(MoveSpeedEffectHandle);
	}
	MoveSpeedEffectHandle.Invalidate();
	FollowerAbilitySystem.Reset();
	bPendingMontageEndTeleport = false;
	if (ACharacter* Pawn = Follower.Get())
	{
		// 비행 속도는 MOV 가 다루지 않아 클래스 기본값이 주인이다.
		Pawn->GetCharacterMovement()->MaxFlySpeed = Pawn->GetClass()->GetDefaultObject<ACharacter>()->GetCharacterMovement()->MaxFlySpeed;
		Pawn->GetCharacterMovement()->RemoveTickPrerequisiteComponent(&OwnerComp);
	}
	if (Master.IsValid()) { OwnerComp.RemoveTickPrerequisiteComponent(Master->GetCharacterMovement()); }
	Master.Reset();
	Follower.Reset();
	TravelTime = 0.f;
}

void UWxBTService_MirrorMovement::HandleAbilityActivated(UGameplayAbility* Ability)
{
	ACharacter* Pawn = Follower.Get();
	const ACharacter* Target = Master.Get();
	if (!Pawn || !Target || !Ability || !Ability->GetAssetTags().HasAnyExact(FaceMasterAbilityTags))
	{
		return;
	}

	// GAS의 PreActivate 알림이므로 몽타주 시작 전에 위치와 방향이 정해진다. 거절된 발동은 이 알림에 닿지 않는다.
	const FVector Forward = FRotator(0.f, Target->GetActorRotation().Yaw, 0.f).Vector();
	const FVector Destination = Target->GetActorLocation() + Forward * AbilityTeleportDistance;
	Pawn->TeleportTo(Destination, (-Forward).Rotation());
	const FRotator Facing(0.f, (Target->GetActorLocation() - Pawn->GetActorLocation()).Rotation().Yaw, 0.f);
	Pawn->SetActorRotation(Facing);
	if (AController* Controller = Pawn->GetController()) { Controller->SetControlRotation(Facing); }
	Pawn->ConsumeMovementInputVector();
	Pawn->GetCharacterMovement()->StopMovementImmediately();
	TravelTime = 0.f;
}

void UWxBTService_MirrorMovement::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 애님 갱신 중 통지라 이동은 BT 틱에서 한다. 콤보 다음 단계에 끊긴 몽타주도 매 단계 보정한다.
	bPendingMontageEndTeleport = true;
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
	if (!Pawn || !Target || Pawn == Target)
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
		FollowerAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (FollowerAbilitySystem.IsValid())
		{
			FollowerAbilitySystem->AbilityActivatedCallbacks.AddUObject(this, &ThisClass::HandleAbilityActivated);
			if (UAnimInstance* AnimInstance = FollowerAbilitySystem->AbilityActorInfo.IsValid() ? FollowerAbilitySystem->AbilityActorInfo->GetAnimInstance() : nullptr)
			{
				AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &ThisClass::HandleMontageEnded);
			}
		}
		OwnerComp.AddTickPrerequisiteComponent(Target->GetCharacterMovement());
		Movement->AddTickPrerequisiteComponent(&OwnerComp);
	}
	// 콜리전이 없어 바닥 판정을 못 하므로, 높이는 직접 맞추고 공중 자세만 Master 를 따른다(중력 0).
	const EMovementMode DesiredMode = SourceMovement->IsFalling() ? MOVE_Falling : MOVE_Flying;
	if (Movement->MovementMode != DesiredMode)
	{
		Movement->SetMovementMode(DesiredMode);
		if (DesiredMode == MOVE_Falling) { Movement->Velocity.Z = 0.f; }
	}
	const FVector Destination = Target->GetActorLocation() + Target->GetActorRotation().RotateVector(LocalOffset);
	Pawn->SetActorLocation(FVector(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, Destination.Z));
	if (FollowerAbilitySystem.IsValid() && FollowerAbilitySystem->HasAnyMatchingGameplayTags(FaceMasterAbilityTags))
	{
		const FRotator Facing(0.f, (Target->GetActorLocation() - Pawn->GetActorLocation()).Rotation().Yaw, 0.f);
		Pawn->SetActorRotation(Facing);
		Controller->SetControlRotation(Facing);
		Pawn->ConsumeMovementInputVector();
		TravelTime = 0.f;
		return;
	}
	Pawn->SetActorRotation(Target->GetActorRotation());
	Controller->SetControlRotation(Target->GetControlRotation());
	// MaxWalkSpeed 는 캐릭터가 MOV 로 정하므로, 직접 쓰지 않고 MOV 를 덮어써 Master 속도를 따른다.
	// 덮어쓰기라 따라 쓴 질주 같은 자기 MOV 효과는 Master 속도에 이미 들어 있어 무시된다.
	const float FollowSpeed = SourceMovement->MaxWalkSpeed * 1.25f;
	if (const FActiveGameplayEffect* SpeedEffect = FollowerAbilitySystem.IsValid() ? FollowerAbilitySystem->GetActiveGameplayEffect(MoveSpeedEffectHandle) : nullptr)
	{
		if (!FMath::IsNearlyEqual(SpeedEffect->Spec.GetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Magnitude, false), FollowSpeed))
		{
			FollowerAbilitySystem->UpdateActiveGameplayEffectSetByCallerMagnitude(MoveSpeedEffectHandle, WxGameplayTags::SetByCaller_Magnitude, FollowSpeed);
		}
	}
	else if (FollowerAbilitySystem.IsValid())
	{
		const FGameplayEffectSpecHandle SpecHandle = FollowerAbilitySystem->MakeOutgoingSpec(UWxEffect_MoveSpeedOverride::StaticClass(), 1.f, FollowerAbilitySystem->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Magnitude, FollowSpeed);
			MoveSpeedEffectHandle = FollowerAbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		}
	}
	Movement->MaxFlySpeed = FollowSpeed;

	// 몽타주 없이 켜져 있는 락온·질주는 몸을 쥐지 않으므로 보정을 막지 않는다.
	const bool bAbilityActive = FollowerAbilitySystem.IsValid() && FollowerAbilitySystem->GetAnimatingAbility();
	if (bPendingMontageEndTeleport && Pawn->TeleportTo(Destination, Target->GetActorRotation()))
	{
		bPendingMontageEndTeleport = false;
		TravelTime = 0.f;
		Movement->StopMovementImmediately();
	}
	FVector Error = Destination - Pawn->GetActorLocation();
	Error.Z = 0.f;
	if (Error.SizeSquared() <= FMath::Square(ArrivalRadius)) { TravelTime = 0.f; Error = FVector::ZeroVector; }
	else if (bAbilityActive) { TravelTime = 0.f; }
	else if (SourceMovement->IsMovingOnGround())
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
