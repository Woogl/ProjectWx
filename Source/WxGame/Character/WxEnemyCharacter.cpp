// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "Character/Component/WxAIBehaviorComponent.h"
#include "Character/Component/WxEnemyComponent.h"
#include "Component/WxNameplateComponent.h"
#include "Controller/WxAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Targeting/WxLockOnPointComponent.h"

AWxEnemyCharacter::AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Enemy;
	AIControllerClass = AWxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	AIBehaviorComponent = CreateDefaultSubobject<UWxAIBehaviorComponent>(TEXT("AIBehaviorComponent"));

	NameplateComponent = CreateDefaultSubobject<UWxNameplateComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));

	EnemyComponent = CreateDefaultSubobject<UWxEnemyComponent>(TEXT("EnemyComponent"));
}

void AWxEnemyCharacter::OnSpawnedBy(AWxSpawner* Spawner)
{
	if (EnemyComponent)
	{
		EnemyComponent->HandleSpawnedBy(Spawner);
	}
}

bool AWxEnemyCharacter::CanInteract(const AActor* Interactor) const
{
	return EnemyComponent && EnemyComponent->CanInteract(Interactor);
}

void AWxEnemyCharacter::OnInteracted(AActor* Interactor)
{
	if (EnemyComponent)
	{
		EnemyComponent->Interact(Interactor);
	}
}

FText AWxEnemyCharacter::GetInteractionPrompt() const
{
	return EnemyComponent ? EnemyComponent->GetInteractionPrompt() : FText::GetEmpty();
}
