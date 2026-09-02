// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_Indicator.h"

void UWxViewModel_Indicator::SetProjection(float InDistanceMeters, bool bInClamped)
{
	if (DistanceMeters != InDistanceMeters)
	{
		DistanceMeters = InDistanceMeters;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DistanceMeters);
	}

	if (bClamped != bInClamped)
	{
		bClamped = bInClamped;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bClamped);
	}
}
