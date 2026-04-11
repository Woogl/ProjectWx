// Copyright Woogle. All Rights Reserved.

#include "Component/WxInteractionComponent.h"

UWxInteractionComponent::UWxInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitSphereRadius(50.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SetGenerateOverlapEvents(true);
}

const TArray<FWxInteractionOption>& UWxInteractionComponent::GetOptions() const
{
	return Options;
}

void UWxInteractionComponent::TryInteract(AActor* Instigator, FGameplayTag InputTag)
{
	const AActor* MyOwner = GetOwner();
	if (!MyOwner || !MyOwner->HasAuthority())
	{
		return;
	}

	bool bHasOption = false;
	for (const FWxInteractionOption& Option : Options)
	{
		if (Option.InputTag == InputTag)
		{
			bHasOption = true;
			break;
		}
	}
	if (!bHasOption)
	{
		return;
	}

	OnInteracted.Broadcast(Instigator, InputTag);
}
