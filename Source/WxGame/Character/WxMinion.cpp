// Copyright Woogle. All Rights Reserved.

#include "Character/WxMinion.h"
#include "Controller/WxAIController.h"

AWxMinion::AWxMinion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = AWxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
