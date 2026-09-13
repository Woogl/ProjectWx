// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MVVM/WxViewModel_Character.h"
#include "View/MVVMView.h"
#include "WxUIModule.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);
}

void UWxNameplateComponent::InitWidget()
{
	UUserWidget* PreviousWidget = GetWidget();
	Super::InitWidget();
	if (GetWidget() != PreviousWidget)
	{
		BindViewModel();
	}
}

void UWxNameplateComponent::SetWidget(UUserWidget* InWidget)
{
	Super::SetWidget(InWidget);
	BindViewModel();
}

void UWxNameplateComponent::BindViewModel()
{
	UUserWidget* NameplateWidget = GetWidget();
	if (!NameplateWidget || !GetWorld() || !GetWorld()->IsGameWorld() || NameplateWidget->IsDesignTime())
	{
		return;
	}

	UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>();
	if (!View)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 위젯에 MVVM View가 없다. Widget=%s"), *GetNameSafe(NameplateWidget));
		return;
	}

	AActor* Owner = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 위젯 구성 전에 Owner의 ASC가 필요하다. Owner=%s"), *GetNameSafe(Owner));
		return;
	}

	// 공유본의 수명은 이를 참조하는 MVVM View가 유지한다. 컴포넌트가 직접 초기화·해제하지 않는다.
	if (!View->SetViewModelByClass(UWxViewModel_Character::GetOrCreate(ASC, Owner)))
	{
		UE_LOG(LogWxUI, Warning, TEXT("Nameplate: Character 뷰모델을 연결하지 못했다. 위젯의 Manual 소스를 확인한다. Widget=%s"), *GetNameSafe(NameplateWidget));
	}
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
