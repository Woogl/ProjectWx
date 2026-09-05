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

	// 새 소환물 한 자리를 확보하되, 상한이 낮아진 경우 초과분도 함께 정리한다.
	// Destroy가 EndPlay를 동기 호출하므로 로스터에서 먼저 내려야 핸들러가 이 순회와 겹치지 않는다.
	// 태그는 새 소환물까지 올린 뒤 한 번만 발행한다 — 교체 소환이 한 프레임에 1→0→1로 튀지 않게.
	const int32 MinionCountToRemove = FMath::Clamp(Minions.Num() - MaxMinionCount + 1, 0, Minions.Num());
	for (int32 RemovedMinionCount = 0; RemovedMinionCount < MinionCountToRemove; ++RemovedMinionCount)
	{
		APawn* OldestMinion = Minions[0].Get();
		if (!OldestMinion)
		{
			Minions.RemoveAt(0);
			continue;
		}

		ReleaseMinion(*OldestMinion);
		OldestMinion->Destroy();
	}

	// 팀은 BeginPlay 전에 심어야 최초 복제값부터 옳고 첫 프레임의 인지·판정이 어긋나지 않는다.
	APawn* Minion = GetWorld()->SpawnActorDeferred<APawn>(MinionClass, SpawnTransform, nullptr, Cast<APawn>(&Master), ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Minion)
	{
		PublishMinionActiveTag(Master);
		return nullptr;
	}

	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Minion))
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(&Master));
	}

	Minion->FinishSpawning(SpawnTransform);

	Minions.Add(Minion);
	Minion->OnEndPlay.AddDynamic(this, &UWxMinionSubsystem::HandleMinionEndPlay);
	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion))
	{
		MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).AddUObject(this, &UWxMinionSubsystem::HandleMinionDeathTagChanged, TWeakObjectPtr<APawn>(Minion));
	}

	PublishMinionActiveTag(Master);

	return Minion;
}

int32 UWxMinionSubsystem::TryActivateAbilityOnMinions(AActor& Master, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload)
{
	if (!Master.HasAuthority() || !AbilityTag.IsValid())
	{
		return 0;
	}

	const TArray<TWeakObjectPtr<APawn>>* Minions = Rosters.Find(&Master);
	if (!Minions)
	{
		return 0;
	}

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

	// 로스터를 먼저 내렸으므로 파괴로 오는 소환물 EndPlay는 주인을 못 찾고 그냥 돌아온다. 주인은 끝나는 중이라 태그를 만지지 않는다.
	for (const TWeakObjectPtr<APawn>& ActiveMinion : Minions)
	{
		if (APawn* Minion = ActiveMinion.Get())
		{
			Minion->Destroy();
		}
	}
}

void UWxMinionSubsystem::HandleMinionEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	APawn* Minion = Cast<APawn>(Actor);
	if (!Minion)
	{
		return;
	}

	if (AActor* Master = ReleaseMinion(*Minion))
	{
		PublishMinionActiveTag(*Master);
	}
}

void UWxMinionSubsystem::HandleMinionDeathTagChanged(const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<APawn> Minion)
{
	APawn* DeadMinion = Minion.Get();
	if (NewCount <= 0 || !DeadMinion)
	{
		return;
	}

	if (AActor* Master = ReleaseMinion(*DeadMinion))
	{
		PublishMinionActiveTag(*Master);
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

AActor* UWxMinionSubsystem::ReleaseMinion(APawn& Minion)
{
	for (TPair<TWeakObjectPtr<AActor>, TArray<TWeakObjectPtr<APawn>>>& Roster : Rosters)
	{
		if (Roster.Value.Remove(TWeakObjectPtr<APawn>(&Minion)) == 0)
		{
			continue;
		}

		Minion.OnEndPlay.RemoveDynamic(this, &UWxMinionSubsystem::HandleMinionEndPlay);
		if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Minion))
		{
			MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).RemoveAll(this);
		}

		return Roster.Key.Get();
	}

	return nullptr;
}

void UWxMinionSubsystem::PublishMinionActiveTag(AActor& Master) const
{
	UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Master);
	if (!MasterASC)
	{
		return;
	}

	const TArray<TWeakObjectPtr<APawn>>* Minions = Rosters.Find(&Master);
	const bool bHasActiveMinion = Minions && !Minions->IsEmpty();

	// 클라이언트도 이 태그로 소환·명령 중 어느 쪽을 예측 발동할지 고르므로 복제한다.
	MasterASC->SetLooseGameplayTagCount(WxGameplayTags::State_Minion_Active, bHasActiveMinion ? 1 : 0, EGameplayTagReplicationState::TagOnly);
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
