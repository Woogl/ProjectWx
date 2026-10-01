// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModel_Indicator.h"

void UWxViewModel_Indicator::SetCameraDistance(float InDistanceMeters)
{
	UE_MVVM_SET_PROPERTY_VALUE(CameraDistance, InDistanceMeters);
}
