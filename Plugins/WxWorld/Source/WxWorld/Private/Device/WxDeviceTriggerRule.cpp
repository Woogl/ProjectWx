// Copyright Woogle. All Rights Reserved.

#include "Device/WxDeviceTriggerRule.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Device/WxDevice.h"

#define LOCTEXT_NAMESPACE "WxDeviceTriggerRule"

FWxDeviceTriggerRule::~FWxDeviceTriggerRule() = default;

void FWxDeviceTriggerRule::GetAcceptedOptions(const AWxDevice& Receiver, const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const
{
}

void FWxDeviceTriggerRule_SplineStops::GetAcceptedOptions(const AWxDevice& Receiver, const AWxDevice* Sender, TArray<FWxInteractionOption>& OutOptions) const
{
	const USceneComponent* PlatformComponent = Platform.Resolve(&Receiver);
	const USplineComponent* SplineComponent = Cast<USplineComponent>(Spline.Resolve(&Receiver));
	if (!Sender || !Sender->GetRootComponent() || !PlatformComponent || !SplineComponent)
	{
		return;
	}

	const int32 NumStops = SplineComponent->GetNumberOfSplinePoints();
	if (NumStops < 2)
	{
		return;
	}

	// 스플라인 포인트의 입력 키가 곧 포인트 번호라, 가장 가까운 키를 반올림하면 가장 가까운 정차 지점이다.
	const int32 CurrentStop = FMath::Clamp(FMath::RoundToInt(SplineComponent->FindInputKeyClosestToWorldLocation(PlatformComponent->GetComponentLocation())), 0, NumStops - 1);

	if (Sender->GetRootComponent()->IsAttachedTo(PlatformComponent))
	{
		for (int32 Stop = 0; Stop < NumStops; ++Stop)
		{
			if (Stop != CurrentStop || bAcceptCurrentStop)
			{
				OutOptions.Add({FText::Format(LOCTEXT("FloorOption", "{0}층"), FText::AsNumber(Stop + 1)), Stop});
			}
		}

		return;
	}

	const int32 NearestStop = FMath::Clamp(FMath::RoundToInt(SplineComponent->FindInputKeyClosestToWorldLocation(Sender->GetActorLocation())), 0, NumStops - 1);
	if (NearestStop != CurrentStop || bAcceptCurrentStop)
	{
		OutOptions.Add({FText::GetEmpty(), NearestStop});
	}
}

#undef LOCTEXT_NAMESPACE
