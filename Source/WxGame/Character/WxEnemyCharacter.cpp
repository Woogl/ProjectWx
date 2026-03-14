// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "AI/WxEnemyController.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	AIControllerClass = AWxEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	bUseControllerRotationYaw = true;
	
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
}

UBehaviorTree* AWxEnemyCharacter::GetBehaviorTree() const
{
	return BehaviorTreeAsset;
}

