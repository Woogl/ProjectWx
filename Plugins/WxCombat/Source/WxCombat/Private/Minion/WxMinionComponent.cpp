// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Minion/WxMinionSubsystem.h"
#include "WxGameplayTags.h"

UWxMinionComponent::UWxMinionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MasterStateTag = WxGameplayTags::State_MinionMaster_Minion;
}

APawn* UWxMinionComponent::GetMaster(const APawn& Minion)
{
	APawn* SpawnInstigator = Minion.GetInstigator();
	return SpawnInstigator != &Minion ? SpawnInstigator : nullptr;
}

APawn* UWxMinionComponent::GetMinionPawn() const
{
	return Cast<APawn>(GetOwner());
}

bool UWxMinionComponent::CanBeSummonedBy(APawn& Master) const
{
	FGameplayTagContainer MasterTags;
	if (const UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Master))
	{
		MasterASC->GetOwnedGameplayTags(MasterTags);
	}

	return MasterTagRequirements.RequirementsMet(MasterTags);
}

int32 UWxMinionComponent::GetMaxCountPerMaster() const
{
	return FMath::Max(0, MaxCountPerMaster);
}

FGameplayTag UWxMinionComponent::GetMasterStateTag() const
{
	return MasterStateTag;
}

void UWxMinionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 소환된 적 없는 적은 Instigator 가 자기 자신이라 주인이 없다. 복제 스폰도 이 시점엔 Instigator 가 들어와 있다.
	const APawn* Minion = GetMinionPawn();
	if (!Minion || !GetMaster(*Minion))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (UWxMinionSubsystem* MinionSubsystem = World ? World->GetSubsystem<UWxMinionSubsystem>() : nullptr)
	{
		MinionSubsystem->RegisterMinion(*this);
	}

	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		DeathTagHandle = MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).AddUObject(this, &UWxMinionComponent::HandleDeathTagChanged);
	}
}

void UWxMinionComponent::HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그가 내려가는 통지도 같은 델리게이트로 오므로 붙는 순간만 고른다.
	if (NewCount <= 0)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (UWxMinionSubsystem* MinionSubsystem = World ? World->GetSubsystem<UWxMinionSubsystem>() : nullptr)
	{
		MinionSubsystem->UnregisterMinion(*this);
	}
}

void UWxMinionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).Remove(DeathTagHandle);
	}

	if (const UWorld* World = GetWorld())
	{
		if (UWxMinionSubsystem* MinionSubsystem = World->GetSubsystem<UWxMinionSubsystem>())
		{
			MinionSubsystem->UnregisterMinion(*this);
		}
	}

	Super::EndPlay(EndPlayReason);
}
