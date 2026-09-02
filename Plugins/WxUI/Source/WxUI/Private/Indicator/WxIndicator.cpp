// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicator.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CoreMisc.h"
#include "MVVM/WxViewModel_Indicator.h"
#include "SceneView.h"
#include "View/MVVMView.h"
#include "WxUIModule.h"

AWxIndicator::AWxIndicator()
{
	PrimaryActorTick.bCanEverTick = true;

	// 카메라 갱신은 틱 그룹이 아니라 월드가 액터 틱을 마친 뒤 직접 돌리므로(TG_PostUpdateWork 직전), 그보다 뒤라야 같은 프레임의 카메라로 투영한다.
	// 앞서 돌면 표시가 카메라를 한 프레임 늦게 따라간다.
	PrimaryActorTick.TickGroup = TG_LastDemotable;

	IndicatorWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("IndicatorWidget"));
	RootComponent = IndicatorWidget;

	IndicatorWidget->SetWidgetSpace(EWidgetSpace::Screen);
	IndicatorWidget->SetDrawAtDesiredSize(true);
	IndicatorWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWxIndicator::BeginPlay()
{
	// 위젯은 이 안에서 만들어진다(위젯 컴포넌트의 BeginPlay).
	Super::BeginPlay();

	// 스폰 위치가 곧 첫 앵커다 — 대상에 붙기 전까지 여기를 가리킨다.
	AnchorLocation = GetActorLocation();

	BindViewModel();
}

void AWxIndicator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateProjection();
}

void AWxIndicator::Initialize(float InZOffset)
{
	ZOffset = InZOffset;
}

void AWxIndicator::SetTarget(AActor* InTarget)
{
	// 루트는 이미 표시 위치를 떠돌고 있어 지켜 봐야 의미 없는 트랜스폼이지만, 첫 틱 전까지 엉뚱한 곳으로 튀지는 않게 둔다.
	AttachToActor(InTarget, FAttachmentTransformRules::KeepWorldTransform);
}

bool AWxIndicator::HasTarget() const
{
	return GetAttachParentActor() != nullptr;
}

void AWxIndicator::BindViewModel()
{
	// 엔진은 데디 서버이거나 Slate 가 없으면 위젯을 만들지 않는다(UWidgetComponent::InitWidget).
	// 둘 다 위젯이 없는 게 정상인 환경이라, 아래 위젯 부재 경고가 오경보로 울리지 않도록 그 앞에서 빠진다.
	if (IsRunningDedicatedServer() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	UUserWidget* Widget = IndicatorWidget->GetWidget();
	if (!Widget)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Indicator: 위젯이 없어 아무것도 표시되지 않는다(%s). 위젯 컴포넌트의 WidgetClass 지정을 확인한다."), *GetNameSafe(GetClass()));
		return;
	}

	UMVVMView* View = Widget->GetExtension<UMVVMView>();
	if (!View)
	{
		UE_LOG(LogWxUI, Warning, TEXT("Indicator: 위젯에 MVVM View 확장이 없어 뷰모델을 묶을 수 없다(%s)."), *GetNameSafe(Widget->GetClass()));
		return;
	}

	UWxViewModel_Indicator* NewViewModel = NewObject<UWxViewModel_Indicator>(this);
	if (!View->SetViewModelByClass(NewViewModel))
	{
		// 실패해도 위젯은 그대로 뜨므로(값만 안 들어옴) 로그가 없으면 원인을 짚을 수가 없다.
		UE_LOG(LogWxUI, Warning, TEXT("Indicator: 위젯의 뷰모델 소스에 값을 넣지 못했다(%s). 소스가 수동 지정(Creation Type: Manual)인지 확인한다."), *GetNameSafe(Widget->GetClass()));
		return;
	}

	ViewModel = NewViewModel;
}

void AWxIndicator::UpdateProjection()
{
	// 부착 중이면 대상이 곧 앵커다.
	// 대상이 파괴되면 엔진이 부착을 풀며 루트를 그 자리(주차 좌표)에 남기므로, 앵커는 트랜스폼이 아니라 여기서 따로 붙들어 둔다.
	if (const USceneComponent* AttachParent = GetRootComponent()->GetAttachParent())
	{
		AnchorLocation = AttachParent->GetComponentLocation();
	}

	const FVector AnchorWorldLocation = AnchorLocation + FVector(0.f, 0.f, ZOffset);

	// 위젯이 그려질 뷰와 같은 로컬 플레이어로 투영해야 좌표가 어긋나지 않는다.
	const ULocalPlayer* LocalPlayer = IndicatorWidget->GetOwnerPlayer();

	FSceneViewProjectionData ProjectionData;
	if (!LocalPlayer || !LocalPlayer->ViewportClient || !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
	{
		IndicatorWidget->SetVisibility(false);
		return;
	}

	IndicatorWidget->SetVisibility(true);

	const FIntRect ViewRect = ProjectionData.GetConstrainedViewRect();

	// 뷰 크기를 넘겨 좌표계 원점을 (0,0) 으로 맞춘다.
	// 엔진은 카메라 뒤 좌표를 접을 때 중심이 아니라 Max 의 절반을 기준으로 뒤집으므로(ULocalPlayer::GetPixelPoint),
	// 레터박스처럼 뷰가 화면 원점에서 떨어져 있으면 원점을 옮겨 주지 않는 한 접힌 좌표가 그만큼 어긋난다.
	const FVector2f ViewSize(ViewRect.Size());

	FVector2D PixelPosition;
	const bool bIsInFrontOfCamera = ULocalPlayer::GetPixelPoint(ProjectionData, AnchorWorldLocation, PixelPosition, &ViewSize);

	// 카메라 뒤 대상은 투영 좌표가 화면 안으로 접혀 들어온다.
	// 화면 중심에서 그 방향으로 화면 밖까지 밀어내 클램프가 등 뒤를 가리키는 가장자리를 잡게 한다(밀어내지 않으면 뒤쪽 목표가 화면 중앙에 뜬다).
	if (!bIsInFrontOfCamera && FBox2D(FVector2D::ZeroVector, FVector2D(ViewSize)).IsInside(PixelPosition))
	{
		const FVector2D ViewCenter = FVector2D(ViewSize) * 0.5;

		// 정확히 등 뒤면 접힌 좌표가 중심에 얹혀 방향이 서지 않는다. 아래로 떨어뜨려 화면 아래 가장자리를 잡게 한다.
		FVector2D CenterToPosition = (PixelPosition - ViewCenter).GetSafeNormal();
		if (CenterToPosition.IsZero())
		{
			CenterToPosition = FVector2D(0.0, 1.0);
		}

		PixelPosition = ViewCenter + CenterToPosition * FVector2D(ViewSize);
	}

	// 여백은 위젯이 놓이는 슬레이트 단위로 저작하고, 픽셀 공간인 여기서만 환산한다.
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), UE_KINDA_SMALL_NUMBER);
	const FVector2D MarginPixels(ScreenMargin * ViewportScale);
	const bool bClamped = ClampToScreenEdge(MarginPixels, FVector2D(ViewSize) - MarginPixels, PixelPosition);

	const double DistanceToCamera = FVector::Dist(ProjectionData.ViewOrigin, AnchorWorldLocation);

	if (bClamped)
	{
		// 엔진의 스크린 스페이스 레이어는 카메라 뒤 대상을 아예 숨기고 투영 후 보정 훅도 주지 않는다.
		// 그래서 당긴 화면 좌표로 투영되는 월드 지점을 역으로 구해 액터를 그리로 옮긴다 — 실제 대상 거리에 놓아 위젯끼리의 앞뒤 정렬도 그대로 둔다.
		// 역투영은 뷰가 아니라 화면 원점 기준 좌표를 받으므로, 옮겨 뒀던 원점을 되돌린다.
		FVector RayOrigin;
		FVector RayDirection;
		FSceneView::DeprojectScreenToWorld(PixelPosition + FVector2D(ViewRect.Min), ViewRect, ProjectionData.ComputeViewProjectionMatrix().Inverse(), RayOrigin, RayDirection);

		SetActorLocation(RayOrigin + RayDirection * DistanceToCamera);
	}
	else
	{
		SetActorLocation(AnchorWorldLocation);
	}

	if (ViewModel)
	{
		// 미세한 거리 변화까지 흘리면 매 프레임 바인딩이 돈다.
		ViewModel->SetProjection(FMath::RoundToFloat(static_cast<float>(DistanceToCamera) / 100.f), bClamped);
	}
}

bool AWxIndicator::ClampToScreenEdge(const FVector2D& ClampRectMin, const FVector2D& ClampRectMax, FVector2D& InOutPixelPosition) const
{
	// 여백이 화면보다 커지는 극단적으로 작은 뷰포트에선 당길 곳이 없다.
	if (ClampRectMin.X >= ClampRectMax.X || ClampRectMin.Y >= ClampRectMax.Y)
	{
		return false;
	}

	const bool bIsOutsideClampRect =
		InOutPixelPosition.X < ClampRectMin.X || InOutPixelPosition.X > ClampRectMax.X ||
		InOutPixelPosition.Y < ClampRectMin.Y || InOutPixelPosition.Y > ClampRectMax.Y;

	if (!bIsOutsideClampRect)
	{
		return false;
	}

	const FVector2D ClampRectCenter = (ClampRectMin + ClampRectMax) * 0.5;
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
		if (!FMath::SegmentPlaneIntersection(FVector(ClampRectCenter, 0.0), FVector(InOutPixelPosition, 0.0), ClampPlane, EdgePoint))
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

		InOutPixelPosition = EdgePosition;
		return true;
	}

	return false;
}
