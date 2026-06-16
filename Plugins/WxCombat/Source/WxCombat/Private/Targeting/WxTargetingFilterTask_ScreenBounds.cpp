// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_ScreenBounds.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_ScreenBounds::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return false;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceContext->SourceActor);
	const APlayerController* PlayerController = SourcePawn ? Cast<APlayerController>(SourcePawn->GetController()) : nullptr;
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!PlayerController || !TargetActor)
	{
		return false;
	}

	// 타겟 위치를 화면 좌표로 투영한다. 실패하면(카메라 뒤) 화면 밖으로 보고 제외한다.
	FVector2D ScreenPosition;
	if (!PlayerController->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenPosition))
	{
		return true;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return false;
	}

	// 가장자리 여백만큼 안쪽으로 좁힌 영역을 벗어나면 제외한다.
	const float MarginX = ViewportSizeX * ScreenEdgeMargin;
	const float MarginY = ViewportSizeY * ScreenEdgeMargin;
	const bool bInsideScreen =
		ScreenPosition.X >= MarginX && ScreenPosition.X <= ViewportSizeX - MarginX &&
		ScreenPosition.Y >= MarginY && ScreenPosition.Y <= ViewportSizeY - MarginY;

	return !bInsideScreen;
}
