// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/MeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"
#include "Interaction/WxInteractionWidgetInterface.h"

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
		SetInteractionWidgetVisible(false);
		SetHighlightEnabled(false);
	}
}

FWxOnInteractedSignature& UWxInteractionComponent::GetOnInteractedDelegate()
{
	return OnInteracted;
}

void UWxInteractionComponent::SetInteractionText(const FText& InText)
{
	InteractionText = InText;

	if (!InteractionWidget)
	{
		return;
	}

	UUserWidget* UserWidget = InteractionWidget->GetUserWidgetObject();
	if (!UserWidget)
	{
		return;
	}

	if (UserWidget->Implements<UWxInteractionWidgetInterface>())
	{
		IWxInteractionWidgetInterface::Execute_SetInteractionText(UserWidget, InteractionText);
	}
}

void UWxInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UWxInteractionComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UWxInteractionComponent::HandleEndOverlap);

	SetInteractionText(InteractionText);

	if (!bInteractionEnabled)
	{
		return;
	}

	// BeginPlay 시점에 이미 오버랩 중인 로컬 플레이어 폰이 있으면 프롬프트를 즉시 표시한다.
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (IsLocalPlayerPawn(OverlappingActor))
		{
			SetInteractionWidgetVisible(true);
			SetHighlightEnabled(true);
			break;
		}
	}
}

void UWxInteractionComponent::OnRegister()
{
	Super::OnRegister();

	if (!InteractionWidget)
	{
		InteractionWidget = NewObject<UWidgetComponent>(this, UWidgetComponent::StaticClass(), TEXT("InteractionWidget"), RF_Transactional);
		InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
		InteractionWidget->SetDrawAtDesiredSize(true);
		InteractionWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InteractionWidget->SetVisibility(false);
		InteractionWidget->SetupAttachment(this);
		InteractionWidget->RegisterComponent();
	}

	if (InteractionWidget->GetWidgetClass() != PromptWidgetClass)
	{
		InteractionWidget->SetWidgetClass(PromptWidgetClass);
	}
}

void UWxInteractionComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}

	SetInteractionWidgetVisible(true);
	SetHighlightEnabled(true);
}

void UWxInteractionComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsLocalPlayerPawn(OtherActor))
	{
		return;
	}

	SetInteractionWidgetVisible(false);
	SetHighlightEnabled(false);
}

void UWxInteractionComponent::MulticastInteracted_Implementation(AActor* InstigatorActor)
{
	OnInteracted.Broadcast(InstigatorActor);
}

void UWxInteractionComponent::SetInteractionWidgetVisible(bool bNewVisible)
{
	if (!InteractionWidget)
	{
		return;
	}

	InteractionWidget->SetVisibility(bNewVisible);

	// visible 전환 시 텍스트를 재적용한다 — UWidgetComponent 가 visibility 토글 시점에
	// 위젯 인스턴스를 lazy 하게 만드는 케이스에서도 BP 오버라이드 텍스트가 누락되지 않도록 보장.
	if (bNewVisible)
	{
		SetInteractionText(InteractionText);
	}
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
		// 프롬프트 위젯도 UMeshComponent 파생이므로 외곽선 대상에서 제외한다.
		if (MeshComponent->IsA<UWidgetComponent>())
		{
			continue;
		}

		MeshComponent->SetRenderCustomDepth(bNewEnabled);
		if (bNewEnabled)
		{
			MeshComponent->SetCustomDepthStencilValue(HighlightStencilValue);
		}
	}
}

bool UWxInteractionComponent::IsLocalPlayerPawn(const AActor* OtherActor) const
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	return Pawn != nullptr && Pawn->IsPlayerControlled() && Pawn->IsLocallyControlled();
}
