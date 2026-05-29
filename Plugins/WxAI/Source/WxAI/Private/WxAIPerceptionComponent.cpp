// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxAIBlackboardKeys.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
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
		SightConfig->LoseSightRadius = SightRadius;
		SightConfig->PeripheralVisionAngleDegrees = SightAngle;
		ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = MaxHearingRange;
		ConfigureSense(*HearingConfig);
	}

	if (DamageConfig)
	{
		ConfigureSense(*DamageConfig);
	}
}

void UWxAIPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 인식 판정은 감지 이벤트뿐 아니라 폰의 이동(귀환/리시 이탈)에도 좌우되므로, 항상 일정 주기로 재판정한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(LeashTimerHandle, this, &UWxAIPerceptionComponent::UpdateRecognition, 1.f, true);
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
	}
	else
	{
		if (BB->GetValueAsObject(WxAIBlackboardKeys::TargetActor) == Actor)
		{
			// 시야를 잃으면 TargetActor 만 비워 BT 를 조사/귀환으로 전환시킨다.
			BB->SetValueAsVector(WxAIBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
			BB->ClearValue(WxAIBlackboardKeys::TargetActor);
		}
	}

	// 인식 판정은 UpdateRecognition 한 곳에서만 한다. 여기서는 BB(TargetActor)만 갱신하고 판정을 위임한다.
	UpdateRecognition();
}

void UWxAIPerceptionComponent::SetRecognized(bool bNewRecognized)
{
	// 인식 상태를 폰의 ASC 태그로 발행한다. MinimalReplication 태그는 GE 없이 서버→클라이언트로
	// 복제(COND_SkipOwner)되어, 각 클라이언트의 네임플레이트가 이 태그를 읽어 표시를 결정한다.
	const AAIController* AIC = Cast<AAIController>(GetOwner());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AIC ? AIC->GetPawn() : nullptr);
	if (!ASC)
	{
		return;
	}

	if (bNewRecognized)
	{
		ASC->AddMinimalReplicationGameplayTag(WxGameplayTags::State_Recognized);
	}
	else
	{
		ASC->RemoveMinimalReplicationGameplayTag(WxGameplayTags::State_Recognized);
	}
}

void UWxAIPerceptionComponent::UpdateRecognition()
{
	const AAIController* AIC = Cast<AAIController>(GetOwner());
	const APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* BB = GetBlackboard();
	if (!Pawn || !BB)
	{
		SetRecognized(false);
		return;
	}

	// 타겟을 직접 확인 중이면 인식한다(최초 감지 시 즉시 ON, 교전 중 유지).
	if (BB->GetValueAsObject(WxAIBlackboardKeys::TargetActor) != nullptr)
	{
		SetRecognized(true);
		return;
	}

	// 타겟을 놓친 상태에서만 추적 종료를 판정한다. 그 전까지는 인식을 유지한다(벽 뒤 등 일시적 시야 상실). 기획서 4.3:
	//  - 배치 지점에서 LeashRadius 이상 멀어짐(리시 이탈), 또는
	//  - 배치 지점 HomeArrivalRadius 안으로 귀환 완료.
	const FVector HomeLocation = BB->GetValueAsVector(WxAIBlackboardKeys::HomeLocation);
	const float DistSquared = FVector::DistSquared(Pawn->GetActorLocation(), HomeLocation);
	if (DistSquared > LeashRadius * LeashRadius || DistSquared <= HomeArrivalRadius * HomeArrivalRadius)
	{
		SetRecognized(false);
	}
}

void UWxAIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LeashTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

UBlackboardComponent* UWxAIPerceptionComponent::GetBlackboard() const
{
	if (AAIController* AIC = Cast<AAIController>(GetOwner()))
	{
		return AIC->GetBlackboardComponent();
	}
	return nullptr;
}
