// Copyright Woogle. All Rights Reserved.

#include "Component/WxNameplateComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MVVM/WxViewModel_Character.h"
#include "View/MVVMView.h"
#include "View/MVVMViewClass.h"
#include "WxUIModule.h"

UWxNameplateComponent::UWxNameplateComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetVisibility(false);
}

void UWxNameplateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseViewModel();
	SetVisibility(false);
	Super::EndPlay(EndPlayReason);
}

void UWxNameplateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (GetWorld()->IsGameWorld())
	{
		UpdatePresentation(UGameplayStatics::GetPlayerPawn(this, 0));
	}
	// 갱신한 가시성을 같은 프레임의 화면 부착·해제에 반영한다.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UWxNameplateComponent::InitWidget()
{
	Super::InitWidget();
	BindViewModel();
}

void UWxNameplateComponent::SetWidget(UUserWidget* InWidget)
{
	if (GetWidget() != InWidget)
	{
		ReleaseViewModel();
		SetVisibility(false);
	}
	Super::SetWidget(InWidget);
	BindViewModel();
}

void UWxNameplateComponent::BindViewModel()
{
	UUserWidget* NameplateWidget = GetWidget();
	if (!IsRegistered() || !GetWorld() || !GetWorld()->IsGameWorld() || !NameplateWidget || NameplateWidget->IsDesignTime())
	{
		return;
	}
	UMVVMView* View = NameplateWidget->GetExtension<UMVVMView>();
	if (View && BoundView == View && CharacterViewModel && View->GetViewModel(BoundSourceName).GetObject() == CharacterViewModel)
	{
		return;
	}
	if (!View || !View->GetViewClass())
	{
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	ReleaseViewModel();
	FName SourceName;
	for (const FMVVMViewClass_Source& Source : View->GetViewClass()->GetSources())
	{
		if (Source.GetSourceClass() == UWxViewModel_Character::StaticClass() && Source.CanBeSet())
		{
			SourceName = Source.GetName();
			break;
		}
	}
	if (SourceName.IsNone())
	{
		if (!bBindingErrorReported)
		{
			UE_LOG(LogWxUI, Warning, TEXT("Nameplate: 수동 주입 가능한 Character VM 소스가 없다(%s)."), *GetNameSafe(NameplateWidget->GetClass()));
			bBindingErrorReported = true;
		}
		return;
	}

	// 같은 ASC를 보는 다른 화면의 구독과 이미지 요청을 끊지 않도록 새 공유본만 초기화한다.
	UWxViewModel_Character* NewViewModel = Cast<UWxViewModel_Character>(UWxViewModel::FindSharedViewModel(ASC, UWxViewModel_Character::StaticClass()));
	if (!NewViewModel)
	{
		NewViewModel = NewObject<UWxViewModel_Character>(ASC);
		NewViewModel->Initialize(ASC, GetOwner());
	}
	if (View->SetViewModel(SourceName, NewViewModel))
	{
		CharacterViewModel = NewViewModel;
		BoundView = View;
		BoundSourceName = SourceName;
		bBindingErrorReported = false;
	}
}

void UWxNameplateComponent::ReleaseViewModel()
{
	if (UMVVMView* View = BoundView.Get())
	{
		if (View->GetViewModel(BoundSourceName).GetObject() == CharacterViewModel)
		{
			View->SetViewModel(BoundSourceName, nullptr);
		}
	}
	BoundView.Reset();
	BoundSourceName = NAME_None;
	// Character VM은 ASC별 공유본이므로 이 컴포넌트의 참조만 놓는다.
	CharacterViewModel = nullptr;
}

void UWxNameplateComponent::UpdatePresentation(const APawn* ViewerPawn)
{
	BindViewModel();
	UUserWidget* NameplateWidget = GetWidget();
	if (!ViewerPawn || !NameplateWidget || !CharacterViewModel || MaxVisibilityDistance <= 0.f)
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
