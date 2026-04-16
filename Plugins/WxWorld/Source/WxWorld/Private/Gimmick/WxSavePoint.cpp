// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxSavePoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Component/WxInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "System/WxSpawnerSubsystem.h"

AWxSavePoint::AWxSavePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionWidget = CreateDefaultSubobject<UWxInteractionWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(MeshComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
	InteractionComponent->InteractionWidget = InteractionWidget;
}

void AWxSavePoint::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionComponent)
	{
		InteractionComponent->OnInteracted.AddDynamic(this, &AWxSavePoint::HandleInteracted);
	}
}

void AWxSavePoint::HandleInteracted(AActor* InteractingActor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (HealEffect)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InteractingActor))
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

	if (UWxSpawnerSubsystem* Subsystem = GetWorld()->GetSubsystem<UWxSpawnerSubsystem>())
	{
		Subsystem->RespawnAll();
	}
}
