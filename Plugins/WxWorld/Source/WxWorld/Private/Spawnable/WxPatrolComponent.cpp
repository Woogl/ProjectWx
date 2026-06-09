// Copyright Woogle. All Rights Reserved.

#include "Spawnable/WxPatrolComponent.h"

TArray<FVector> UWxPatrolComponent::GetWorldPoints() const
{
	TArray<FVector> Points;

	const int32 NumPoints = GetNumberOfSplinePoints();
	Points.Reserve(NumPoints);
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		Points.Add(GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World));
	}
	return Points;
}

EWxPatrolMoveMode UWxPatrolComponent::GetMoveMode() const
{
	return MoveMode;
}
