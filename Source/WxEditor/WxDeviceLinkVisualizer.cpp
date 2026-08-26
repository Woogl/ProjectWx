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

	// 등록 키가 엔진 StateTree 컴포넌트라 장치가 아닌 액터의 것도 여기까지 온다.
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
