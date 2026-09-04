// Copyright Woogle. All Rights Reserved.

#include "Character/Component/WxEnemyComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/WxCharacterBase.h"
#include "Component/WxNameplateComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Spawnable/WxSpawner.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "WxCombatLibrary.h"
#include "WxGame.h"
#include "WxGameplayTags.h"
#include "WxRewardLibrary.h"

FWxOnBossReady UWxEnemyComponent::OnAnyBossReady;

void UWxEnemyComponent::BeginPlay()
{
	Super::BeginPlay();

	AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy)
	{
		UE_LOG(LogWxGame, Warning, TEXT("%s: WxEnemyComponent 는 WxCharacterBase 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
		return;
	}

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	NameplateComponent = Enemy->FindComponentByClass<UWxNameplateComponent>();
	if (NameplateComponent)
	{
		NameplateComponent->InitializeViewModels(ASC, Enemy->GetCharacterName(), Enemy->GetPortrait());
	}

	Enemy->GetLockOnComponent()->OnLockOnTargetChanged.AddDynamic(this, &ThisClass::HandleAITargetChanged);
	Enemy->OnDeath.AddDynamic(this, &ThisClass::HandleOwnerDeath);

	TArray<UWxLockOnPointComponent*> LockOnPoints;
	Enemy->GetComponents<UWxLockOnPointComponent>(LockOnPoints);
	for (UWxLockOnPointComponent* Point : LockOnPoints)
	{
		Point->OnLockedOnChanged.AddUObject(this, &ThisClass::HandleLockedOnChanged);
	}

	const bool bDead = ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
	SetEngaged(!bDead && Enemy->HasCombatTarget());
	RefreshNameplateVisibility();
	if (bDead)
	{
		HandleOwnerDeath(Enemy);
	}

	if (IsBoss())
	{
		OnAnyBossReady.Broadcast(this);
	}
}

void UWxEnemyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AWxCharacterBase* Enemy = GetEnemyCharacter())
	{
		Enemy->GetLockOnComponent()->OnLockOnTargetChanged.RemoveDynamic(this, &ThisClass::HandleAITargetChanged);
		Enemy->OnDeath.RemoveDynamic(this, &ThisClass::HandleOwnerDeath);

		TArray<UWxLockOnPointComponent*> LockOnPoints;
		Enemy->GetComponents<UWxLockOnPointComponent>(LockOnPoints);
		for (UWxLockOnPointComponent* Point : LockOnPoints)
		{
			Point->OnLockedOnChanged.RemoveAll(this);
		}
	}

	NameplateComponent = nullptr;
	OwningSpawner.Reset();
	SetEngaged(false);
	if (IsBoss())
	{
		OnBossEndPlay.Broadcast(this);
	}

	Super::EndPlay(EndPlayReason);
}

AWxCharacterBase* UWxEnemyComponent::GetEnemyCharacter() const
{
	return GetOwner<AWxCharacterBase>();
}

AWxSpawner* UWxEnemyComponent::GetOwningSpawner() const
{
	return OwningSpawner.Get();
}

EWxEnemyRank UWxEnemyComponent::GetEnemyRank() const
{
	return EnemyRank;
}

bool UWxEnemyComponent::IsBoss() const
{
	return EnemyRank == EWxEnemyRank::Boss;
}

bool UWxEnemyComponent::IsEngaged() const
{
	return bEngaged;
}

void UWxEnemyComponent::HandleSpawnedBy(AWxSpawner* Spawner)
{
	OwningSpawner = Spawner;

	AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy || !Spawner)
	{
		return;
	}

	// 정찰 경로를 스포너에 그려 두므로 폰에서 거슬러 올라갈 링크가 필요하다(UWxPatrolComponent::FindPatrolComponent).
	// 부착으로 아웃라이너에서도 소속이 보이지만 이동 복제는 AttachmentReplication 경로를 탄다.
	Enemy->AttachToActor(Spawner, FAttachmentTransformRules::KeepWorldTransform);
}

bool UWxEnemyComponent::CanInteract(const AActor* Interactor) const
{
	const AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy || !UWxCombatLibrary::IsHostile(Interactor, Enemy) || !Enemy->IsAlive())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_PlayMontageOnce))
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Groggy))
	{
		return true;
	}

	return !Enemy->HasCombatTarget() && IsInRearCone(Interactor);
}

void UWxEnemyComponent::Interact(AActor* Interactor)
{
	AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy || !Interactor)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = Enemy;
	EventData.EventTag = WxGameplayTags::Event_Finisher;
	Enemy->GetAbilitySystemComponent()->GetOwnedGameplayTags(EventData.TargetTags);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, WxGameplayTags::Event_Finisher, EventData);
}

FText UWxEnemyComponent::GetInteractionPrompt() const
{
	return FText::FromString(TEXT("Finisher"));
}

void UWxEnemyComponent::HandleAITargetChanged(USceneComponent* NewTarget)
{
	const AWxCharacterBase* Enemy = GetEnemyCharacter();
	const bool bDead = !Enemy || Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
	SetEngaged(!bDead && NewTarget != nullptr);
	RefreshNameplateVisibility();
}

void UWxEnemyComponent::HandleOwnerDeath(AWxCharacterBase* DeadCharacter)
{
	SetEngaged(false);
	RefreshNameplateVisibility();

	if (bDeathHandled)
	{
		return;
	}

	bDeathHandled = true;

	AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy || !Enemy->HasAuthority())
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
		UWxRewardLibrary::GrantReward(Enemy, RewardRow, PlayerController, Enemy->GetActorTransform(), FVector::UpVector * LaunchSpeed);
	}
}

void UWxEnemyComponent::HandleLockedOnChanged(bool bLockedOn)
{
	RefreshNameplateVisibility();
}

void UWxEnemyComponent::RefreshNameplateVisibility()
{
	AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy || !NameplateComponent)
	{
		return;
	}

	const bool bDead = Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
	const bool bShow = Enemy->HasCombatTarget() || UWxLockOnPointComponent::IsActorLockedOn(Enemy);
	NameplateComponent->SetVisibility(!bDead && bShow);
}

bool UWxEnemyComponent::IsInRearCone(const AActor* Interactor) const
{
	const AWxCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy || !Interactor)
	{
		return false;
	}

	FVector ToInteractor = Interactor->GetActorLocation() - Enemy->GetActorLocation();
	ToInteractor.Z = 0.0;
	if (!ToInteractor.Normalize())
	{
		return false;
	}

	const float ForwardDot = FVector::DotProduct(Enemy->GetActorForwardVector(), ToInteractor);
	const float RearThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabRearHalfAngle));
	return ForwardDot <= RearThreshold;
}

void UWxEnemyComponent::SetEngaged(bool bInEngaged)
{
	if (bEngaged == bInEngaged)
	{
		return;
	}

	bEngaged = bInEngaged;
	OnEngagementChanged.Broadcast(bEngaged);
}
