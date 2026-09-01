// Copyright Woogle. All Rights Reserved.

#include "WxAIPerceptionComponent.h"
#include "WxBlackboardKeys.h"
#include "WxGameplayTags.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	// 시야 상실 반경을 감지 반경과 같게 둔다 — 반경 경계에서 붙었다 떨어졌다 하는 것은 리시 복귀가 다루므로 히스테리시스를 두지 않는다.
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1500.0f;
	// 정면 기준 편측 시야각(도). 전체 시야각은 이 값의 2배다.
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	HearingConfig->HearingRange = 1000.0f;

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
	// 태그 제거(부활)는 무시한다 — 다음 감지 자극에서 정상적으로 재획득한다.
	if (NewCount <= 0)
	{
		return;
	}

	SetTargetActor(nullptr);
}

void UWxAIPerceptionComponent::HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	// 엔진은 액터를 무효화하기 전에 EndPlay 를 방송하므로, 이 시점엔 BB 의 약참조가 아직 살아 있어 타겟·포커스·회전 모드가 정상적으로 원복된다.
	SetTargetActor(nullptr);
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
	// 타겟·포커스·소실 구독은 폰이 아니라 컨트롤러에 남는다. 새 폰이 이전 타겟을 물려받지 않도록 먼저 되돌린다.
	SetTargetActor(nullptr);

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

	// 반응 히트는 Event.Hit 자식으로 나가므로 정확 매칭 구독은 놓친다. 컨테이너 델리게이트로 부모 매칭한다.
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

void UWxAIPerceptionComponent::SetTargetActor(AActor* NewTarget)
{
	// 엔진 청각은 소리를 낸 본인의 리스너를 제외하지 않아, 자기 발소리가 그대로 자기 자극으로 돌아온다.
	// 유효한 타겟일 때만 가른다 — 폰이 없는 순간의 해제 요청(nullptr)이 여기 걸리면 포커스가 영영 풀리지 않는다.
	if (NewTarget && NewTarget == GetOwnerPawn())
	{
		return;
	}

	UBlackboardComponent* BB = GetBlackboard();
	if (!BB)
	{
		return;
	}

	// 블랙보드 값이 아니라 적용 기록과 비교한다. 블랙보드 Object 키는 약참조라 타겟이 파괴되면 이 컴포넌트 모르게 비워지고, 그걸 기준으로 삼으면 뒤늦은 해제 요청이 "이미 비어 있다" 로 걸러져 포커스·회전 모드가 strafe 에 고착된다.
	if (AppliedTarget == TObjectKey<AActor>(NewTarget))
	{
		return;
	}

	WxBlackboardKeys::SetTargetActor(BB, NewTarget);

	BindTargetLoss(NewTarget);
	OnTargetChanged.Broadcast(NewTarget);

	AAIController* AIC = Cast<AAIController>(GetOwner());
	ACharacter* Character = AIC ? Cast<ACharacter>(AIC->GetPawn()) : nullptr;
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (NewTarget)
	{
		AIC->SetFocus(NewTarget);
		if (Movement)
		{
			Movement->bOrientRotationToMovement = false;
			Movement->bUseControllerDesiredRotation = true;
		}
	}
	else
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);

		// 평상시 회전 모드는 폰마다 다를 수 있으므로, 상수 대신 컴포넌트 아키타입(폰 BP·C++ 생성자 기본값)에서 읽어 되돌린다.
		const UCharacterMovementComponent* MovementDefaults = Movement ? Cast<UCharacterMovementComponent>(Movement->GetArchetype()) : nullptr;
		if (MovementDefaults)
		{
			Movement->bUseControllerDesiredRotation = MovementDefaults->bUseControllerDesiredRotation;
			Movement->bOrientRotationToMovement = MovementDefaults->bOrientRotationToMovement;
		}
	}
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
