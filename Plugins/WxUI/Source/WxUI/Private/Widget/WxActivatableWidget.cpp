// Copyright Woogle. All Rights Reserved.

#include "Widget/WxActivatableWidget.h"

#include "Kismet/GameplayStatics.h"

static constexpr float MinTimeDilation = 0.001f;

void UWxActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	const float EffectiveDilation = FMath::Max(TimeDilation, MinTimeDilation);
	if (EffectiveDilation != 1.0f)
	{
		UWorld* World = GetWorld();
		if (World && World->IsNetMode(NM_Standalone))
		{
			const float Current = UGameplayStatics::GetGlobalTimeDilation(World);
			UGameplayStatics::SetGlobalTimeDilation(World, Current * EffectiveDilation);
		}
	}
}

void UWxActivatableWidget::NativeOnDeactivated()
{
	const float EffectiveDilation = FMath::Max(TimeDilation, MinTimeDilation);
	if (EffectiveDilation != 1.0f)
	{
		UWorld* World = GetWorld();
		if (World && World->IsNetMode(NM_Standalone))
		{
			const float Current = UGameplayStatics::GetGlobalTimeDilation(World);
			UGameplayStatics::SetGlobalTimeDilation(World, Current / EffectiveDilation);
		}
	}

	Super::NativeOnDeactivated();
}

TOptional<FUIInputConfig> UWxActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputMode)
	{
	case ECommonInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, MouseCaptureMode);
	case ECommonInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, MouseCaptureMode);
	default:
		return TOptional<FUIInputConfig>();
	}
}
