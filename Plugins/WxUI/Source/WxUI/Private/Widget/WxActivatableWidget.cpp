// Copyright Woogle. All Rights Reserved.

#include "Widget/WxActivatableWidget.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UWxActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (bPauseGame)
	{
		ApplyGamePause(true);
	}
}

void UWxActivatableWidget::NativeOnDeactivated()
{
	if (bPauseGame)
	{
		ApplyGamePause(false);
	}

	Super::NativeOnDeactivated();
}

void UWxActivatableWidget::NativeDestruct()
{
	// OnDeactivated 없이 파괴되는 비정상 경로 안전망. 정상 경로에선 이미 IsActivated()가 false라 자동 무시.
	if (bPauseGame && IsActivated())
	{
		ApplyGamePause(false);
	}

	Super::NativeDestruct();
}

void UWxActivatableWidget::ApplyGamePause(bool bPaused)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsNetMode(NM_Standalone))
	{
		return;
	}

	UGameplayStatics::SetGamePaused(World, bPaused);
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
