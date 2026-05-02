// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractableActor.h"

#include "Interaction/WxInteractionWidgetComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"

AWxInteractableActor::AWxInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	bInteractionEnabled = true;

	InteractionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionComponent"));
	InteractionComponent->InitSphereRadius(200.f);
	InteractionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionComponent->SetGenerateOverlapEvents(true);

	InteractionWidget = CreateDefaultSubobject<UWxInteractionWidgetComponent>(TEXT("InteractionWidget"));
}

void AWxInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	InteractionComponent->OnComponentBeginOverlap.AddDynamic(this, &AWxInteractableActor::HandleBeginOverlap);
	InteractionComponent->OnComponentEndOverlap.AddDynamic(this, &AWxInteractableActor::HandleEndOverlap);

	if (!bInteractionEnabled)
	{
		return;
	}

	// BeginPlay 시점에 이미 오버랩 중인 로컬 폰이 있으면 프롬프트를 즉시 표시한다.
	TArray<AActor*> OverlappingActors;
	InteractionComponent->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		const APawn* Pawn = Cast<APawn>(OverlappingActor);
		if (Pawn && Pawn->IsLocallyControlled())
		{
			SetInteractionWidgetVisible(true);
			break;
		}
	}
}

void AWxInteractableActor::TryInteract(AActor* InstigatorActor)
{
	if (!HasAuthority() || !bInteractionEnabled)
	{
		return;
	}

	MulticastInteracted(InstigatorActor);
}

void AWxInteractableActor::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled == bEnabled)
	{
		return;
	}

	bInteractionEnabled = bEnabled;

	if (bEnabled)
	{
		InteractionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionComponent->UpdateOverlaps();
	}
	else
	{
		InteractionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetInteractionWidgetVisible(false);
	}
}

void AWxInteractableActor::MulticastInteracted_Implementation(AActor* InstigatorActor)
{
	OnInteracted.Broadcast(InstigatorActor);
}

void AWxInteractableActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	SetInteractionWidgetVisible(true);
}

void AWxInteractableActor::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<APawn>(OtherActor))
	{
		return;
	}

	// 로컬 폰이 아직 범위 안에 있으면 프롬프트를 유지한다.
	TArray<AActor*> OverlappingActors;
	InteractionComponent->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn && Pawn->IsLocallyControlled())
		{
			return;
		}
	}

	SetInteractionWidgetVisible(false);
}

void AWxInteractableActor::SetInteractionWidgetVisible(bool bNewVisible)
{
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(bNewVisible);
	}
}
