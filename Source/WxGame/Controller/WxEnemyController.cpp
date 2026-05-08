// Copyright Woogle. All Rights Reserved.

#include "WxEnemyController.h"
#include "AI/WxEnemyBlackboardKeys.h"
#include "Character/WxEnemyCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"

AWxEnemyController::AWxEnemyController()
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AWxEnemyController::HandleTargetPerceptionUpdated);
}

void AWxEnemyController::PostInitProperties()
{
	Super::PostInitProperties();
	
	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightAngle;
		AIPerceptionComponent->ConfigureSense(*SightConfig);
	}

	if (DamageConfig)
	{
		AIPerceptionComponent->ConfigureSense(*DamageConfig);
	}
}

void AWxEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(WxEnemyBlackboardKeys::SelfActor, InPawn);
		if (InPawn)
		{
			BB->SetValueAsVector(WxEnemyBlackboardKeys::HomeLocation, InPawn->GetActorLocation());
		}
	}

	if (AWxEnemyCharacter* Enemy = Cast<AWxEnemyCharacter>(InPawn))
	{
		if (UBehaviorTree* BT = Enemy->GetBehaviorTree())
		{
			RunBehaviorTree(BT);
		}
	}
}

void AWxEnemyController::OnUnPossess()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsObject(WxEnemyBlackboardKeys::SelfActor, nullptr);
	}

	Super::OnUnPossess();
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
		BB->SetValueAsObject(WxEnemyBlackboardKeys::TargetActor, Actor);
		BB->SetValueAsVector(WxEnemyBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
		SetAlerted(true);
	}
	else
	{
		if (BB->GetValueAsObject(WxEnemyBlackboardKeys::TargetActor) == Actor)
		{
			BB->SetValueAsVector(WxEnemyBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
			BB->ClearValue(WxEnemyBlackboardKeys::TargetActor);
			SetAlerted(false);
		}
	}
}

void AWxEnemyController::SetAlerted(bool bNewAlerted)
{
	SightConfig->PeripheralVisionAngleDegrees = bNewAlerted ? 180.f : SightAngle;
	AIPerceptionComponent->RequestStimuliListenerUpdate();
}
