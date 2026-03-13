// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "AI/WxAIController.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	AIControllerClass = AWxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

