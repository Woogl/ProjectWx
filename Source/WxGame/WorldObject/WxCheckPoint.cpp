// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxCheckPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "System/WxSpawnerSubsystem.h"

AWxCheckPoint::AWxCheckPoint()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxCheckPoint::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnInteracted.AddDynamic(this, &AWxCheckPoint::HandleInteracted);
}

void AWxCheckPoint::HandleInteracted(AActor* InstigatorActor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (HealEffect)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor))
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffect, 1.0f, Context);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 에스트병 등 충전형 소비 아이템의 충전을 가득 채운다(다크소울 모닥불 방식). 충전형이 아닌 아이템은 내부에서 무시된다.
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(InstigatorActor))
	{
		for (UWxItemInstance* Item : Inventory->GetAllItems())
		{
			Inventory->RefillItemCharges(Item);
		}
	}

	if (UWxSpawnerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWxSpawnerSubsystem>())
	{
		Subsystem->RespawnAutoSpawners();
	}
}
