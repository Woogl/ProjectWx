// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "Minion/WxMinionComponent.h"
#include "WxCombatModule.h"
#include "WxGameplayTags.h"

APawn* UWxMinionSubsystem::FindActiveMinion(const APawn& Master) const
{
	// CollectMinions 가 폰이 유효한 후보만 소환 순서대로 담으므로 첫 항목이 곧 가장 오래된 소환물이다.
	const TArray<TWeakObjectPtr<UWxMinionComponent>> MasterMinions = CollectMinions(Master);
	const UWxMinionComponent* FirstMinion = MasterMinions.IsEmpty() ? nullptr : MasterMinions[0].Get();
	return FirstMinion ? FirstMinion->GetMinionPawn() : nullptr;
}

APawn* UWxMinionSubsystem::SpawnMinion(APawn& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform)
{
	if (!Master.HasAuthority() || !MinionClass)
	{
		return nullptr;
	}

	// 소환물은 소환하지 못한다 — 주인의 어빌리티를 따라 하는 분신은 주인 태그가 없어 소환 조건을 그냥 통과한다.
	if (UWxMinionComponent::GetMaster(Master))
	{
		return nullptr;
	}

	// 컴포넌트가 적 캐릭터에 네이티브로 붙어 CDO에 실리므로, 스폰 전에 조건과 상한을 읽을 수 있다.
	const APawn* MinionDefaultPawn = MinionClass.GetDefaultObject();
	const UWxMinionComponent* MinionDefaults = MinionDefaultPawn ? MinionDefaultPawn->FindComponentByClass<UWxMinionComponent>() : nullptr;
	if (!MinionDefaults)
	{
		// 상한도 조건도 이 컴포넌트가 들고 있어, 없는 채로 소환하면 추적되지 않는 소환물이 무제한으로 쌓인다.
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 소환 클래스 %s의 CDO에 MinionComponent가 없어 소환하지 않는다. 네이티브로 부착해야 한다."), *Master.GetName(), *GetNameSafe(MinionClass.Get()));
		return nullptr;
	}

	// 조건 검사는 상한 정리보다 앞선다 — 소환하지 않을 참에 낡은 소환물을 파괴하면 안 된다.
	if (!MinionDefaults->CanBeSummonedBy(Master))
	{
		return nullptr;
	}
	Master.OnEndPlay.AddUniqueDynamic(this, &UWxMinionSubsystem::HandleMasterEndPlay);

	// 새 소환물 한 자리를 확보하되, 상한이 낮아진 경우 초과분도 함께 정리한다.
	// 자리를 다투는 것은 같은 주인 태그를 선언한 소환물뿐이라, 다른 종류는 이 정리에 휘말리지 않는다.
	const TArray<TWeakObjectPtr<UWxMinionComponent>> MasterMinions = CollectMinions(Master, MinionDefaults->GetMasterStateTag());
	const int32 MaxCountPerMaster = MinionDefaults->GetMaxCountPerMaster();
	const int32 MinionCountToRemove = MaxCountPerMaster > 0
		? FMath::Clamp(MasterMinions.Num() - MaxCountPerMaster + 1, 0, MasterMinions.Num()) : 0;
	for (int32 RemovedMinionCount = 0; RemovedMinionCount < MinionCountToRemove; ++RemovedMinionCount)
	{
		const UWxMinionComponent* OldestMinion = MasterMinions[RemovedMinionCount].Get();
		if (APawn* OldestPawn = OldestMinion ? OldestMinion->GetMinionPawn() : nullptr)
		{
			OldestPawn->Destroy();
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
	else
	{
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 소환물 %s가 GenericTeamAgentInterface를 구현하지 않아 주인의 팀을 물려받지 못한다."), *Master.GetName(), *Minion->GetName());
	}

	// 로스터 등재는 소환물의 MinionComponent가 BeginPlay에서 한다. 이 호출이 그 BeginPlay를 낸다.
	Minion->FinishSpawning(SpawnTransform);

	return Minion;
}

void UWxMinionSubsystem::DespawnMinions(const APawn& Master, TSubclassOf<APawn> MinionClass)
{
	if (!Master.HasAuthority() || !MinionClass)
	{
		return;
	}

	// 로스터 해제는 소환물 컴포넌트가 사망 태그를 보고 하므로 여기서 내리지 않는다. 이미 죽은 소환물은 그 해제로 목록에 없다.
	const TArray<TWeakObjectPtr<UWxMinionComponent>> MasterMinions = CollectMinions(Master);
	for (const TWeakObjectPtr<UWxMinionComponent>& MasterMinion : MasterMinions)
	{
		const UWxMinionComponent* MinionComponent = MasterMinion.Get();
		APawn* Minion = MinionComponent ? MinionComponent->GetMinionPawn() : nullptr;
		UAbilitySystemComponent* MinionASC = Minion && Minion->IsA(MinionClass) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion) : nullptr;
		if (!MinionASC)
		{
			continue;
		}

		FGameplayEventData EventData;
		EventData.EventTag = WxGameplayTags::Event_Death;
		EventData.Instigator = &Master;
		EventData.Target = Minion;
		const int32 TriggeredCount = MinionASC->HandleGameplayEvent(WxGameplayTags::Event_Death, &EventData);

		// 사망 어빌리티가 없는 경우 강제 파괴
		if (TriggeredCount == 0)
		{
			Minion->Destroy();
		}
	}
}

void UWxMinionSubsystem::RegisterMinion(UWxMinionComponent& MinionComponent)
{
	APawn* Minion = MinionComponent.GetMinionPawn();
	if (!Minion || Minions.Contains(TWeakObjectPtr<UWxMinionComponent>(&MinionComponent)))
	{
		return;
	}

	Minions.Add(&MinionComponent);
	if (APawn* Master = UWxMinionComponent::GetMaster(*Minion))
	{
		RefreshMasterStateTag(*Master, MinionComponent.GetMasterStateTag());
	}
}

void UWxMinionSubsystem::UnregisterMinion(UWxMinionComponent& MinionComponent)
{
	if (Minions.Remove(TWeakObjectPtr<UWxMinionComponent>(&MinionComponent)) == 0)
	{
		return;
	}

	const APawn* Minion = MinionComponent.GetMinionPawn();
	if (APawn* Master = Minion ? UWxMinionComponent::GetMaster(*Minion) : nullptr)
	{
		RefreshMasterStateTag(*Master, MinionComponent.GetMasterStateTag());
	}
}

bool UWxMinionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TArray<TWeakObjectPtr<UWxMinionComponent>> UWxMinionSubsystem::CollectMinions(const APawn& Master, const FGameplayTag StateTag) const
{
	TArray<TWeakObjectPtr<UWxMinionComponent>> MasterMinions;
	for (const TWeakObjectPtr<UWxMinionComponent>& Candidate : Minions)
	{
		const UWxMinionComponent* MinionComponent = Candidate.Get();
		const APawn* Minion = MinionComponent ? MinionComponent->GetMinionPawn() : nullptr;
		if (!Minion || UWxMinionComponent::GetMaster(*Minion) != &Master)
		{
			continue;
		}

		if (!StateTag.IsValid() || MinionComponent->GetMasterStateTag() == StateTag)
		{
			MasterMinions.Add(Candidate);
		}
	}

	return MasterMinions;
}

void UWxMinionSubsystem::HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	const APawn* Master = Cast<APawn>(Actor);
	if (!Master)
	{
		return;
	}

	const TArray<TWeakObjectPtr<UWxMinionComponent>> MasterMinions = CollectMinions(*Master);
	for (const TWeakObjectPtr<UWxMinionComponent>& ActiveMinion : MasterMinions)
	{
		const UWxMinionComponent* MinionComponent = ActiveMinion.Get();
		if (APawn* Minion = MinionComponent ? MinionComponent->GetMinionPawn() : nullptr)
		{
			Minion->Destroy();
		}
	}
}

void UWxMinionSubsystem::RefreshMasterStateTag(APawn& Master, const FGameplayTag StateTag) const
{
	// 복제되는 태그라 권위에서만 쓴다. 클라이언트가 덧쓰면 복제값과 싸운다.
	UAbilitySystemComponent* MasterASC = Master.HasAuthority() ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Master) : nullptr;
	if (!MasterASC || !StateTag.IsValid())
	{
		return;
	}

	const bool bHoldsTag = !CollectMinions(Master, StateTag).IsEmpty();
	MasterASC->SetLooseGameplayTagCount(StateTag, bHoldsTag ? 1 : 0, EGameplayTagReplicationState::TagOnly);
}
