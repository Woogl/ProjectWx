// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Controller/WxEnemyController.h"
#include "Component/WxNameplateComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxRewardLibrary.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Spawnable/WxSpawner.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxGameplayTags.h"

AWxEnemyCharacter::AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Enemy;

	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;

	// 네임플레이트 UI(아이콘, 남은 시간 비율)에 필요.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	NameplateComponent = CreateDefaultSubobject<UWxNameplateComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	NameplateComponent->InitializeViewModels(AbilitySystemComponent, CharacterName, Portrait);
}

bool AWxEnemyCharacter::IsInRearCone(const AActor* Interactor) const
{
	if (!Interactor)
	{
		return false;
	}

	FVector ToInteractor = Interactor->GetActorLocation() - GetActorLocation();
	ToInteractor.Z = 0.0;
	if (!ToInteractor.Normalize())
	{
		return false;
	}

	const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToInteractor);
	const float RearThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabRearHalfAngle));
	return ForwardDot <= RearThreshold;
}

void AWxEnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	if (!HasAuthority())
	{
		return;
	}

	if (AWxSpawner* Spawner = OwningSpawner.Get())
	{
		Spawner->MarkKilled();
	}


	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UWxRewardLibrary::GrantReward(this, RewardRow, PlayerController, GetActorTransform(), FVector::UpVector * LaunchSpeed);
	}
}

void AWxEnemyCharacter::OnInteracted(AActor* Interactor)
{
	// 서버 권위에서만 호출된다.
	if (!Interactor)
	{
		return;
	}

	const FGameplayTag EventTag = AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy)
		? WxGameplayTags::Event_Finisher
		: WxGameplayTags::Event_Backstab;

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = EventTag;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, EventTag, EventData);
}

FText AWxEnemyCharacter::GetInteractionPrompt() const
{
	return FText::FromString(TEXT("Finisher"));
}

AWxSpawner* AWxEnemyCharacter::GetOwningSpawner() const
{
	return OwningSpawner.Get();
}

void AWxEnemyCharacter::OnSpawnedBy(AWxSpawner* Spawner)
{
	OwningSpawner = Spawner;
}

bool AWxEnemyCharacter::CanInteract(const AActor* Interactor) const
{
	if (!IsAlive())
	{
		return false;
	}

	// 공격자 어빌리티(WxAbility_Finisher)가 연출 동안 대상에 State.BeingFinished 를 걸어 둔다.
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_BeingFinished))
	{
		return false;
	}

	// 그로기는 이미 무방비라 방향을 묻지 않는다(앞잡).
	if (AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		return true;
	}

	// 뒤잡은 적이 아직 나를 인지하지 못했을 때만 성립한다.
	return !AbilitySystemComponent->HasMatchingGameplayTag(WxGameplayTags::State_InCombat) && IsInRearCone(Interactor);
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

