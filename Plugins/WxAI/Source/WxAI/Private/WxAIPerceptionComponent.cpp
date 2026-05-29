// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxAIBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"

UWxAIPerceptionComponent::UWxAIPerceptionComponent()
{
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

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

	if (HearingConfig)
	{
		HearingConfig->HearingRange = HearingRange;
		ConfigureSense(*HearingConfig);
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

	// 청각은 위치만 기록(조사형). 타겟 확정(TargetActor)과 시야 확장은 시각/피해 전용.
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			BB->SetValueAsVector(WxAIBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
		}
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
