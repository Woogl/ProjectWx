// Copyright Woogle. All Rights Reserved.

#include "Component/WxInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Pawn.h"

UWxInteractionComponent::UWxInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InitSphereRadius(200.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
}

void UWxInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UWxInteractionComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddDynamic(this, &UWxInteractionComponent::HandleEndOverlap);

	if (const AActor* MyOwner = GetOwner())
	{
		CachedPromptWidget = MyOwner->FindComponentByClass<UWidgetComponent>();
	}

	// BeginPlay 시점에 이미 오버랩 중인 로컬 폰이 있으면 프롬프트를 즉시 표시한다.
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	for (AActor* OverlappingActor : OverlappingActors)
	{
		const APawn* Pawn = Cast<APawn>(OverlappingActor);
		if (Pawn && Pawn->IsLocallyControlled())
		{
			SetPromptVisible(true);
			break;
		}
	}
}

void UWxInteractionComponent::TryInteract(AActor* InstigatorActor)
{
	const AActor* MyOwner = GetOwner();
	if (!MyOwner || !MyOwner->HasAuthority())
	{
		return;
	}

	OnInteracted.Broadcast(InstigatorActor);
}

void UWxInteractionComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	SetPromptVisible(true);
}

void UWxInteractionComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 로컬 체크 없이 폰이 벗어나면 숨긴다. 컨트롤러 전환 등으로 End 시점에
	// IsLocallyControlled가 false가 되어 프롬프트가 잔존하는 것을 방지한다.
	if (!Cast<APawn>(OtherActor))
	{
		return;
	}

	SetPromptVisible(false);
}

void UWxInteractionComponent::SetPromptVisible(bool bNewVisible)
{
	if (UWidgetComponent* Widget = CachedPromptWidget.Get())
	{
		Widget->SetVisibility(bNewVisible);
	}
}
