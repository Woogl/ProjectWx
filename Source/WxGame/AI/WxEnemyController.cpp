// Copyright Woogle. All Rights Reserved.

#include "AI/WxEnemyController.h"
#include "Character/WxEnemyCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"

const FName AWxEnemyController::BBKey_TargetActor = TEXT("TargetActor");
const FName AWxEnemyController::BBKey_Alerted = TEXT("Alerted");

AWxEnemyController::AWxEnemyController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = SightAngle;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->SetMaxAge(MemoryLength);

	AIPerceptionComponent->ConfigureSense(*SightConfig);

	UAISenseConfig_Damage* DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(MemoryLength);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);

	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AWxEnemyController::HandleTargetPerceptionUpdated);
}

void AWxEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		if (UBehaviorTree* BT = Enemy->GetBehaviorTree())
		{
			RunBehaviorTree(BT);
		}
	}
}

void AWxEnemyController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BB->SetValueAsObject(BBKey_TargetActor, Actor);
		SetAlerted(true);
	}
	else
	{
		if (BB->GetValueAsObject(BBKey_TargetActor) == Actor)
		{
			BB->ClearValue(BBKey_TargetActor);
			SetAlerted(false);
		}
	}
}

void AWxEnemyController::SetAlerted(bool bNewAlerted)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	if (BB->GetValueAsBool(BBKey_Alerted) == bNewAlerted)
	{
		return;
	}

	BB->SetValueAsBool(BBKey_Alerted, bNewAlerted);
	SightConfig->PeripheralVisionAngleDegrees = bNewAlerted ? 180.f : SightAngle;
	AIPerceptionComponent->RequestStimuliListenerUpdate();
}
