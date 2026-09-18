// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Minion/WxMinionSubsystem.h"
#include "WxCombatModule.h"
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

	// 빈 요건은 RequirementsMet 가 참이라, 취소 조건은 비었는지부터 본다.
	const bool bWouldBeCanceled = !CancelMasterTagRequirements.IsEmpty() && CancelMasterTagRequirements.RequirementsMet(MasterTags);
	return SummonMasterTagRequirements.RequirementsMet(MasterTags) && !bWouldBeCanceled;
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
	APawn* Master = Minion ? GetMaster(*Minion) : nullptr;
	if (!Master)
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

	// 취소 조건이 어떤 태그를 보든 받도록 주인 태그 변화 전체를 듣는다.
	UAbilitySystemComponent* MasterASC = Minion->HasAuthority() && !CancelMasterTagRequirements.IsEmpty() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master) : nullptr;
	if (MasterASC)
	{
		MasterTagHandle = MasterASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxMinionComponent::HandleMasterTagChanged);
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

	// 죽으면 더 이상 소환물이 아니다. 시체가 취소에 휩쓸려 사망 연출 도중 사라지지 않게 끊는다.
	const APawn* Minion = GetMinionPawn();
	if (UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion ? GetMaster(*Minion) : nullptr))
	{
		MasterASC->RegisterGenericGameplayTagEvent().Remove(MasterTagHandle);
	}
}

void UWxMinionComponent::HandleMasterTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	APawn* Minion = GetMinionPawn();
	APawn* Master = Minion ? GetMaster(*Minion) : nullptr;
	const UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master);
	if (!MasterASC)
	{
		return;
	}

	FGameplayTagContainer MasterTags;
	MasterASC->GetOwnedGameplayTags(MasterTags);
	if (!CancelMasterTagRequirements.RequirementsMet(MasterTags))
	{
		return;
	}

	UE_LOG(LogWxCombat, Log, TEXT("%s: 주인 %s의 %s 태그 변화로 소환 취소 조건을 만족해 사라진다."), *Minion->GetName(), *Master->GetName(), *Tag.ToString());
	Minion->Destroy();
}

void UWxMinionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).Remove(DeathTagHandle);
	}

	const APawn* Minion = GetMinionPawn();
	if (UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion ? GetMaster(*Minion) : nullptr))
	{
		MasterASC->RegisterGenericGameplayTagEvent().Remove(MasterTagHandle);
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
