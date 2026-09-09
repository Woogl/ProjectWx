// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxAIBehaviorComponent.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Sight.h"

UWxAIPerceptionComponent::UWxAIPerceptionComponent()
{
	// 감지 거리·각도는 폰의 UWxAIBehaviorComponent 가 정하므로, 여기서는 폰을 가리지 않는 피아 필터와 자극 수명만 잡는다.
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

	// Sight 와 달리 이 둘은 성공 자극만 등록하는 일회성 센스라 해제 이벤트가 없다.
	// MaxAge 를 비워 두면 GetMaxAge 가 NeverHappenedAge 를 돌려줘 자극이 영영 "감지 중" 으로 남으므로, 유한한 수명을 준다.
	HearingConfig->SetMaxAge(5.0f);
	DamageConfig->SetMaxAge(5.0f);

	SetDominantSense(UAISense_Sight::StaticClass());
}

void UWxAIPerceptionComponent::PostInitProperties()
{
	Super::PostInitProperties();

	// 엔진은 여기서 등록한 센스만 OnRegister 에서 퍼셉션 시스템 리스너로 올린다. 셋 다 이 컴포넌트가 항상 갖추는 감각이므로 등록을 외부에 맡기지 않는다.
	if (SightConfig)
	{
		ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
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

	// 배치된 폰은 컨트롤러 BeginPlay 전에 빙의되므로 델리게이트만으로는 첫 폰을 놓친다. 그 사이엔 게임플레이가 돌지 않아 여기서 따라잡으면 된다.
	if (AController* Controller = Cast<AController>(GetOwner()))
	{
		Controller->OnPossessedPawnChanged.AddDynamic(this, &UWxAIPerceptionComponent::HandlePossessedPawnChanged);
		ApplySenseSettings(Controller->GetPawn());
		BindPawnHit(Controller->GetPawn());
	}
}

void UWxAIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindPawnHit();

	if (AController* Controller = Cast<AController>(GetOwner()))
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(this, &UWxAIPerceptionComponent::HandlePossessedPawnChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UWxAIPerceptionComponent::ApplySenseSettings(const APawn* Pawn)
{
	const UWxAIBehaviorComponent* BehaviorComponent = Pawn ? Pawn->FindComponentByClass<UWxAIBehaviorComponent>() : nullptr;
	if (!BehaviorComponent)
	{
		BehaviorComponent = GetDefault<UWxAIBehaviorComponent>();
	}

	SightConfig->SightRadius = BehaviorComponent->GetSightRadius();
	// 반경 경계에서 붙었다 떨어졌다 하는 것은 리시 복귀가 다루므로 시야 상실 반경에 히스테리시스를 두지 않는다.
	SightConfig->LoseSightRadius = BehaviorComponent->GetSightRadius();
	SightConfig->PeripheralVisionAngleDegrees = BehaviorComponent->GetSightAngle();
	HearingConfig->HearingRange = BehaviorComponent->GetHearingRadius();

	// 엔진은 센스 설정이 바뀐 것을 스스로 알아채지 못한다. 리스너를 갱신해야 각 센스가 설정을 다시 읽어 판정용 수치 캐시를 새로 만든다.
	RequestStimuliListenerUpdate();
}

void UWxAIPerceptionComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// 감지 기록은 옛 폰이 모은 것이라 지운다 — 남겨 두면 새 폰이 보고 있는 액터가 이미 감지 상태여서 상태 변화 통지가 나오지 않는다.
	ForgetAll();

	ApplySenseSettings(NewPawn);
	BindPawnHit(NewPawn);
}

void UWxAIPerceptionComponent::HandlePawnHit(FGameplayTag MatchingTag, const FGameplayEventData* Payload)
{
	// 패리 반동은 대미지 없이 같은 이벤트를 쓰므로 자극에서 뺀다.
	if (!Payload || Payload->EventMagnitude <= 0.f)
	{
		return;
	}

	APawn* Pawn = GetOwnerPawn();
	AActor* DamageInstigator = Payload->ContextHandle.GetInstigator();
	if (!Pawn || !DamageInstigator)
	{
		return;
	}

	// Sight·Hearing 과 달리 Damage 센스에는 DetectionByAffiliation 이 없어 엔진이 가해자를 가려 주지 않는다. 여기서 막지 않으면 아군 오사 한 번에 서로를 타겟으로 확정한다.
	if (FGenericTeamId::GetAttitude(Pawn, DamageInstigator) != ETeamAttitude::Hostile)
	{
		return;
	}

	const FHitResult* HitResult = Payload->ContextHandle.GetHitResult();
	const FVector HitLocation = HitResult ? FVector(HitResult->ImpactPoint) : Pawn->GetActorLocation();
	UAISense_Damage::ReportDamageEvent(this, Pawn, DamageInstigator, Payload->EventMagnitude, DamageInstigator->GetActorLocation(), HitLocation);
}

void UWxAIPerceptionComponent::BindPawnHit(APawn* Pawn)
{
	UnbindPawnHit();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC)
	{
		return;
	}

	// 반응 히트는 Event.Hit 자식으로 나가므로 정확 매칭 구독은 놓친다.
	AbilitySystemComponent = ASC;
	PawnHitDelegateHandle = ASC->AddGameplayEventTagContainerDelegate(FGameplayTagContainer(WxGameplayTags::Event_Hit),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UWxAIPerceptionComponent::HandlePawnHit));
}

void UWxAIPerceptionComponent::UnbindPawnHit()
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(WxGameplayTags::Event_Hit), PawnHitDelegateHandle);
	}
	AbilitySystemComponent = nullptr;
	PawnHitDelegateHandle.Reset();
}

APawn* UWxAIPerceptionComponent::GetOwnerPawn() const
{
	const AAIController* AIC = Cast<AAIController>(GetOwner());
	return AIC ? AIC->GetPawn() : nullptr;
}
