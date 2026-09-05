// Copyright Woogle. All Rights Reserved.

#include "Character/WxEnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/Component/WxAIBehaviorComponent.h"
#include "Component/WxNameplateComponent.h"
#include "Controller/WxAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Spawnable/WxSpawner.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxCombatLibrary.h"
#include "WxGameplayTags.h"
#include "WxRewardLibrary.h"

FWxOnBossEngagementChanged AWxEnemyCharacter::OnAnyBossEngagementChanged;

AWxEnemyCharacter::AWxEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Team = EWxTeam::Enemy;
	AIControllerClass = AWxAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	AIBehaviorComponent = CreateDefaultSubobject<UWxAIBehaviorComponent>(TEXT("AIBehaviorComponent"));

	NameplateComponent = CreateDefaultSubobject<UWxNameplateComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetRootComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	LockOnPoint = CreateDefaultSubobject<UWxLockOnPointComponent>(TEXT("LockOnPoint"));
	LockOnPoint->SetupAttachment(GetMesh(), TEXT("pelvis"));
}

void AWxEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Full);
	NameplateComponent->InitializeViewModels(ASC, GetCharacterName(), GetPortrait());

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

	OwningSpawner.Reset();
	GetAbilitySystemComponent()->SetLooseGameplayTagCount(WxGameplayTags::State_Engaged, 0);
	if (bIsBoss)
	{
		OnAnyBossEngagementChanged.Broadcast(this, false);
	}

	Super::EndPlay(EndPlayReason);
}

bool AWxEnemyCharacter::IsBoss() const
{
	return bIsBoss;
}

AWxSpawner* AWxEnemyCharacter::GetOwningSpawner() const
{
	return OwningSpawner.Get();
}

void AWxEnemyCharacter::OnSpawnedBy(AWxSpawner* Spawner)
{
	OwningSpawner = Spawner;
	if (!Spawner)
	{
		return;
	}

	// 정찰 경로를 스포너에 그려 두므로 폰에서 거슬러 올라갈 링크가 필요하다(UWxPatrolComponent::FindPatrolComponent).
	// 부착으로 아웃라이너에서도 소속이 보이지만 이동 복제는 AttachmentReplication 경로를 탄다.
	AttachToActor(Spawner, FAttachmentTransformRules::KeepWorldTransform);
}

bool AWxEnemyCharacter::CanInteract(const AActor* Interactor) const
{
	if (!UWxCombatLibrary::IsHostile(Interactor, this) || !IsAlive())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_PlayMontageOnce))
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		return true;
	}

	return !ASC->HasMatchingGameplayTag(WxGameplayTags::State_Engaged) && IsInRearCone(Interactor);
}

void AWxEnemyCharacter::OnInteracted(AActor* Interactor)
{
	if (!Interactor)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = this;
	EventData.EventTag = WxGameplayTags::Event_Finisher;
	GetAbilitySystemComponent()->GetOwnedGameplayTags(EventData.TargetTags);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, WxGameplayTags::Event_Finisher, EventData);
}

FText AWxEnemyCharacter::GetInteractionPrompt() const
{
	return FText::FromString(TEXT("Finisher"));
}

void AWxEnemyCharacter::HandleAITargetChanged(USceneComponent* NewTarget)
{
	RefreshEngagement();
}

void AWxEnemyCharacter::HandleOwnerDeath(AWxCharacterBase* DeadCharacter)
{
	// 사망 통지는 전 머신에 오므로, 표시에 쓰이는 교전 상태는 권위 검사 앞에서 갱신한다.
	RefreshEngagement();

	if (!HasAuthority())
	{
		return;
	}

	if (AWxSpawner* Spawner = OwningSpawner.Get())
	{
		Spawner->MarkKilled();
	}

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

	if (bIsBoss)
	{
		OnAnyBossEngagementChanged.Broadcast(this, bEngaged);
	}
}
