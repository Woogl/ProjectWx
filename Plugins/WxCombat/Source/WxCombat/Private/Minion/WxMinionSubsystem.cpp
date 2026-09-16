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

APawn* UWxMinionSubsystem::FindActiveMinion(const APawn& Master, TSubclassOf<APawn> MinionClass) const
{
	const TArray<TWeakObjectPtr<APawn>> MasterMinions = CollectMinions(Master);
	for (const TWeakObjectPtr<APawn>& Entry : MasterMinions)
	{
		APawn* Minion = Entry.Get();
		if (Minion && (!MinionClass || Minion->IsA(MinionClass)))
		{
			return Minion;
		}
	}
	return nullptr;
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
