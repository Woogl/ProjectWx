// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"

#include "AbilitySystem/Ability/WxAbility_Finisher.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Battle/WxBattleSubsystem.h"
#include "Controller/WxAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Minion/WxMinionComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxAIBehaviorComponent.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"
#include "WxRewardLibrary.h"

AWxEnemyCharacter::AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Enemy;
	AIControllerClass = AWxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	AIBehaviorComponent = CreateDefaultSubobject<UWxAIBehaviorComponent>(TEXT("AIBehaviorComponent"));

	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));

	MinionComponent = CreateDefaultSubobject<UWxMinionComponent>(TEXT("MinionComponent"));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Full);

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

	GetAbilitySystemComponent()->SetLooseGameplayTagCount(WxGameplayTags::State_Engaged, 0);
	if (UWxBattleSubsystem* Battle = UWorld::GetSubsystem<UWxBattleSubsystem>(GetWorld()))
	{
		Battle->NotifyEngagementChanged(this, false);
	}

	Super::EndPlay(EndPlayReason);
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
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_PlayMontageOnce))
	{
		return;
	}

	const bool bFinishable = ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy) || (!ASC->HasMatchingGameplayTag(WxGameplayTags::State_Engaged) && IsInRearCone(Interactor));
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
		UWxRewardLibrary::GrantReward(this, RewardRow, PlayerController, GetActorTransform(), FVector::UpVector * 300.f);
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
	// 죽어도 겨누던 대상은 그대로 남는다 — 그것만 보면 시체가 계속 교전 중으로 남는다.
	const bool bEngaged = IsAlive() && GetLockOnComponent()->GetLockOnTarget() != nullptr;
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(WxGameplayTags::State_Engaged, bEngaged ? 1 : 0);
	if (UWxBattleSubsystem* Battle = UWorld::GetSubsystem<UWxBattleSubsystem>(GetWorld()))
	{
		Battle->NotifyEngagementChanged(this, bEngaged);
	}
}
