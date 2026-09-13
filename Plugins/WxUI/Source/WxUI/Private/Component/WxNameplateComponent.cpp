// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);
}

void UWxNameplateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (GetWorld()->IsGameWorld())
	{
		const APawn* ViewerPawn = MaxVisibilityDistance > 0.f ? UGameplayStatics::GetPlayerPawn(this, 0) : nullptr;
		UpdatePresentation(ViewerPawn);
	}
	// 갱신한 가시성을 같은 프레임의 화면 부착·해제에 반영한다.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UWxNameplateComponent::UpdatePresentation(const APawn* ViewerPawn)
{
	UUserWidget* NameplateWidget = GetWidget();
	if (!ViewerPawn || !NameplateWidget || MaxVisibilityDistance <= 0.f)
	{
		SetVisibility(false);
		return;
	}

	const double Distance = FVector::Dist(GetComponentLocation(), ViewerPawn->GetActorLocation());
	if (Distance > MaxVisibilityDistance)
	{
		SetVisibility(false);
		return;
	}

	const double Scale = FMath::Clamp(ReferenceDistance / FMath::Max(Distance, 1.0),
		static_cast<double>(MinScale), static_cast<double>(MaxScale));
	const FVector2D RenderScale(Scale, Scale);
	if (!NameplateWidget->GetRenderTransform().Scale.Equals(RenderScale))
	{
		NameplateWidget->SetRenderScale(RenderScale);
	}
	// WBP의 태그 가시성을 덮어쓰지 않고 컴포넌트의 거리 게이트만 연다.
	SetVisibility(true);
}
