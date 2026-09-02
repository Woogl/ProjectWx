// Copyright Woogle. All Rights Reserved.

#include "Minion/WxMinionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AnimNotify/WxAnimNotify_SpawnMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
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
		AActor* Minion = ActiveMinion.Get();
		if (!Minion)
		{
			continue;
		}

		Minion->Destroy();
	}

	ActiveMinions.Reset();

	Super::EndPlay(EndPlayReason);
}

int32 UWxMinionComponent::TryActivateAbilityOnMinions(const FGameplayTag& AbilityTag)
{
	return TryActivateAbilityOnMinions(AbilityTag, FGameplayEventData());
}

int32 UWxMinionComponent::TryActivateAbilityOnMinions(const FGameplayTag& AbilityTag, const FGameplayEventData& Payload)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return 0;
	}

	if (!AbilityTag.IsValid())
	{
		return 0;
	}

	RemoveInvalidOrDeadMinions();

	FGameplayEventData CommandPayload = Payload;
	if (!CommandPayload.Instigator)
	{
		CommandPayload.Instigator = Owner;
	}

	// 발동한 어빌리티가 액터 수명이나 소환 목록을 바꾸더라도 이번 명령의 대상 집합은 유지한다.
	const TArray<TWeakObjectPtr<AActor>> CommandTargets = ActiveMinions;
	int32 ActivatedMinionCount = 0;

	for (const TWeakObjectPtr<AActor>& CommandTarget : CommandTargets)
	{
		AActor* Minion = CommandTarget.Get();
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

	RemoveInvalidOrDeadMinions();

	// 새 소환수 한 자리를 확보하되, 런타임에 상한이 낮아진 경우 초과분도 함께 정리한다.
	const int32 MinionCountToRemove = FMath::Max(ActiveMinions.Num() - MaxMinionCount + 1, 0);
	for (int32 RemovedMinionCount = 0; RemovedMinionCount < MinionCountToRemove; ++RemovedMinionCount)
	{
		if (AActor* OldestMinion = ActiveMinions[0].Get())
		{
			OldestMinion->Destroy();
		}

		ActiveMinions.RemoveAt(0);
	}

	const FVector SpawnLocation = Owner->GetActorTransform().TransformPosition(MinionNotify->GetSpawnOffset());
	const FTransform SpawnTransform(Owner->GetActorRotation(), SpawnLocation);

	// 팀은 BeginPlay 전에 심어야 최초 복제값부터 옳고 첫 프레임의 인지·판정이 어긋나지 않는다.
	APawn* Master = Cast<APawn>(Owner);
	AActor* Minion = Owner->GetWorld()->SpawnActorDeferred<AActor>(MinionNotify->GetMinionClass(), SpawnTransform, nullptr, Master, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Minion)
	{
		return;
	}

	if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Minion))
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::GetTeamIdentifier(Master));
	}

	Minion->FinishSpawning(SpawnTransform);

	if (MinionNotify->GetLifetime() > 0.f)
	{
		Minion->SetLifeSpan(MinionNotify->GetLifetime());
	}

	ActiveMinions.Add(Minion);
}

void UWxMinionComponent::RemoveInvalidOrDeadMinions()
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
		if (!MinionASC || !MinionASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
		{
			continue;
		}

		ActiveMinions.RemoveAt(MinionIndex);
	}
}

bool UWxMinionComponent::TryActivateAbilityByExactTag(UAbilitySystemComponent& MinionASC, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload) const
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
