// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Indicator.h"

void UWxViewModel_Indicator::SetProjection(float InCameraDistance, bool bInClamped)
{
	if (CameraDistance != InCameraDistance)
	{
		CameraDistance = InCameraDistance;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CameraDistance);
	}

	if (bClamped != bInClamped)
	{
		bClamped = bInClamped;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bClamped);
	}
}
