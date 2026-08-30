// Copyright Woogle. All Rights Reserved.

#include "Summon/WxSummonComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "WxGameplayTags.h"

UWxSummonComponent* UWxSummonComponent::FindComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UWxSummonComponent>() : nullptr;
}

bool UWxSummonComponent::RegisterSummon(APawn* Summon)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !Summon)
	{
		return false;
	}

	for (const FWxActiveSummon& ActiveSummon : ActiveSummons)
	{
		if (ActiveSummon.Pawn.Get() == Summon)
		{
			return false;
		}
	}

	FWxActiveSummon& ActiveSummon = ActiveSummons.AddDefaulted_GetRef();
	ActiveSummon.Pawn = Summon;
	Summon->OnEndPlay.AddDynamic(this, &UWxSummonComponent::HandleSummonEndPlay);

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Summon))
	{
		ActiveSummon.AbilitySystemComponent = ASC;
		ActiveSummon.DeathTagDelegateHandle = ASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UWxSummonComponent::HandleSummonDeath, TWeakObjectPtr<APawn>(Summon));

		if (ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
		{
			RemoveSummon(Summon, false);
		}
	}

	return true;
}

void UWxSummonComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (int32 SummonIndex = ActiveSummons.Num() - 1; SummonIndex >= 0; --SummonIndex)
	{
		RemoveSummonAt(SummonIndex, true);
	}

	Super::EndPlay(EndPlayReason);
}

void UWxSummonComponent::RemoveSummon(APawn* Summon, bool bDestroyActor)
{
	if (!Summon)
	{
		return;
	}

	for (int32 SummonIndex = 0; SummonIndex < ActiveSummons.Num(); ++SummonIndex)
	{
		if (ActiveSummons[SummonIndex].Pawn.Get() == Summon)
		{
			RemoveSummonAt(SummonIndex, bDestroyActor);
			return;
		}
	}
}

void UWxSummonComponent::RemoveSummonAt(int32 SummonIndex, bool bDestroyActor)
{
	if (!ActiveSummons.IsValidIndex(SummonIndex))
	{
		return;
	}

	FWxActiveSummon ActiveSummon = MoveTemp(ActiveSummons[SummonIndex]);
	ActiveSummons.RemoveAtSwap(SummonIndex);

	if (APawn* Summon = ActiveSummon.Pawn.Get())
	{
		Summon->OnEndPlay.RemoveDynamic(this, &UWxSummonComponent::HandleSummonEndPlay);
	}

	if (UAbilitySystemComponent* ASC = ActiveSummon.AbilitySystemComponent.Get())
	{
		ASC->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
			.Remove(ActiveSummon.DeathTagDelegateHandle);
	}

	if (bDestroyActor)
	{
		if (APawn* Summon = ActiveSummon.Pawn.Get())
		{
			if (!Summon->IsActorBeingDestroyed())
			{
				Summon->Destroy();
			}
		}
	}
}

void UWxSummonComponent::HandleSummonDeath(FGameplayTag CallbackTag, int32 NewCount, TWeakObjectPtr<APawn> Summon)
{
	if (NewCount > 0)
	{
		RemoveSummon(Summon.Get(), false);
	}
}

void UWxSummonComponent::HandleSummonEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	RemoveSummon(Cast<APawn>(Actor), false);
}
