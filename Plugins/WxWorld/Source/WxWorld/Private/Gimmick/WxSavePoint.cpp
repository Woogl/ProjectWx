// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxSavePoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/WxInteractionWidgetComponent.h"
#include "System/WxSpawnerSubsystem.h"

AWxSavePoint::AWxSavePoint()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionWidget->SetupAttachment(MeshComponent);
	InteractionComponent->SetupAttachment(MeshComponent);
}

void AWxSavePoint::BeginPlay()
{
	Super::BeginPlay();

	OnInteracted.AddDynamic(this, &AWxSavePoint::HandleInteracted);
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
