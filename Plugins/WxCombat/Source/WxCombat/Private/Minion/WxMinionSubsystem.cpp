// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "WxCombatModule.h"
#include "WxGameplayTags.h"

APawn* UWxMinionSubsystem::SpawnMinion(AActor& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform, int32 MaxMinionCount)
{
	if (!Master.HasAuthority() || !MinionClass || MaxMinionCount <= 0)
	{
		return nullptr;
	}
	if (!MinionClass->ImplementsInterface(UGenericTeamAgentInterface::StaticClass()))
	{
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 소환 클래스 %s가 GenericTeamAgentInterface를 구현하지 않아 생성하지 않는다."), *Master.GetName(), *GetNameSafe(MinionClass.Get()));
		return nullptr;
	}
	if (IsMinion(Master))
	{
		return nullptr;
	}

	if (!Rosters.Contains(&Master))
	{
		Master.OnEndPlay.AddDynamic(this, &UWxMinionSubsystem::HandleMasterEndPlay);
	}

	TArray<TWeakObjectPtr<APawn>>& Minions = Rosters.FindOrAdd(&Master);
	RemoveInvalidOrDeadMinions(Minions);

	// 새 소환물 한 자리를 확보하되, 상한이 낮아진 경우 초과분도 함께 정리한다.
	const int32 MinionCountToRemove = FMath::Clamp(Minions.Num() - MaxMinionCount + 1, 0, Minions.Num());
	for (int32 RemovedMinionCount = 0; RemovedMinionCount < MinionCountToRemove; ++RemovedMinionCount)
	{
		if (APawn* OldestMinion = Minions[0].Get())
		{
			OldestMinion->Destroy();
		}

		Minions.RemoveAt(0);
	}

	// 팀은 BeginPlay 전에 심어야 최초 복제값부터 옳고 첫 프레임의 인지·판정이 어긋나지 않는다.
	APawn* Minion = GetWorld()->SpawnActorDeferred<APawn>(MinionClass, SpawnTransform, nullptr, Cast<APawn>(&Master), ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Minion)
	{
		return nullptr;
	}

	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Minion))
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(&Master));
	}

	Minion->FinishSpawning(SpawnTransform);

	Minions.Add(Minion);

	return Minion;
}

int32 UWxMinionSubsystem::TryActivateAbilityOnMinions(AActor& Master, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload)
{
	if (!Master.HasAuthority() || !AbilityTag.IsValid())
	{
		return 0;
	}

	TArray<TWeakObjectPtr<APawn>>* Minions = Rosters.Find(&Master);
	if (!Minions)
	{
		return 0;
	}

	RemoveInvalidOrDeadMinions(*Minions);

	FGameplayEventData CommandPayload = Payload;
	if (!CommandPayload.Instigator)
	{
		CommandPayload.Instigator = &Master;
	}

	// 발동한 어빌리티가 액터 수명이나 로스터를 바꾸더라도 이번 명령의 대상 집합은 유지한다.
	const TArray<TWeakObjectPtr<APawn>> CommandTargets = *Minions;
	int32 ActivatedMinionCount = 0;

	for (const TWeakObjectPtr<APawn>& CommandTarget : CommandTargets)
	{
		APawn* Minion = CommandTarget.Get();
		if (!Minion)
		{
			continue;
		}

		UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion);
		if (!MinionASC)
		{
			continue;
		}

		if (!TryActivateAbilityByExactTag(*MinionASC, AbilityTag, CommandPayload))
		{
			continue;
		}

		++ActivatedMinionCount;
	}

	return ActivatedMinionCount;
}

bool UWxMinionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UWxMinionSubsystem::HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	TArray<TWeakObjectPtr<APawn>> Minions;
	if (!Rosters.RemoveAndCopyValue(Actor, Minions))
	{
		return;
	}

	for (const TWeakObjectPtr<APawn>& ActiveMinion : Minions)
	{
		if (APawn* Minion = ActiveMinion.Get())
		{
			Minion->Destroy();
		}
	}
}

bool UWxMinionSubsystem::IsMinion(const AActor& Actor) const
{
	for (const TPair<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<APawn>>>& Roster : Rosters)
	{
		for (const TWeakObjectPtr<APawn>& Minion : Roster.Value)
		{
			if (Minion.Get() == &Actor)
			{
				return true;
			}
		}
	}

	return false;
}

void UWxMinionSubsystem::RemoveInvalidOrDeadMinions(TArray<TWeakObjectPtr<APawn>>& Minions) const
{
	for (int32 MinionIndex = Minions.Num() - 1; MinionIndex >= 0; --MinionIndex)
	{
		APawn* Minion = Minions[MinionIndex].Get();
		if (!Minion)
		{
			Minions.RemoveAt(MinionIndex);
			continue;
		}

		const UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion);
		if (!MinionASC || !MinionASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
		{
			continue;
		}

		Minions.RemoveAt(MinionIndex);
	}
}

bool UWxMinionSubsystem::TryActivateAbilityByExactTag(UAbilitySystemComponent& MinionASC, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload) const
{
	FGameplayAbilityActorInfo* ActorInfo = MinionASC.AbilityActorInfo.Get();
	if (!ActorInfo)
	{
		return false;
	}

	// 발동과 실패 통지가 부여 목록을 바꿀 수 있으므로 후보 순회가 끝날 때까지 변경을 지연한다.
	FScopedAbilityListLock ActiveScopeLock(MinionASC);

	for (const FGameplayAbilitySpec& AbilitySpec : MinionASC.GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability || !AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
		{
			continue;
		}

		// 같은 식별 태그에 조건별 후보가 여럿이면 발동 가능한 첫 후보 하나만 명령한다.
		if (!MinionASC.TriggerAbilityFromGameplayEvent(AbilitySpec.Handle, ActorInfo, WxGameplayTags::Event_CommandMinionAbility, &Payload, MinionASC))
		{
			continue;
		}

		return true;
	}

	return false;
}
