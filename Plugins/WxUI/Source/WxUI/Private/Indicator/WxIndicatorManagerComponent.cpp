// Copyright Woogle. All Rights Reserved.

#include "Indicator/WxIndicatorManagerComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/SceneComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Indicator/WxIndicatorDescriptor.h"
#include "SceneView.h"

namespace
{
	/**
	 * 클램프된 인디케이터와 화면 가장자리 사이에 둘 여백(슬레이트 단위).
	 * 위젯의 중심 좌표를 당기므로 "아이콘 절반 + 여백" 만큼이 필요하다 — 아이콘을 크게 바꾸면 이 값도 함께 키운다.
	 */
	constexpr float WxIndicatorScreenMargin = 48.f;

	/** 대상의 월드 위치를 화면 좌표와 카메라 거리로 바꾼다. 대상이 무효면 false. */
	bool ProjectIndicator(const UWxIndicatorDescriptor& Indicator, const FSceneViewProjectionData& ProjectionData, const FVector2f& ScreenSize, FVector2D& OutScreenPosition, double& OutDistanceToCamera)
	{
		const USceneComponent* TargetComponent = Indicator.GetTargetComponent();
		if (!IsValid(TargetComponent))
		{
			return false;
		}

		const FVector WorldLocation = TargetComponent->GetComponentLocation() + Indicator.GetWorldOffset();

		FVector2D ScreenPosition;
		const bool bIsInFrontOfCamera = ULocalPlayer::GetPixelPoint(ProjectionData, WorldLocation, ScreenPosition, &ScreenSize);

		// 카메라 뒤 대상은 투영 좌표가 화면 안으로 접혀 들어온다.
		// 화면 중심에서 그 방향으로 화면 밖까지 밀어내 클램프가 등 뒤를 가리키는 가장자리를 잡게 한다(밀어내지 않으면 뒤쪽 목표가 화면 중앙에 뜬다).
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

	/** 화면 밖 좌표를 화면 가장자리로 당긴다. 당겼으면 true — 위젯이 화면 밖임을 알 수 있다. */
	bool ClampToScreen(const FVector2D& ScreenSize, FVector2D& InOutScreenPosition)
	{
		const FVector2D ClampRectMin(WxIndicatorScreenMargin, WxIndicatorScreenMargin);
		const FVector2D ClampRectMax = ScreenSize - ClampRectMin;

		// 여백이 화면보다 커지는 극단적으로 작은 뷰포트에선 당길 곳이 없다.
		if (ClampRectMin.X >= ClampRectMax.X || ClampRectMin.Y >= ClampRectMax.Y)
		{
			return false;
		}

		const bool bIsOutsideClampRect =
			InOutScreenPosition.X < ClampRectMin.X || InOutScreenPosition.X > ClampRectMax.X ||
			InOutScreenPosition.Y < ClampRectMin.Y || InOutScreenPosition.Y > ClampRectMax.Y;

		if (!bIsOutsideClampRect)
		{
			return false;
		}

		const FVector2D ScreenCenter = ScreenSize * 0.5;
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
			if (!FMath::SegmentPlaneIntersection(FVector(ScreenCenter, 0.0), FVector(InOutScreenPosition, 0.0), ClampPlane, EdgePoint))
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

			InOutScreenPosition = EdgePosition;
			return true;
		}

		return false;
	}
}

FWxOnIndicatorManagerReady UWxIndicatorManagerComponent::OnAnyManagerReady;

UWxIndicatorManagerComponent::UWxIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	// 등록이 있을 때만 돈다. 투영할 것이 없으면 카메라를 볼 이유도 없다.
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 카메라 매니저(TG_PostUpdateWork)보다 뒤라야 같은 프레임의 카메라로 투영한다. 앞서 돌면 표시가 카메라를 한 프레임 늦게 따라간다.
	PrimaryComponentTick.TickGroup = TG_LastDemotable;
}

void UWxIndicatorManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 등록을 받지 않는 원격 사본은 관찰자에게 알릴 것도 없다.
	const APlayerController* OwningController = GetController<APlayerController>();
	if (OwningController && OwningController->IsLocalController())
	{
		OnAnyManagerReady.Broadcast(this);
	}
}

void UWxIndicatorManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateProjections();

	OnIndicatorsUpdated.Broadcast();
}

UWxIndicatorDescriptor* UWxIndicatorManagerComponent::AddIndicator(USceneComponent* InTargetComponent, const FVector& InWorldOffset)
{
	if (!IsValid(InTargetComponent))
	{
		return nullptr;
	}

	const APlayerController* OwningController = GetController<APlayerController>();
	if (!OwningController || !OwningController->IsLocalController())
	{
		return nullptr;
	}

	UWxIndicatorDescriptor* Indicator = NewObject<UWxIndicatorDescriptor>(this);
	Indicator->Initialize(this, InTargetComponent, InWorldOffset);

	Indicators.Add(Indicator);
	UpdateTickEnabled();

	// 첫 틱을 기다리면 한 프레임 동안 좌표 없이 뜬다. 등록 즉시 한 번 계산해 곧바로 제자리에 뜨게 한다.
	UpdateProjections();
	OnIndicatorsUpdated.Broadcast();

	return Indicator;
}

void UWxIndicatorManagerComponent::RemoveIndicator(UWxIndicatorDescriptor* Indicator)
{
	if (Indicators.Remove(Indicator) == 0)
	{
		return;
	}

	UpdateTickEnabled();

	UpdateProjections();
	OnIndicatorsUpdated.Broadcast();
}

const TArray<TObjectPtr<UWxIndicatorDescriptor>>& UWxIndicatorManagerComponent::GetIndicators() const
{
	return Indicators;
}

void UWxIndicatorManagerComponent::UpdateProjections()
{
	const APlayerController* OwningController = GetController<APlayerController>();
	const ULocalPlayer* LocalPlayer = OwningController ? OwningController->GetLocalPlayer() : nullptr;

	FSceneViewProjectionData ProjectionData;
	if (!LocalPlayer || !LocalPlayer->ViewportClient || !LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
	{
		for (const TObjectPtr<UWxIndicatorDescriptor>& Indicator : Indicators)
		{
			Indicator->ClearProjection();
		}
		return;
	}

	// 위젯이 쓰는 슬레이트 좌표로 투영한다 — 뷰포트 픽셀 크기를 DPI 스케일로 나눈 값이 전체화면 위젯의 로컬 크기다.
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), UE_KINDA_SMALL_NUMBER);
	const FVector2D ScreenSize = UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale;

	for (const TObjectPtr<UWxIndicatorDescriptor>& Indicator : Indicators)
	{
		FVector2D ScreenPosition;
		double DistanceToCamera = 0.0;
		if (!ProjectIndicator(*Indicator, ProjectionData, FVector2f(ScreenSize), ScreenPosition, DistanceToCamera))
		{
			// 대상이 파괴되면 표시만 접는다 — 등록증 제거는 등록한 쪽의 몫이다.
			Indicator->ClearProjection();
			continue;
		}

		const bool bClamped = ClampToScreen(ScreenSize, ScreenPosition);

		// 거리는 1m 단위로만 발행한다. 미세한 변화까지 흘리면 매 프레임 바인딩이 돈다.
		const float DistanceMeters = FMath::RoundToFloat(static_cast<float>(DistanceToCamera) / 100.f);

		Indicator->SetProjection(ScreenPosition, DistanceMeters, bClamped);
	}
}

void UWxIndicatorManagerComponent::UpdateTickEnabled()
{
	SetComponentTickEnabled(!Indicators.IsEmpty());
}
