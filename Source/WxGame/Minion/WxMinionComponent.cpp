// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "WxGame.h"
#include "WxGameplayTags.h"

UWxMinionComponent::UWxMinionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MasterStateTag = WxGameplayTags::Master_Minion;
}

APawn* UWxMinionComponent::GetMaster(const APawn& Minion)
{
	APawn* SpawnInstigator = Minion.GetInstigator();
	return SpawnInstigator != &Minion ? SpawnInstigator : nullptr;
}

APawn* UWxMinionComponent::SpawnMinion(APawn& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform)
{
	if (!Master.HasAuthority() || !MinionClass)
	{
		return nullptr;
	}

	// 소환물은 소환하지 못한다 — 주인의 어빌리티를 따라 하는 분신은 주인 태그가 없어 소환 조건을 그냥 통과한다.
	if (GetMaster(Master))
	{
		return nullptr;
	}

	// 컴포넌트가 적 캐릭터에 네이티브로 붙어 CDO에 실리므로, 스폰 전에 조건과 종류를 읽을 수 있다.
	const APawn* MinionDefaultPawn = MinionClass.GetDefaultObject();
	const UWxMinionComponent* MinionDefaults = MinionDefaultPawn ? MinionDefaultPawn->FindComponentByClass<UWxMinionComponent>() : nullptr;
	if (!MinionDefaults)
	{
		// 조건과 종류를 이 컴포넌트가 들고 있어, 없는 채로 소환하면 교체되지 않는 소환물이 무제한으로 쌓인다.
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 소환 클래스 %s의 CDO에 MinionComponent가 없어 소환하지 않는다. 네이티브로 부착해야 한다."), *Master.GetName(), *GetNameSafe(MinionClass.Get()));
		return nullptr;
	}

	// 조건 검사는 교체보다 앞선다 — 소환하지 않을 참에 기존 소환물을 파괴하면 안 된다.
	if (!MinionDefaults->CanBeSummonedBy(Master))
	{
		return nullptr;
	}

	for (UWxMinionComponent* Existing : FindMinions(Master))
	{
		if (Existing->MasterStateTag == MinionDefaults->MasterStateTag)
		{
			Existing->GetMinionPawn()->Destroy();
		}
	}

	// 팀은 BeginPlay 전에 심어야 최초 복제값부터 옳고 첫 프레임의 인지·판정이 어긋나지 않는다.
	APawn* Minion = Master.GetWorld()->SpawnActorDeferred<APawn>(MinionClass, SpawnTransform, nullptr, &Master, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
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

	// 이 호출이 내는 BeginPlay에서 소환물이 주인과 묶인다.
	Minion->FinishSpawning(SpawnTransform);

	return Minion;
}

void UWxMinionComponent::DespawnMinions(const APawn& Master, TSubclassOf<APawn> MinionClass)
{
	if (!Master.HasAuthority() || !MinionClass)
	{
		return;
	}

	for (const UWxMinionComponent* MinionComponent : FindMinions(Master))
	{
		APawn* Minion = MinionComponent->GetMinionPawn();
		UAbilitySystemComponent* MinionASC = Minion->IsA(MinionClass) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion) : nullptr;
		if (!MinionASC)
		{
			continue;
		}

		FGameplayEventData EventData;
		EventData.EventTag = WxGameplayTags::Event_Death;
		EventData.Instigator = &Master;
		EventData.Target = Minion;
		const int32 TriggeredCount = MinionASC->HandleGameplayEvent(WxGameplayTags::Event_Death, &EventData);

		if (TriggeredCount == 0)
		{
			Minion->Destroy();
		}
	}
}

APawn* UWxMinionComponent::FindActiveMinion(const APawn& Master)
{
	APawn* OldestMinion = nullptr;
	for (const UWxMinionComponent* MinionComponent : FindMinions(Master))
	{
		APawn* Minion = MinionComponent->GetMinionPawn();
		if (!OldestMinion || Minion->GetGameTimeSinceCreation() > OldestMinion->GetGameTimeSinceCreation())
		{
			OldestMinion = Minion;
		}
	}
	return OldestMinion;
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

void UWxMinionComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* Minion = GetMinionPawn();
	APawn* Master = Minion && Minion->HasAuthority() ? GetMaster(*Minion) : nullptr;
	if (!Master)
	{
		return;
	}

	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion))
	{
		DeathTagHandle = MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).AddUObject(this, &UWxMinionComponent::HandleDeathTagChanged);
	}

	Master->OnEndPlay.AddDynamic(this, &UWxMinionComponent::HandleMasterEndPlay);

	UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master);
	if (!MasterASC)
	{
		return;
	}

	if (MasterStateTag.IsValid())
	{
		MasterASC->AddLooseGameplayTag(MasterStateTag, 1, EGameplayTagReplicationState::TagOnly);
	}

	// 자기 태그를 쌓은 뒤에 들어야 그 변화로 자기 취소 조건을 재지 않는다. 취소 조건이 어떤 태그를 보든 받도록 주인 태그 변화 전체를 듣는다.
	if (!CancelMasterTagRequirements.IsEmpty())
	{
		MasterTagHandle = MasterASC->RegisterGenericGameplayTagEvent().AddUObject(this, &UWxMinionComponent::HandleMasterTagChanged);
	}
}

void UWxMinionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		MinionASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death).Remove(DeathTagHandle);
	}

	ReleaseMaster();

	Super::EndPlay(EndPlayReason);
}

TArray<UWxMinionComponent*> UWxMinionComponent::FindMinions(const APawn& Master)
{
	TArray<UWxMinionComponent*> Minions;
	for (TActorIterator<APawn> It(Master.GetWorld()); It; ++It)
	{
		APawn* Minion = *It;
		UWxMinionComponent* MinionComponent = GetMaster(*Minion) == &Master && Minion->HasActorBegunPlay() ? Minion->FindComponentByClass<UWxMinionComponent>() : nullptr;
		if (!MinionComponent)
		{
			continue;
		}

		// 죽으면 더 이상 소환물이 아니다 — 시체는 교체·회수·돌진 대상에서 빠진다.
		const UAbilitySystemComponent* MinionASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Minion);
		if (MinionASC && MinionASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
		{
			continue;
		}

		Minions.Add(MinionComponent);
	}
	return Minions;
}

void UWxMinionComponent::ReleaseMaster()
{
	const APawn* Minion = GetMinionPawn();
	APawn* Master = Minion ? GetMaster(*Minion) : nullptr;
	if (!Master || !Master->OnEndPlay.IsAlreadyBound(this, &UWxMinionComponent::HandleMasterEndPlay))
	{
		return;
	}

	Master->OnEndPlay.RemoveDynamic(this, &UWxMinionComponent::HandleMasterEndPlay);

	if (UAbilitySystemComponent* MasterASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Master))
	{
		MasterASC->RegisterGenericGameplayTagEvent().Remove(MasterTagHandle);
		if (MasterStateTag.IsValid())
		{
			MasterASC->RemoveLooseGameplayTag(MasterStateTag, 1, EGameplayTagReplicationState::TagOnly);
		}
	}
	MasterTagHandle.Reset();
}

void UWxMinionComponent::HandleDeathTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 태그가 내려가는 통지도 같은 델리게이트로 오므로 붙는 순간만 고른다.
	if (NewCount <= 0)
	{
		return;
	}

	// 죽으면 더 이상 소환물이 아니다. 주인 태그를 내리고, 시체가 취소·주인 소멸에 휩쓸려 사망 연출 도중 사라지지 않게 끊는다.
	ReleaseMaster();
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

void UWxMinionComponent::HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (APawn* Minion = GetMinionPawn())
	{
		Minion->Destroy();
	}
}
