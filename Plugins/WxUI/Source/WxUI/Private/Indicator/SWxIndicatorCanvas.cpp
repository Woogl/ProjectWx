// Copyright Woogle. All Rights Reserved.

#include "SWxIndicatorCanvas.h"

#include "Components/SceneComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "Indicator/WxIndicatorManagerComponent.h"
#include "Indicator/WxIndicatorWidget.h"
#include "Layout/ArrangedChildren.h"
#include "SceneView.h"

namespace
{
	/** 클램프된 인디케이터와 화면 가장자리 사이에 둘 여백(px). */
	constexpr float WxIndicatorClampEdgePadding = 10.f;
}

SWxIndicatorCanvas::SWxIndicatorCanvas()
	: CanvasChildren(this)
{
}

void SWxIndicatorCanvas::Construct(const FArguments& InArgs, const FLocalPlayerContext& InLocalPlayerContext)
{
	LocalPlayerContext = InLocalPlayerContext;

	IndicatorPool.SetWorld(LocalPlayerContext.GetWorld());
	IndicatorPool.SetDefaultPlayerController(LocalPlayerContext.GetPlayerController());

	// 배치·투영은 Active Timer 가 돌린다(등록이 비면 멈출 수 있어 Tick 보다 싸다).
	SetCanTick(false);

	UpdateActiveTimer();
}

void SWxIndicatorCanvas::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SWxIndicatorCanvas_OnArrangeChildren);

	if (!bShowAnyIndicators)
	{
		return;
	}

	const FVector2D CanvasSize = FVector2D(AllottedGeometry.GetLocalSize());
	const FVector2D CanvasCenter = CanvasSize * 0.5;
	const FVector2D EdgePadding = FVector2D(WxIndicatorClampEdgePadding, WxIndicatorClampEdgePadding);

	// 가까운 인디케이터가 위에 오도록 먼 것부터 배치한다.
	TArray<const FSlot*> SortedSlots;
	SortedSlots.Reserve(CanvasChildren.Num());
	for (int32 ChildIndex = 0; ChildIndex < CanvasChildren.Num(); ++ChildIndex)
	{
		SortedSlots.Add(&CanvasChildren[ChildIndex]);
	}

	SortedSlots.StableSort([](const FSlot& A, const FSlot& B)
	{
		return A.GetDistanceToCamera() > B.GetDistanceToCamera();
	});

	for (const FSlot* CurrentSlot : SortedSlots)
	{
		if (!ArrangedChildren.Accepts(CurrentSlot->GetWidget()->GetVisibility()))
		{
			CurrentSlot->SetClamped(false);
			continue;
		}

		// 인디케이터는 투영 지점에 중앙을 맞추므로, 화면 안으로 당길 때 자기 크기의 절반만큼 더 들어와야 한다.
		const FVector2D SlotSize = CurrentSlot->GetWidget()->GetDesiredSize();
		const FVector2D SlotPadding = SlotSize * 0.5;
		const FVector2D ClampRectMin = SlotPadding + EdgePadding;
		const FVector2D ClampRectMax = CanvasSize - SlotPadding - EdgePadding;

		FVector2D ScreenPosition = CurrentSlot->GetScreenPosition();

		const bool bIsOutsideClampRect =
			ScreenPosition.X < ClampRectMin.X || ScreenPosition.X > ClampRectMax.X ||
			ScreenPosition.Y < ClampRectMin.Y || ScreenPosition.Y > ClampRectMax.Y;

		// 여백이 화면보다 커지는 극단적으로 작은 뷰포트에선 당길 곳이 없다.
		const bool bHasClampRect = ClampRectMin.X < ClampRectMax.X && ClampRectMin.Y < ClampRectMax.Y;

		bool bIsClamped = false;
		if (bIsOutsideClampRect && bHasClampRect)
		{
			const FPlane ClampPlanes[] =
			{
				FPlane(FVector(1.0, 0.0, 0.0), ClampRectMin.X),
				FPlane(FVector(0.0, 1.0, 0.0), ClampRectMin.Y),
				FPlane(FVector(-1.0, 0.0, 0.0), -ClampRectMax.X),
				FPlane(FVector(0.0, -1.0, 0.0), -ClampRectMax.Y)
			};

			for (const FPlane& ClampPlane : ClampPlanes)
			{
				FVector EdgePoint;
				if (!FMath::SegmentPlaneIntersection(FVector(CanvasCenter, 0.0), FVector(ScreenPosition, 0.0), ClampPlane, EdgePoint))
				{
					continue;
				}

				// 모서리 밖 대상은 두 평면과 함께 교차하므로, 사각형 경계 위에 놓인 교차점만 실제 이탈 지점이다.
				const FVector2D EdgePosition(EdgePoint);
				if (EdgePosition.X < ClampRectMin.X - 1.0 || EdgePosition.X > ClampRectMax.X + 1.0 ||
					EdgePosition.Y < ClampRectMin.Y - 1.0 || EdgePosition.Y > ClampRectMax.Y + 1.0)
				{
					continue;
				}

				ScreenPosition = EdgePosition;
				bIsClamped = true;
				break;
			}
		}

		CurrentSlot->SetClamped(bIsClamped);

		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(CurrentSlot->GetWidget(), ScreenPosition - SlotPadding, SlotSize, 1.f));
	}
}

FVector2D SWxIndicatorCanvas::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	// 인디케이터는 HUD 레이아웃에 크기를 요구하지 않는다 — 주어진 화면 전체를 좌표계로만 쓴다.
	return FVector2D::ZeroVector;
}

FChildren* SWxIndicatorCanvas::GetChildren()
{
	return &CanvasChildren;
}

int32 SWxIndicatorCanvas::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SWxIndicatorCanvas_OnPaint);

	// 투영에 필요한 화면 크기는 여기서만 알 수 있다.
	OptionalPaintGeometry = AllottedGeometry;

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	ArrangeChildren(AllottedGeometry, ArrangedChildren);

	int32 MaxLayerId = LayerId;
	const FPaintArgs NewArgs = Args.WithNewParent(this);
	const bool bShouldBeEnabled = ShouldBeEnabled(bParentEnabled);

	for (const FArrangedWidget& CurrentWidget : ArrangedChildren.GetInternalArray())
	{
		if (IsChildWidgetCulled(MyCullingRect, CurrentWidget))
		{
			continue;
		}

		// 모든 인디케이터를 같은 레이어에 그려 배칭을 유지한다(겹침 순서는 배치 순서가 정한다).
		const int32 CurrentWidgetMaxLayerId = CurrentWidget.Widget->Paint(NewArgs, CurrentWidget.Geometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bShouldBeEnabled);
		MaxLayerId = FMath::Max(MaxLayerId, CurrentWidgetMaxLayerId);
	}

	return MaxLayerId;
}

void SWxIndicatorCanvas::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObjects(AllIndicators);
	IndicatorPool.AddReferencedObjects(Collector);
}

FString SWxIndicatorCanvas::GetReferencerName() const
{
	return TEXT("SWxIndicatorCanvas");
}

bool SWxIndicatorCanvas::TryBindManager()
{
	if (ManagerPtr.IsValid())
	{
		return true;
	}

	APlayerController* PlayerController = LocalPlayerContext.GetPlayerController();
	UWxIndicatorManagerComponent* Manager = PlayerController ? PlayerController->FindComponentByClass<UWxIndicatorManagerComponent>() : nullptr;
	if (!Manager)
	{
		return false;
	}

	// 위젯 풀은 매니저를 찾은 시점의 월드·컨트롤러에 맞춘다(PIE 재시작 등으로 바뀌었을 수 있다).
	IndicatorPool.SetWorld(LocalPlayerContext.GetWorld());
	IndicatorPool.SetDefaultPlayerController(PlayerController);

	ManagerPtr = Manager;
	Manager->OnIndicatorAdded.AddSP(this, &SWxIndicatorCanvas::HandleIndicatorAdded);
	Manager->OnIndicatorRemoved.AddSP(this, &SWxIndicatorCanvas::HandleIndicatorRemoved);

	// 캔버스가 뒤늦게 만들어졌을 수 있으므로 이미 등록된 것들을 따라잡는다.
	for (const TObjectPtr<UWxIndicatorDescriptor>& Indicator : Manager->GetIndicators())
	{
		HandleIndicatorAdded(Indicator);
	}

	return true;
}

void SWxIndicatorCanvas::HandleIndicatorAdded(UWxIndicatorDescriptor* Indicator)
{
	AllIndicators.Add(Indicator);

	RequestIndicatorWidget(Indicator);
}

void SWxIndicatorCanvas::HandleIndicatorRemoved(UWxIndicatorDescriptor* Indicator)
{
	PendingWidgetClassLoads.Remove(Indicator);
	RemoveIndicatorSlot(Indicator);

	AllIndicators.Remove(Indicator);
}

void SWxIndicatorCanvas::HandleIndicatorWidgetClassLoaded(UWxIndicatorDescriptor* Indicator)
{
	// 로드를 기다리는 동안 해제됐으면 대기 목록에서 이미 빠졌다 — 늦게 도착한 통지는 버린다(포인터를 역참조하지 않는다).
	if (PendingWidgetClassLoads.Remove(Indicator) == 0)
	{
		return;
	}

	CreateIndicatorWidget(Indicator);
}

EActiveTimerReturnType SWxIndicatorCanvas::HandleActiveTimer(double InCurrentTime, float InDeltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SWxIndicatorCanvas_HandleActiveTimer);

	// 한 번도 페인트되지 않았으면 화면 크기를 몰라 투영할 수 없다.
	if (!OptionalPaintGeometry.IsSet())
	{
		return EActiveTimerReturnType::Continue;
	}

	// 매니저는 컨트롤러에 주입되므로 HUD 보다 늦게 붙을 수 있다. 찾을 때까지는 계속 돈다.
	if (!TryBindManager())
	{
		SetShowAnyIndicators(false);
		return EActiveTimerReturnType::Continue;
	}

	ULocalPlayer* LocalPlayer = LocalPlayerContext.GetLocalPlayer();
	FSceneViewProjectionData ProjectionData;
	if (!LocalPlayer || !LocalPlayer->ViewportClient || !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
	{
		SetShowAnyIndicators(false);
		return EActiveTimerReturnType::Continue;
	}

	SetShowAnyIndicators(true);

	const FVector2f ScreenSize = FVector2f(OptionalPaintGeometry.GetValue().GetLocalSize());
	bool bIndicatorsChanged = false;

	for (int32 ChildIndex = 0; ChildIndex < CanvasChildren.Num(); ++ChildIndex)
	{
		FSlot& CurrentSlot = CanvasChildren[ChildIndex];

		FVector2D ScreenPosition;
		double DistanceToCamera = 0.0;
		if (!ProjectIndicator(*CurrentSlot.GetIndicator(), ProjectionData, ScreenSize, ScreenPosition, DistanceToCamera))
		{
			// 대상이 파괴되면 표시만 접는다 — 등록증 제거는 등록한 쪽의 몫이다.
			CurrentSlot.SetIndicatorVisible(false);

			bIndicatorsChanged |= CurrentSlot.IsDirty();
			CurrentSlot.ClearDirtyFlag();
			continue;
		}

		CurrentSlot.SetIndicatorVisible(true);
		CurrentSlot.SetScreenPosition(ScreenPosition);
		CurrentSlot.SetDistanceToCamera(DistanceToCamera);

		const float DistanceMeters = static_cast<float>(DistanceToCamera) / 100.f;
		if (CurrentSlot.NeedsWidgetRefresh(DistanceMeters))
		{
			if (UWxIndicatorWidget* IndicatorWidget = CurrentSlot.GetIndicatorWidget())
			{
				IndicatorWidget->OnIndicatorRefreshed(DistanceMeters, CurrentSlot.IsClamped());
			}
			CurrentSlot.MarkWidgetRefreshed(DistanceMeters);
		}

		bIndicatorsChanged |= CurrentSlot.IsDirty();
		CurrentSlot.ClearDirtyFlag();
	}

	if (bIndicatorsChanged)
	{
		Invalidate(EInvalidateWidget::Paint);
	}

	// 등록이 비면 멈춘다. 매니저는 HUD 와 같은 컨트롤러가 소유하므로 HUD 가 사는 동안 사라지지 않고,
	// 다음 등록은 구독한 통지로 들어와 타이머를 되살린다.
	if (AllIndicators.IsEmpty())
	{
		TickHandle.Reset();
		return EActiveTimerReturnType::Stop;
	}

	return EActiveTimerReturnType::Continue;
}

void SWxIndicatorCanvas::RequestIndicatorWidget(UWxIndicatorDescriptor* Indicator)
{
	const TSoftClassPtr<UWxIndicatorWidget> IndicatorWidgetClass = Indicator->GetIndicatorWidgetClass();

	// 이미 로드돼 있으면 스트리밍을 거치지 않는다(같은 위젯을 쓰는 두 번째 인디케이터 등).
	if (IndicatorWidgetClass.Get())
	{
		CreateIndicatorWidget(Indicator);
		return;
	}

	PendingWidgetClassLoads.Add(Indicator, UAssetManager::GetStreamableManager().RequestAsyncLoad(
		IndicatorWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateSP(this, &SWxIndicatorCanvas::HandleIndicatorWidgetClassLoaded, Indicator)));
}

void SWxIndicatorCanvas::CreateIndicatorWidget(UWxIndicatorDescriptor* Indicator)
{
	UClass* IndicatorWidgetClass = Indicator->GetIndicatorWidgetClass().Get();
	if (!IndicatorWidgetClass)
	{
		return;
	}

	UWxIndicatorWidget* IndicatorWidget = IndicatorPool.GetOrCreateInstance(TSubclassOf<UWxIndicatorWidget>(IndicatorWidgetClass));
	if (!IndicatorWidget)
	{
		return;
	}

	AddIndicatorSlot(Indicator, IndicatorWidget);
}

void SWxIndicatorCanvas::AddIndicatorSlot(UWxIndicatorDescriptor* Indicator, UWxIndicatorWidget* IndicatorWidget)
{
	{
		FScopedWidgetSlotArguments SlotArguments{ MakeUnique<FSlot>(Indicator, IndicatorWidget), CanvasChildren, INDEX_NONE };
		SlotArguments
		[
			IndicatorWidget->TakeWidget()
		];
	}

	// 슬롯은 위 스코프를 벗어날 때 실제로 삽입되므로, 타이머 재개는 그 뒤에 요청한다.
	UpdateActiveTimer();
}

void SWxIndicatorCanvas::RemoveIndicatorSlot(UWxIndicatorDescriptor* Indicator)
{
	for (int32 ChildIndex = 0; ChildIndex < CanvasChildren.Num(); ++ChildIndex)
	{
		if (CanvasChildren[ChildIndex].GetIndicator() != Indicator)
		{
			continue;
		}

		if (UWxIndicatorWidget* IndicatorWidget = CanvasChildren[ChildIndex].GetIndicatorWidget())
		{
			IndicatorPool.Release(IndicatorWidget);
		}

		CanvasChildren.RemoveAt(ChildIndex);
		return;
	}
}

void SWxIndicatorCanvas::SetShowAnyIndicators(bool bInShowAnyIndicators)
{
	if (bShowAnyIndicators == bInShowAnyIndicators)
	{
		return;
	}

	bShowAnyIndicators = bInShowAnyIndicators;

	if (!bShowAnyIndicators)
	{
		for (int32 ChildIndex = 0; ChildIndex < CanvasChildren.Num(); ++ChildIndex)
		{
			CanvasChildren.GetChildAt(ChildIndex)->SetVisibility(EVisibility::Collapsed);
		}
	}
}

void SWxIndicatorCanvas::UpdateActiveTimer()
{
	// 매니저를 못 찾은 동안은 찾기 위해 돌아야 하고, 찾은 뒤엔 등록이 있을 때만 돈다.
	const bool bNeedsTicks = !AllIndicators.IsEmpty() || !ManagerPtr.IsValid();
	if (bNeedsTicks && !TickHandle.IsValid())
	{
		TickHandle = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SWxIndicatorCanvas::HandleActiveTimer));
	}
}

bool SWxIndicatorCanvas::ProjectIndicator(const UWxIndicatorDescriptor& Indicator, const FSceneViewProjectionData& ProjectionData, const FVector2f& ScreenSize, FVector2D& OutScreenPosition, double& OutDistanceToCamera) const
{
	const USceneComponent* TargetComponent = Indicator.GetTargetComponent();
	if (!IsValid(TargetComponent))
	{
		return false;
	}

	const FVector WorldLocation = TargetComponent->GetComponentLocation() + Indicator.GetWorldOffset();

	FVector2D ScreenPosition;
	const bool bIsInFrontOfCamera = ULocalPlayer::GetPixelPoint(ProjectionData, WorldLocation, ScreenPosition, &ScreenSize);

	// 카메라 뒤 대상은 투영 좌표가 화면 안으로 접혀 들어온다. 화면 중심에서 그 방향으로 화면 밖까지 밀어내
	// 클램프가 등 뒤를 가리키는 가장자리를 잡게 한다(밀어내지 않으면 뒤쪽 목표가 화면 중앙에 뜬다).
	if (!bIsInFrontOfCamera && FBox2f(FVector2f::Zero(), ScreenSize).IsInside(FVector2f(ScreenPosition)))
	{
		const FVector2f ScreenCenter = ScreenSize * 0.5f;
		const FVector2f CenterToPosition = (FVector2f(ScreenPosition) - ScreenCenter).GetSafeNormal();
		ScreenPosition = FVector2D(ScreenCenter + CenterToPosition * ScreenSize);
	}

	OutScreenPosition = ScreenPosition;
	OutDistanceToCamera = FVector::Dist(ProjectionData.ViewOrigin, WorldLocation);

	return true;
}
