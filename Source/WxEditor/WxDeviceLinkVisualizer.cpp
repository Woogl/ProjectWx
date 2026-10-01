// Copyright Woogle. All Rights Reserved.

#include "WxDeviceLinkVisualizer.h"

#include "Device/WxDevice.h"
#include "SceneManagement.h"

void FWxDeviceLinkVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (!Component)
	{
		return;
	}

	const AWxDevice* Device = Cast<AWxDevice>(Component->GetOwner());
	if (!Device)
	{
		return;
	}

	const FVector Start = Device->GetActorLocation();
	for (const AWxDevice* LinkedDevice : Device->LinkedDevices)
	{
		if (!IsValid(LinkedDevice))
		{
			continue;
		}

		PDI->DrawLine(Start, LinkedDevice->GetActorLocation(), LinkColor, SDPG_Foreground, LinkThickness);
	}
}
