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
	// 액터 파괴 등 OnUnPossess 를 거치지 않는 종료 경로에서도 사망 콜백을 안전하게 해제한다.
	UnbindOwnerDeath();

	Super::EndPlay(EndPlayReason);
}

void UWxAIPerceptionComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 억제(복귀) 중에는 어떤 감지 자극도 무시한다(복귀 중 재-어그로 방지).
	if (bTargetingSuppressed)
	{
		return;
	}

	// 시각·청각·피해 모두 동일한 획득 경로를 탄다. 감지 성공이면 그 액터(소리 발생원 포함)를 TargetActor 로 확정한다.
	// 감지 실패(시야/소리 상실)에는 TargetActor 를 건드리지 않아 그대로 유지된다 — 실제 해제는 BT 의 리시 복귀(UWxBTTask_ReturnHome → SetTargetingSuppressed)가 담당한다.
	if (Stimulus.WasSuccessfullySensed())
	{
		SetTargetActor(Actor);
	}

	// 인식/추적 판정은 UpdateRecognition 한 곳에서만 한다. 여기서는 TargetActor 만 갱신하고 판정을 위임한다.
	UpdateRecognition();
}

void UWxAIPerceptionComponent::UpdateRecognition()
{
	UBlackboardComponent* BB = GetBlackboard();
	if (!BB)
	{
		return;
	}

	// 억제(복귀) 중이면 인식을 끈 채로 둔다. 리시 이탈 판정·복귀는 BT 로 이관됐다.
	if (bTargetingSuppressed)
	{
		SetRecognized(false);
		return;
	}

	// 죽은 폰은 전투 상태가 아니다. 사망 정리는 HandleDeathTagChanged 가 1회 수행하고, 여기서는 잔여 감지 자극이 인식을 재부여하지 않도록 방어한다.
	// SetRecognized(false) 가 곧 State.InCombat 제거이며, 이 태그를 감시하는 BGMSourceComponent 가 시체 위에서 계속 재생되는 것을 막는다.
	if (const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwnerPawn()))
	{
		if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
		{
			SetRecognized(false);
			return;
		}
	}

	// 추적 대상이 있으면 인식 on, 없으면 off. 복귀(Home)는 BT 가 처리한다.
	SetRecognized(WxBlackboardKeys::GetTargetActor(BB) != nullptr);
}

void UWxAIPerceptionComponent::SetRecognized(bool bNewRecognized)
{
	// 인식 상태를 폰의 ASC 태그로 발행한다.
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

	// 억제를 켜는 순간, 현재 타겟과 인식을 함께 해제한다(회전 모드 원복은 SetTargetActor(nullptr)가 담당).
	// 억제를 끌 때는 상태를 건드리지 않는다 — 다음 감지 자극에서 정상적으로 재획득한다.
	if (bSuppressed)
	{
		SetTargetActor(nullptr);
		SetRecognized(false);
	}
}

void UWxAIPerceptionComponent::BindOwnerDeath()
{
	// 재빙의 등으로 중복 바인드되지 않도록 먼저 정리한다.
	UnbindOwnerDeath();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwnerPawn());
	if (!ASC)
	{
		return;
	}

	DeathBoundASC = ASC;
	DeathTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UWxAIPerceptionComponent::HandleDeathTagChanged);
}

void UWxAIPerceptionComponent::UnbindOwnerDeath()
{
	if (UAbilitySystemComponent* ASC = DeathBoundASC.Get())
	{
		if (DeathTagDelegateHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(DeathTagDelegateHandle, WxGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved);
		}
	}
	DeathBoundASC = nullptr;
	DeathTagDelegateHandle.Reset();
}

void UWxAIPerceptionComponent::HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그 제거(부활)는 무시한다 — 리스폰은 새 액터라 별도 초기화가 필요 없다.
	if (NewCount <= 0)
	{
		return;
	}

	// 사망: 타겟과 인식을 정리해 시체 위에 State.InCombat 이 남지 않게 한다(네임플레이트/BGM 잔존 방지).
	SetTargetActor(nullptr);
	SetRecognized(false);
}

void UWxAIPerceptionComponent::SetTargetActor(AActor* NewTarget)
{
	UBlackboardComponent* BB = GetBlackboard();
	if (!BB)
	{
		return;
	}

	if (WxBlackboardKeys::GetTargetActor(BB) == NewTarget)
	{
		return;
	}

	WxBlackboardKeys::SetTargetActor(BB, NewTarget);

	// 타겟 유무에 따라 회전 모드를 발행한다(상태는 원천이 발행). 
	// 타겟이 있으면 그 액터를 포커스로 두고 bUseControllerDesiredRotation 으로 전환해 타겟을 바라본 채 이동(strafe).
	// 타겟이 없으면 이동 방향으로 회전하는 평상시 로코모션으로 되돌린다.
	AAIController* AIC = Cast<AAIController>(GetOwner());
	ACharacter* Character = AIC ? Cast<ACharacter>(AIC->GetPawn()) : nullptr;
	if (!Character)
	{
		return;
	}

	// 포커스는 MovementComponent 가 없어도 발행하고, 회전 모드 플래그 쓰기만 Movement 유효할 때로 가드한다(Patrol 과 동일한 방어).
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
