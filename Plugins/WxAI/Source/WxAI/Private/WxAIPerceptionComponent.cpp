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
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"

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

void UWxAIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타겟이 살아 있는 채로 컨트롤러째 사라지는 종료 경로에서도 구독을 안전하게 해제한다.
	UnbindTargetLoss();

	Super::EndPlay(EndPlayReason);
}

void UWxAIPerceptionComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (bTargetingSuppressed)
	{
		return;
	}

	// 죽은 액터는 잡지 않는다 — 시체는 파괴되지 않고 남아 시야에 다시 들어오면 성공 자극을 또 만들므로, 이 가드가 없으면 사망 정리가 다음 자극에 되돌려진다.
	// 감지 실패(시야/소리 상실)에는 TargetActor 를 건드리지 않아 그대로 유지된다 — 실제 해제는 BT 의 리시 복귀(UWxBTTask_ReturnHome → SetTargetingSuppressed)가 담당한다.
	if (Stimulus.WasSuccessfullySensed() && !IsActorDead(Actor))
	{
		SetTargetActor(Actor);
	}

	UpdateRecognition();
}

void UWxAIPerceptionComponent::UpdateRecognition()
{
	if (bTargetingSuppressed)
	{
		SetRecognized(false);
		return;
	}

	// 타겟은 살아있는 액터만 담기므로(획득 가드 + 소실 구독) 유무만 봐도 된다.
	// 블랙보드가 없으면 추적 대상도 없는 것이므로 off 로 본다.
	const UBlackboardComponent* BB = GetBlackboard();
	SetRecognized(BB && WxBlackboardKeys::GetTargetActor(BB) != nullptr);
}

void UWxAIPerceptionComponent::SetRecognized(bool bNewRecognized)
{
	// MinimalReplication 태그는 GE 없이 서버→클라이언트로 복제(COND_SkipOwner)되어, 각 클라이언트의 네임플레이트가 이 태그를 읽어 표시를 결정한다.
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwnerPawn());
	if (!ASC)
	{
		return;
	}
	
	const bool bCurrentlyRecognized = ASC->HasMatchingGameplayTag(WxGameplayTags::State_InCombat);
	if (bNewRecognized == bCurrentlyRecognized)
	{
		return;
	}

	if (bNewRecognized)
	{
		ASC->AddMinimalReplicationGameplayTag(WxGameplayTags::State_InCombat);
	}
	else
	{
		ASC->RemoveMinimalReplicationGameplayTag(WxGameplayTags::State_InCombat);
	}
}

void UWxAIPerceptionComponent::SetTargetingSuppressed(bool bSuppressed)
{
	if (bTargetingSuppressed == bSuppressed)
	{
		return;
	}

	bTargetingSuppressed = bSuppressed;

	// 인식은 판정에 맡기면 억제 중이라 자연히 꺼진다.
	if (bSuppressed)
	{
		SetTargetActor(nullptr);
		UpdateRecognition();
	}
}

void UWxAIPerceptionComponent::HandleTargetDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그 제거(부활)는 무시한다 — 다음 감지 자극에서 정상적으로 재획득한다.
	if (NewCount <= 0)
	{
		return;
	}

	SetTargetActor(nullptr);
	UpdateRecognition();
}

void UWxAIPerceptionComponent::HandleTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	// 엔진은 액터를 무효화하기 전에 EndPlay 를 방송하므로, 이 시점엔 BB 의 약참조가 아직 살아 있어 타겟·포커스·회전 모드가 정상적으로 원복된다.
	SetTargetActor(nullptr);
	UpdateRecognition();
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

void UWxAIPerceptionComponent::SetTargetActor(AActor* NewTarget)
{
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
		if (Movement)
		{
			Movement->bUseControllerDesiredRotation = false;
			Movement->bOrientRotationToMovement = true;
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
