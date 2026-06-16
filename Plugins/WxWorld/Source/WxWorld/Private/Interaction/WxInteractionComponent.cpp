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

void UWxInteractionComponent::SetHighlightEnabled(bool bNewEnabled)
{
	if (!bEnableHighlight)
	{
		return;
	}

	// 이 컴포넌트가 부착된 메시만 강조한다. 부착 부모가 메시가 아니면 강조 대상이 없다.
	UMeshComponent* MeshComponent = Cast<UMeshComponent>(GetAttachParent());
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetRenderCustomDepth(bNewEnabled);
	if (bNewEnabled)
	{
		MeshComponent->SetCustomDepthStencilValue(HighlightStencilValue);
	}
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

	// BeginPlay 시점에 이미 오버랩 중인 로컬 플레이어 폰이 있으면 레지스트리에 즉시 등록한다(강조는 레지스트리가 조율).
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsLocalPlayerPawn(OverlappingActor))
		{
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

	RegisterWithRegistry(OtherActor);
}

void UWxInteractionComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}

	UnregisterFromRegistry();
}

void UWxInteractionComponent::MulticastInteracted_Implementation(AActor* InstigatorActor)
{
	OnInteracted.Broadcast(InstigatorActor);
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
