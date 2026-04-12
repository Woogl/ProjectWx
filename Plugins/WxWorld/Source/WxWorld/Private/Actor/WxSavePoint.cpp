// Copyright Woogle. All Rights Reserved.

#include "Actor/WxSavePoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Component/WxInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

AWxSavePoint::AWxSavePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);

	PromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptWidget"));
	PromptWidget->SetupAttachment(MeshComponent);
	PromptWidget->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidget->SetDrawAtDesiredSize(true);
	PromptWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PromptWidget->SetVisibility(false);
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
	if (!HasAuthority() || !HealEffect)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InteractingActor);
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffect, 1.0f, Context);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

#if WITH_EDITOR
UStreamableRenderAsset* AWxSavePoint::GetEditorPreviewMesh() const
{
	return MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
}
#endif
