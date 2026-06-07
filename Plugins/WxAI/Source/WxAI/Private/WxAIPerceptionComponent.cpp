// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxAIBlackboardKeys.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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
		SetTargetActor(Actor);
	}
	else if (BB->GetValueAsObject(WxAIBlackboardKeys::TargetActor) == Actor)
	{
		// 시야를 잃어도 TargetActor 는 유지한다(보스 등 뒤로 이동 등 일시적 상실). 마지막 인지 위치만 갱신하고,
		// 실제 해제는 UpdateRecognition 의 리시 이탈 판정에 맡긴다.
		BB->SetValueAsVector(WxAIBlackboardKeys::TargetLastKnownLocation, Stimulus.StimulusLocation);
	}

	// 인식/추적 판정은 UpdateRecognition 한 곳에서만 한다. 여기서는 BB(TargetActor/LastKnown)만 갱신하고 판정을 위임한다.
	UpdateRecognition();
}

void UWxAIPerceptionComponent::SetRecognized(bool bNewRecognized)
{
	// 인식 상태를 폰의 ASC 태그로 발행한다. MinimalReplication 태그는 GE 없이 서버→클라이언트로
	// 복제(COND_SkipOwner)되어, 각 클라이언트의 네임플레이트가 이 태그를 읽어 표시를 결정한다.
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwnerPawn());
	if (!ASC)
	{
		return;
	}
	
	const bool bCurrentlyRecognized = ASC->HasMatchingGameplayTag(WxGameplayTags::State_Recognized);
	if (bNewRecognized == bCurrentlyRecognized)
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

void UWxAIPerceptionComponent::SetTargetActor(AActor* NewTarget)
{
	UBlackboardComponent* BB = GetBlackboard();
	if (!BB)
	{
		return;
	}

	if (BB->GetValueAsObject(WxAIBlackboardKeys::TargetActor) == NewTarget)
	{
		return;
	}

	if (NewTarget)
	{
		BB->SetValueAsObject(WxAIBlackboardKeys::TargetActor, NewTarget);
	}
	else
	{
		BB->ClearValue(WxAIBlackboardKeys::TargetActor);
	}

	// 타겟 유무에 따라 회전 모드를 발행한다(상태는 원천이 발행). 타겟이 있으면 그 액터를 포커스로 두고
	// bUseControllerDesiredRotation 으로 전환해, 컨트롤러가 매 틱 포커스 방향으로 갱신하는 ControlRotation 으로
	// CharacterMovement 가 RotationRate 만큼 회전한다 → 타겟을 바라본 채 이동(strafe). 없으면 이동 방향으로 회전하는 평상시 로코모션으로 되돌린다.
	// 포커스는 Gameplay(2) 우선순위라 경로 추종(MoveTo)의 Move(1) 포커스보다 우선하므로, 추격 중에도 타겟을 바라본다.
	AAIController* AIC = Cast<AAIController>(GetOwner());
	ACharacter* Character = AIC ? Cast<ACharacter>(AIC->GetPawn()) : nullptr;
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (NewTarget)
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		AIC->SetFocus(NewTarget);
	}
	else
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		Movement->bUseControllerDesiredRotation = false;
		Movement->bOrientRotationToMovement = true;
	}
}

void UWxAIPerceptionComponent::UpdateRecognition()
{
	const APawn* Pawn = GetOwnerPawn();
	UBlackboardComponent* BB = GetBlackboard();
	if (!Pawn || !BB)
	{
		return;
	}

	const AActor* Target = Cast<AActor>(BB->GetValueAsObject(WxAIBlackboardKeys::TargetActor));
	if (!Target)
	{
		// 추적 대상이 없으면 인식도 없다. 조사(LastKnown)/복귀(Home)는 BT 가 처리한다.
		SetRecognized(false);
		return;
	}

	// 리시 이탈 판정: 자신(폰)이 배치 지점(HomeLocation)에서 LeashRadius 이상 벗어나면 추적을 끝내고 TargetActor/LastKnown 을 모두 비워 BT 가 복귀(MoveTo HomeLocation)로 떨어지게 한다.
	const float PawnHomeDistSquared = FVector::DistSquared(Pawn->GetActorLocation(), BB->GetValueAsVector(WxAIBlackboardKeys::HomeLocation));
	const bool bExceededLeash = PawnHomeDistSquared > LeashRadius * LeashRadius;
	if (bExceededLeash)
	{
		SetTargetActor(nullptr);
		BB->ClearValue(WxAIBlackboardKeys::TargetLastKnownLocation);
		SetRecognized(false);
		return;
	}

	SetRecognized(true);
}

void UWxAIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LeashTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

APawn* UWxAIPerceptionComponent::GetOwnerPawn() const
{
	const AAIController* AIC = Cast<AAIController>(GetOwner());
	return AIC ? AIC->GetPawn() : nullptr;
}

UBlackboardComponent* UWxAIPerceptionComponent::GetBlackboard() const
{
	if (AAIController* AIC = Cast<AAIController>(GetOwner()))
	{
		return AIC->GetBlackboardComponent();
	}
	return nullptr;
}
