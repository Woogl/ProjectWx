// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxAIBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"

UWxAIPerceptionComponent::UWxAIPerceptionComponent()
{
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	SetDominantSense(UAISense_Sight::StaticClass());
	OnTargetPerceptionUpdated.AddDynamic(this, &UWxAIPerceptionComponent::HandleTargetPerceptionUpdated);
}

void UWxAIPerceptionComponent::PostInitProperties()
{
	Super::PostInitProperties();

	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightAngle;
		ConfigureSense(*SightConfig);
	}

	if (DamageConfig)
	{
		ConfigureSense(*DamageConfig);
	}
}

void UWxAIPerceptionComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BB = GetBlackboard();
	if (!BB)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		BB->SetValueAsObject(WxAIBlackboardKeys::TargetActor, Actor);
		BB->SetValueAsVector(WxAIBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
		SetAlerted(true);
	}
	else
	{
		if (BB->GetValueAsObject(WxAIBlackboardKeys::TargetActor) == Actor)
		{
			BB->SetValueAsVector(WxAIBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
			BB->ClearValue(WxAIBlackboardKeys::TargetActor);
			SetAlerted(false);
		}
	}
}

void UWxAIPerceptionComponent::SetAlerted(bool bNewAlerted)
{
	if (!SightConfig)
	{
		return;
	}

	SightConfig->PeripheralVisionAngleDegrees = bNewAlerted ? 180.f : SightAngle;
	RequestStimuliListenerUpdate();
}

UBlackboardComponent* UWxAIPerceptionComponent::GetBlackboard() const
{
	if (AAIController* AIC = Cast<AAIController>(GetOwner()))
	{
		return AIC->GetBlackboardComponent();
	}
	return nullptr;
}
