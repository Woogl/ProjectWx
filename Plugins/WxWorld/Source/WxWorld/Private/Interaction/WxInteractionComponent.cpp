// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionRegistrySubsystem.h"

UWxInteractionComponent::UWxInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	bInteractionEnabled = true;

	InitSphereRadius(150.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);

	InteractionText = FText::FromString(TEXT("[F] Interact"));
}

void UWxInteractionComponent::TryInteract(AActor* InstigatorActor)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !bInteractionEnabled)
	{
		return;
	}

	MulticastInteracted(InstigatorActor);
}

void UWxInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled == bEnabled)
	{
		return;
	}

	bInteractionEnabled = bEnabled;

	if (bEnabled)
	{
		SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		UpdateOverlaps();
	}
	else
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetHighlightEnabled(false);
		UnregisterFromRegistry();
	}
}

FWxOnInteractedSignature& UWxInteractionComponent::GetOnInteractedDelegate()
{
	return OnInteracted;
}

void UWxInteractionComponent::SetInteractionText(const FText& InText)
{
	InteractionText = InText;
}

void UWxInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UWxInteractionComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UWxInteractionComponent::HandleEndOverlap);

	if (!bInteractionEnabled)
	{
		return;
	}

	// BeginPlay 시점에 이미 오버랩 중인 로컬 플레이어 폰이 있으면 강조/등록을 즉시 적용한다.
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsLocalPlayerPawn(OverlappingActor))
		{
			SetHighlightEnabled(true);
			RegisterWithRegistry(OverlappingActor);
			break;
		}
	}
}

void UWxInteractionComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}

	SetHighlightEnabled(true);
	RegisterWithRegistry(OtherActor);
}

void UWxInteractionComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}

	SetHighlightEnabled(false);
	UnregisterFromRegistry();
}

void UWxInteractionComponent::MulticastInteracted_Implementation(AActor* InstigatorActor)
{
	OnInteracted.Broadcast(InstigatorActor);
}

void UWxInteractionComponent::SetHighlightEnabled(bool bNewEnabled)
{
	if (!bEnableHighlight)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		MeshComponent->SetRenderCustomDepth(bNewEnabled);
		if (bNewEnabled)
		{
			MeshComponent->SetCustomDepthStencilValue(HighlightStencilValue);
		}
	}
}

void UWxInteractionComponent::RegisterWithRegistry(AActor* PlayerActor)
{
	const APawn* Pawn = Cast<APawn>(PlayerActor);
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (UWxInteractionRegistrySubsystem* Registry = LocalPlayer ? LocalPlayer->GetSubsystem<UWxInteractionRegistrySubsystem>() : nullptr)
	{
		Registry->RegisterInRange(this);
		RegisteredRegistry = Registry;
	}
}

void UWxInteractionComponent::UnregisterFromRegistry()
{
	if (UWxInteractionRegistrySubsystem* Registry = RegisteredRegistry.Get())
	{
		Registry->UnregisterInRange(this);
	}
	RegisteredRegistry = nullptr;
}

bool UWxInteractionComponent::IsLocalPlayerPawn(const AActor* OtherActor) const
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	return Pawn != nullptr && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled();
}
