// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "Minion/WxMinion.h"
#include "WxCombatModule.h"
#include "WxGameplayTags.h"

APawn* UWxMinionSubsystem::GetMaster(const APawn& Minion)
{
	APawn* SpawnInstigator = Minion.GetInstigator();
	return SpawnInstigator != &Minion ? SpawnInstigator : nullptr;
}

APawn* UWxMinionSubsystem::FindActiveMinion(const APawn& Master) const
{
	const TArray<TWeakObjectPtr<APawn>> MasterMinions = CollectMinions(Master);
	return MasterMinions.IsEmpty() ? nullptr : MasterMinions[0].Get();
}

APawn* UWxMinionSubsystem::SpawnMinion(APawn& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform)
{
	if (!Master.HasAuthority() || !MinionClass)
	{
		return nullptr;
	}
	if (!MinionClass->ImplementsInterface(UGenericTeamAgentInterface::StaticClass()))
	{
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 소환 클래스 %s가 GenericTeamAgentInterface를 구현하지 않아 생성하지 않는다."), *Master.GetName(), *GetNameSafe(MinionClass.Get()));
		return nullptr;
	}
	if (!MinionClass->ImplementsInterface(UWxMinion::StaticClass()))
	{
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 소환 클래스 %s가 IWxMinion을 구현하지 않아 생성하지 않는다."), *Master.GetName(), *GetNameSafe(MinionClass.Get()));
		return nullptr;
	}
	Master.OnEndPlay.AddUniqueDynamic(this, &UWxMinionSubsystem::HandleMasterEndPlay);

	// 새 소환물 한 자리를 확보하되, 상한이 낮아진 경우 초과분도 함께 정리한다.
	const TArray<TWeakObjectPtr<APawn>> MasterMinions = CollectMinions(Master);
	const int32 MaxCountPerMaster = FMath::Max(0, IWxMinion::Execute_GetMaxCountPerMaster(MinionClass.GetDefaultObject()));
	const int32 MinionCountToRemove = MaxCountPerMaster > 0
		? FMath::Clamp(MasterMinions.Num() - MaxCountPerMaster + 1, 0, MasterMinions.Num()) : 0;
	for (int32 RemovedMinionCount = 0; RemovedMinionCount < MinionCountToRemove; ++RemovedMinionCount)
	{
		if (APawn* OldestMinion = MasterMinions[RemovedMinionCount].Get())
		{
			OldestMinion->Destroy();
		}
	}

	// 팀은 BeginPlay 전에 심어야 최초 복제값부터 옳고 첫 프레임의 인지·판정이 어긋나지 않는다.
	APawn* Minion = GetWorld()->SpawnActorDeferred<APawn>(MinionClass, SpawnTransform, nullptr, &Master, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Minion)
	{
		return nullptr;
	}

	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Minion))
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(&Master));
	}

	// 로스터 등재는 스폰 통지가 맡는다. 이 호출이 그 통지를 낸다.
	Minion->FinishSpawning(SpawnTransform);

	RefreshMasterStateTag(Master);
	return Minion;
}

int32 UWxMinionSubsystem::TryActivateAbilityOnMinions(APawn& Master, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload)
{
	if (!Master.HasAuthority() || !AbilityTag.IsValid())
	{
		return 0;
	}

	FGameplayEventData CommandPayload = Payload;
	if (!CommandPayload.Instigator)
	{
		CommandPayload.Instigator = &Master;
	}

	const TArray<TWeakObjectPtr<APawn>> CommandTargets = CollectMinions(Master);
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

void UWxMinionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UWxMinionSubsystem::HandleActorSpawned));
}

void UWxMinionSubsystem::Deinitialize()
{
	if (const UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}

	Super::Deinitialize();
}

TArray<TWeakObjectPtr<APawn>> UWxMinionSubsystem::CollectMinions(const APawn& Master) const
{
	TArray<TWeakObjectPtr<APawn>> MasterMinions;
	for (const TWeakObjectPtr<APawn>& Candidate : Minions)
	{
		const APawn* Minion = Candidate.Get();
		if (Minion && GetMaster(*Minion) == &Master)
		{
			MasterMinions.Add(Candidate);
		}
	}

	return MasterMinions;
}

void UWxMinionSubsystem::HandleActorSpawned(AActor* Actor)
{
	APawn* Minion = Cast<APawn>(Actor);
	if (!Minion || !Minion->GetClass()->ImplementsInterface(UWxMinion::StaticClass()))
	{
		return;
	}

	Minions.Add(Minion);
	Minion->OnEndPlay.AddDynamic(this, &UWxMinionSubsystem::HandleMinionEndPlay);
	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion))
	{
		MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).AddUObject(this, &UWxMinionSubsystem::HandleMinionDeathTagChanged, TWeakObjectPtr<APawn>(Minion));
	}
}

void UWxMinionSubsystem::HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	const APawn* Master = Cast<APawn>(Actor);
	if (!Master)
	{
		return;
	}

	// 파괴가 소환물 EndPlay를 동기 호출해 로스터를 줄이지만, 대상 집합은 이미 떠 왔으므로 순회와 겹치지 않는다.
	const TArray<TWeakObjectPtr<APawn>> MasterMinions = CollectMinions(*Master);
	for (const TWeakObjectPtr<APawn>& ActiveMinion : MasterMinions)
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

	ReleaseMinion(*Minion);
}

void UWxMinionSubsystem::HandleMinionDeathTagChanged(const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<APawn> Minion)
{
	APawn* DeadMinion = Minion.Get();
	if (NewCount <= 0 || !DeadMinion)
	{
		return;
	}

	ReleaseMinion(*DeadMinion);
}

void UWxMinionSubsystem::ReleaseMinion(APawn& Minion)
{
	if (Minions.Remove(TWeakObjectPtr<APawn>(&Minion)) == 0)
	{
		return;
	}

	Minion.OnEndPlay.RemoveDynamic(this, &UWxMinionSubsystem::HandleMinionEndPlay);
	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Minion))
	{
		MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).RemoveAll(this);
	}
	if (APawn* Master = GetMaster(Minion))
	{
		RefreshMasterStateTag(*Master);
	}
}

void UWxMinionSubsystem::RefreshMasterStateTag(APawn& Master) const
{
	// 복제되는 태그라 권위에서만 쓴다. 클라이언트가 덧쓰면 복제값과 싸운다.
	UAbilitySystemComponent* MasterASC = Master.HasAuthority() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Master) : nullptr;
	if (!MasterASC)
	{
		return;
	}

	MasterASC->SetLooseGameplayTagCount(WxGameplayTags::State_Minion_Active, FindActiveMinion(Master) ? 1 : 0, EGameplayTagReplicationState::TagOnly);
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
