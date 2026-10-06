// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"

#include "AbilitySystem/Abilities/WxAbility_Finisher.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/WxBattleSubsystem.h"
#include "AI/WxAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Minion/WxMinionComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "AI/WxAIBehaviorComponent.h"
#include "Combat/WxCombatLibrary.h"
#include "WxGameplayTags.h"
#include "Inventory/WxRewardLibrary.h"

AWxEnemyCharacter::AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Enemy;
	AIControllerClass = AWxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	GetAbilitySystemComponent()->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AIBehaviorComponent = CreateDefaultSubobject<UWxAIBehaviorComponent>(TEXT("AIBehaviorComponent"));

	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));

	MinionComponent = CreateDefaultSubobject<UWxMinionComponent>(TEXT("MinionComponent"));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	GetLockOnComponent()->OnLockOnTargetChanged.AddDynamic(this, &ThisClass::HandleAITargetChanged);
	OnDeath.AddDynamic(this, &ThisClass::HandleOwnerDeath);

	const bool bDead = ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
	RefreshEngagement();
	if (bDead)
	{
		HandleOwnerDeath(this);
	}
}

void AWxEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetLockOnComponent()->OnLockOnTargetChanged.RemoveDynamic(this, &ThisClass::HandleAITargetChanged);
	OnDeath.RemoveDynamic(this, &ThisClass::HandleOwnerDeath);

	if (UWxBattleSubsystem* Battle = UWorld::GetSubsystem<UWxBattleSubsystem>(GetWorld()))
	{
		Battle->NotifyEngagementChanged(this, false);
	}

	Super::EndPlay(EndPlayReason);
}

bool AWxEnemyCharacter::IsEngaged() const
{
	return IsAlive() && GetLockOnComponent()->GetLockOnTarget() != nullptr;
}

FWxOnSpawnableKilled& AWxEnemyCharacter::GetOnKilledDelegate()
{
	return OnSpawnableKilled;
}

void AWxEnemyCharacter::GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const
{
	if (!UWxCombatLibrary::IsHostile(Interactor, this) || !IsAlive())
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_FinisherReserved)
		|| ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_PlayMontageOnce))
	{
		return;
	}

	const bool bFinishable = ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy) || (!IsEngaged() && IsInRearCone(Interactor));
	if (!bFinishable)
	{
		return;
	}

	// 문구의 주인은 실제로 나갈 처형 어빌리티다. 그 어빌리티가 없는 상호작용자에겐 눌러도 나갈 것이 없으니 선택지를 내지 않는다.
	const UAbilitySystemComponent* InteractorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Interactor);
	if (!InteractorASC)
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : InteractorASC->GetActivatableAbilities())
	{
		if (const UWxAbility_Finisher* Finisher = Cast<UWxAbility_Finisher>(Spec.Ability.Get()))
		{
			OutOptions.Add({Finisher->InteractionPrompt});
			return;
		}
	}
}

void AWxEnemyCharacter::OnInteracted(AActor* Interactor, int32 OptionValue)
{
	if (!Interactor)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = WxGameplayTags::Event_Finisher;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, WxGameplayTags::Event_Finisher, EventData);
}

void AWxEnemyCharacter::HandleAITargetChanged(USceneComponent* NewTarget)
{
	RefreshEngagement();
}

void AWxEnemyCharacter::HandleOwnerDeath(AWxCharacterBase* DeadCharacter)
{
	// 사망 통지는 전 머신에 오므로, 표시에 쓰이는 교전 상태는 권위 검사 앞에서 갱신한다.
	RefreshEngagement();

	if (!HasAuthority() || bDeathNotified)
	{
		return;
	}
	// BeginPlay의 이미 죽은 캐릭터 처리도 이 경로로 오므로 처치 통지는 이 객체에서 한 번만 보낸다.
	bDeathNotified = true;

	OnSpawnableKilled.Broadcast();

	// 처치자를 가리지 않고 항상 0번 플레이어에게 지급하는 것이 기존 정책이다.
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UWxRewardLibrary::GrantReward(this, RewardRow, PlayerController, GetActorTransform());
	}
}

bool AWxEnemyCharacter::IsInRearCone(const AActor* Interactor) const
{
	if (!Interactor)
	{
		return false;
	}

	FVector ToInteractor = Interactor->GetActorLocation() - GetActorLocation();
	ToInteractor.Z = 0.0;
	if (!ToInteractor.Normalize())
	{
		return false;
	}

	const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), ToInteractor);
	const float RearThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabRearHalfAngle));
	return ForwardDot <= RearThreshold;
}

void AWxEnemyCharacter::RefreshEngagement()
{
	if (UWxBattleSubsystem* Battle = UWorld::GetSubsystem<UWxBattleSubsystem>(GetWorld()))
	{
		Battle->NotifyEngagementChanged(this, IsEngaged());
	}
}
