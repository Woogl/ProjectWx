// Copyright Woogle. All Rights Reserved.

#include "WxWaterToolset.h"

#include "Kismet/KismetSystemLibrary.h"
#include "WaterBodyActor.h"
#include "WaterSplineComponent.h"
#include "WaterSplineMetadata.h"

bool UWxWaterToolset::SetWaterBodySpline(AWaterBody* WaterBody, const TArray<FVector>& Points, const TArray<float>& Widths, const TArray<float>& Depths, bool bLinear)
{
	if (!WaterBody || Points.Num() < 2 || (!Widths.IsEmpty() && Widths.Num() != Points.Num()) || (!Depths.IsEmpty() && Depths.Num() != Points.Num()))
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("물 바디가 없거나 점·폭·깊이 개수가 맞지 않는다."));
		return false;
	}

	UWaterSplineComponent* Spline = WaterBody->GetWaterSpline();
	UWaterSplineMetadata* Metadata = WaterBody->GetWaterSplineMetadata();
	Spline->Modify();
	Metadata->Modify();

	// 점을 하나씩 더하면 위치·회전·배율과 메타데이터 점 개수가 함께 늘어난다.
	Spline->ClearSplinePoints(false);
	const ESplinePointType::Type PointType = bLinear ? ESplinePointType::Linear : ESplinePointType::Curve;
	for (const FVector& Point : Points)
	{
		Spline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
		Spline->SetSplinePointType(Spline->GetNumberOfSplinePoints() - 1, PointType, false);
	}
	Spline->UpdateSpline();

	for (int32 Index = 0; Index < Widths.Num(); ++Index)
	{
		Metadata->RiverWidth.Points[Index].OutVal = Widths[Index];
	}
	for (int32 Index = 0; Index < Depths.Num(); ++Index)
	{
		Metadata->Depth.Points[Index].OutVal = Depths[Index];
	}

	// K2_SynchronizeAndBroadcastDataChange 는 Interactive 변경으로 알려 물 메시·물 정보 텍스처를 다시 만들지 않는다 — 디테일 패널 편집과 같은 경로로 알린다.
	Spline->PostEditChange();
	return true;
}
