// Copyright Woogle. All Rights Reserved.

#include "Widget/WxActivatableWidget.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

static constexpr float MinTimeDilation = 0.001f;

void UWxActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	ApplyGlobalTimeDilation(FMath::Max(TimeDilation, MinTimeDilation));
}

void UWxActivatableWidget::NativeOnDeactivated()
{
	ApplyGlobalTimeDilation(1.0f / FMath::Max(TimeDilation, MinTimeDilation));

	Super::NativeOnDeactivated();
}

void UWxActivatableWidget::NativeDestruct()
{
	// OnDeactivated 없이 파괴되는 비정상 경로 안전망. 정상 경로에선 이미 IsActivated()가 false라 자동 무시.
	if (IsActivated())
	{
		ApplyGlobalTimeDilation(1.0f / FMath::Max(TimeDilation, MinTimeDilation));
	}

	Super::NativeDestruct();
}

void UWxActivatableWidget::ApplyGlobalTimeDilation(float Dilation)
{
	if (FMath::IsNearlyEqual(Dilation, 1.0f))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		return;
	}

	const float Current = UGameplayStatics::GetGlobalTimeDilation(World);
	UGameplayStatics::SetGlobalTimeDilation(World, Current * Dilation);
}

TOptional<FUIInputConfig> UWxActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case ECommonInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	case ECommonInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently);
	default:
		return TOptional<FUIInputConfig>();
	}
}
