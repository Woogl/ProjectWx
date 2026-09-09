// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxAIBehaviorComponent.h"
#include "WxBlackboardKeys.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
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
	OnTargetPerceptionUpdated.AddDynamic(this, &UWxAIPerceptionComponent::HandleTargetPerceptionUpdated);
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
	// 타겟이 살아 있는 채로 컨트롤러째 사라지는 종료 경로에서도 구독을 안전하게 해제한다.
	UnbindTargetLoss();
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

void UWxAIPerceptionComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 죽은 액터는 잡지 않는다 — 시체는 파괴되지 않고 남아 시야에 다시 들어오면 성공 자극을 또 만들므로, 이 가드가 없으면 사망 정리가 다음 자극에 되돌려진다.
	if (Stimulus.WasSuccessfullySensed() && !IsActorDead(Actor) && !AppliedTarget.ResolveObjectPtr())
	{
		SetTargetActor(Actor);
	}
}

void UWxAIPerceptionComponent::ForgetTargetActor()
{
	if (AActor* Target = AppliedTarget.ResolveObjectPtr())
	{
		ForgetActor(Target);
	}

	SetTargetActor(nullptr);
}
void UWxAIPerceptionComponent::HandleTargetDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그 제거(부활)는 무시한다 — 사망 시점에 이 구독까지 함께 해제되므로 여기로 오지 않는다.
	if (NewCount <= 0)
	{
		return;
	}

	SetTargetActor(FindPerceivedTarget());
}

void UWxAIPerceptionComponent::HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	// 엔진은 액터를 무효화하기 전에 EndPlay 를 방송하므로, 이 시점엔 그 액터가 아직 유효하다.
	// 사라지는 액터는 아직 감지 목록에 남아 자기 자신이 다시 뽑히므로, 승계 전에 기록을 지운다.
	ForgetActor(Actor);

	SetTargetActor(FindPerceivedTarget());
}

void UWxAIPerceptionComponent::BindTargetLoss(AActor* NewTarget)
{
	UnbindTargetLoss();

	if (!NewTarget)
	{
		return;
	}

	AppliedTarget = NewTarget;

	NewTarget->OnEndPlay.AddDynamic(this, &UWxAIPerceptionComponent::HandleTargetEndPlay);

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewTarget))
	{
		TargetDeathTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWxAIPerceptionComponent::HandleTargetDeathTagChanged);
	}
}

void UWxAIPerceptionComponent::UnbindTargetLoss()
{
	// 대상이 이미 사라졌으면 그 구독도 함께 사라졌으므로 해제할 것이 없다.
	if (AActor* Target = AppliedTarget.ResolveObjectPtr())
	{
		Target->OnEndPlay.RemoveDynamic(this, &UWxAIPerceptionComponent::HandleTargetEndPlay);

		if (TargetDeathTagDelegateHandle.IsValid())
		{
			if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
			{
				ASC->UnregisterGameplayTagEvent(TargetDeathTagDelegateHandle, WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved);
			}
		}
	}
	AppliedTarget = nullptr;
	TargetDeathTagDelegateHandle.Reset();
}

void UWxAIPerceptionComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// 타겟과 소실 구독은 폰이 아니라 컨트롤러에 남는다. 새 폰이 이전 타겟을 물려받지 않도록 먼저 되돌린다.
	SetTargetActor(nullptr);

	// 감지 기록도 옛 폰이 모은 것이라 함께 지운다 — 남겨 두면 새 폰이 보고 있는 액터가 이미 감지 상태여서 상태 변화 통지가 나오지 않는다.
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

AActor* UWxAIPerceptionComponent::FindPerceivedTarget()
{
	TArray<AActor*> PerceivedActors;
	GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	// 세 센스 모두 적대만 등록하므로 피아는 다시 가르지 않는다.
	// 자기 폰은 여기서 걸러야 한다 — SetTargetActor 의 자기 폰 가드는 조기 return 이라, 후보로 뽑히면 잃은 타겟이 블랙보드에 그대로 남는다.
	const APawn* OwnerPawn = GetOwnerPawn();
	for (AActor* PerceivedActor : PerceivedActors)
	{
		if (PerceivedActor != OwnerPawn && !IsActorDead(PerceivedActor))
		{
			return PerceivedActor;
		}
	}

	return nullptr;
}

void UWxAIPerceptionComponent::SetTargetActor(AActor* NewTarget)
{
	// 엔진 청각은 소리를 낸 본인의 리스너를 제외하지 않아, 자기 발소리가 그대로 자기 자극으로 돌아온다.
	// 유효한 타겟일 때만 가른다 — 폰이 없는 순간의 해제 요청(nullptr)이 여기 걸리면 타겟이 영영 비워지지 않는다.
	if (NewTarget && NewTarget == GetOwnerPawn())
	{
		return;
	}

	// 블랙보드가 없으면 구독까지 통째로 건너뛴다 — 적용 기록만 남기면 다음 자극이 중복으로 걸러져 블랙보드가 영영 빈 채로 어긋난다.
	UBlackboardComponent* BB = GetBlackboard();
	if (!BB)
	{
		return;
	}

	// 블랙보드 Object 키는 약참조라 타겟이 파괴되면 이 컴포넌트 모르게 비워지고, 그걸 기준으로 삼으면 뒤늦은 해제 요청이 "이미 비어 있다" 로 걸러져 소실 구독이 남는다.
	if (AppliedTarget == TObjectKey<AActor>(NewTarget))
	{
		return;
	}

	WxBlackboardKeys::SetTargetActor(BB, NewTarget);

	BindTargetLoss(NewTarget);
	OnTargetChanged.Broadcast(NewTarget);
}

bool UWxAIPerceptionComponent::IsActorDead(AActor* Actor)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	return ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
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
