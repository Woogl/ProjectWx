// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AnimNotify/WxAnimNotify_SpawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "NavigationSystem.h"
#include "WxGameplayTags.h"

void UWxMinionComponent::BeginPlay()
{
	Super::BeginPlay();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	AbilitySystemComponent = ASC;
	SpawnMinionEventHandle = ASC->GenericGameplayEventCallbacks
		.FindOrAdd(WxGameplayTags::Event_SpawnMinion)
		.AddUObject(this, &UWxMinionComponent::HandleSpawnMinionEvent);
}

void UWxMinionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (FGameplayEventMulticastDelegate* EventDelegate = ASC->GenericGameplayEventCallbacks.Find(WxGameplayTags::Event_SpawnMinion))
		{
			EventDelegate->Remove(SpawnMinionEventHandle);
		}
	}

	AbilitySystemComponent.Reset();
	SpawnMinionEventHandle.Reset();

	for (const TWeakObjectPtr<AActor>& ActiveMinion : ActiveMinions)
	{
		if (AActor* Minion = ActiveMinion.Get())
		{
			Minion->Destroy();
		}
	}

	ActiveMinions.Reset();

	Super::EndPlay(EndPlayReason);
}

void UWxMinionComponent::HandleSpawnMinionEvent(const FGameplayEventData* Payload)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Payload)
	{
		return;
	}

	const UWxAnimNotify_SpawnMinion* MinionNotify = Cast<UWxAnimNotify_SpawnMinion>(Payload->OptionalObject.Get());
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Payload->OptionalObject2.Get());
	if (!MinionNotify || !Mesh || Mesh->GetOwner() != Owner || !MinionNotify->GetMinionClass())
	{
		return;
	}

	PruneInactiveMinions();

	while (!ActiveMinions.IsEmpty() && ActiveMinions.Num() >= MaxMinionCount)
	{
		if (AActor* OldestMinion = ActiveMinions[0].Get())
		{
			OldestMinion->Destroy();
		}

		ActiveMinions.RemoveAt(0);
	}

	FVector SpawnLocation = Owner->GetActorTransform().TransformPosition(MinionNotify->GetSpawnOffset());

	// 내비메시 밖에 세우면 AI가 한 발짝도 못 움직인다.
	if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(Owner->GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(SpawnLocation, ProjectedLocation))
		{
			SpawnLocation = ProjectedLocation.Location;
		}
	}

	const FTransform SpawnTransform(Owner->GetActorRotation(), SpawnLocation);

	// 팀은 BeginPlay 전에 심어야 최초 복제값부터 옳고 첫 프레임의 인지·판정이 어긋나지 않는다.
	AActor* Minion = Owner->GetWorld()->SpawnActorDeferred<AActor>(
		MinionNotify->GetMinionClass(), SpawnTransform, Owner, Cast<APawn>(Owner),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Minion)
	{
		return;
	}

	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Minion))
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(Owner));
	}

	Minion->FinishSpawning(SpawnTransform);

	if (MinionNotify->GetLifetime() > 0.f)
	{
		Minion->SetLifeSpan(MinionNotify->GetLifetime());
	}

	ActiveMinions.Add(Minion);
}

void UWxMinionComponent::PruneInactiveMinions()
{
	for (int32 MinionIndex = ActiveMinions.Num() - 1; MinionIndex >= 0; --MinionIndex)
	{
		AActor* Minion = ActiveMinions[MinionIndex].Get();
		if (!Minion)
		{
			ActiveMinions.RemoveAt(MinionIndex);
			continue;
		}

		const UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion);
		if (MinionASC && MinionASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
		{
			ActiveMinions.RemoveAt(MinionIndex);
		}
	}
}
