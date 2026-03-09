// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

AWxEnemyCharacter::AWxEnemyCharacter()
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void AWxEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitAbilityActorInfo();
}

void AWxEnemyCharacter::InitAbilityActorInfo()
{
	Super::InitAbilityActorInfo();
}
