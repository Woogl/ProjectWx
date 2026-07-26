// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidgetPool.h"
#include "Engine/LocalPlayer.h"
#include "UObject/GCObject.h"
#include "Widgets/SPanel.h"

class FActiveTimerHandle;
class FArrangedChildren;
class UWxIndicatorDescriptor;
class UWxIndicatorManagerComponent;
class UWxIndicatorWidget;
struct FSceneViewProjectionData;
struct FStreamableHandle;

/**
 * 등록된 인디케이터를 화면에 배치하는 패널. Lyra 의 SActorCanvas 대응이며 HUD 루트 위젯이 자기 내용 아래에 깐다.
 *
 * 컨트롤러의 인디케이터 매니저를 구독해 슬롯을 만들고, 매 갱신마다 대상의 월드 위치를 화면 좌표로 투영한다.
 * 대상이 화면 밖이면 화면 가장자리로 당겨 유지한다 — 월드 마커로는 줄 수 없는 정보다.
 *
 * 비용을 세 겹으로 줄인다: Tick 대신 Active Timer 로 돌면서 등록이 비면 타이머 자체를 멈추고, 슬롯 값이 실제로 변한
 * 갱신에서만 페인트를 무효화하며, 위젯은 풀에서 재사용한다. 위젯 클래스는 소프트 참조라 HUD 가 인디케이터 위젯을
 * 하드 참조하지 않고 등록 시점에 비동기 로드된다.
 */
class SWxIndicatorCanvas : public SPanel, public FGCObject
{
public:
	/** 인디케이터 1개의 슬롯. 캔버스가 계산한 화면 배치 상태를 함께 들고 있다. */
	class FSlot : public TSlotBase<FSlot>
	{
	public:
		FSlot(UWxIndicatorDescriptor* InIndicator, UWxIndicatorWidget* InIndicatorWidget)
			: TSlotBase<FSlot>()
			, Indicator(InIndicator)
			, IndicatorWidget(InIndicatorWidget)
			, ScreenPosition(FVector2D::ZeroVector)
			, DistanceToCamera(0.0)
			, NotifiedDistanceMeters(TNumericLimits<float>::Lowest())
			, bIsIndicatorVisible(true)
			, bDirty(true)
			, bClamped(false)
			, bClampedChanged(false)
		{
		}

		SLATE_SLOT_BEGIN_ARGS(FSlot, TSlotBase<FSlot>)
		SLATE_SLOT_END_ARGS()
		using TSlotBase<FSlot>::Construct;

		UWxIndicatorDescriptor* GetIndicator() const { return Indicator; }

		UWxIndicatorWidget* GetIndicatorWidget() const { return IndicatorWidget; }

		bool IsIndicatorVisible() const { return bIsIndicatorVisible; }

		void SetIndicatorVisible(bool bInVisible)
		{
			if (bIsIndicatorVisible != bInVisible)
			{
				bIsIndicatorVisible = bInVisible;
				bDirty = true;
				GetWidget()->SetVisibility(bInVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
			}
		}

		FVector2D GetScreenPosition() const { return ScreenPosition; }

		void SetScreenPosition(const FVector2D& InScreenPosition)
		{
			if (ScreenPosition != InScreenPosition)
			{
				ScreenPosition = InScreenPosition;
				bDirty = true;
			}
		}

		double GetDistanceToCamera() const { return DistanceToCamera; }

		void SetDistanceToCamera(double InDistanceToCamera)
		{
			if (DistanceToCamera != InDistanceToCamera)
			{
				DistanceToCamera = InDistanceToCamera;
				bDirty = true;
			}
		}

		bool IsDirty() const { return bDirty; }

		void ClearDirtyFlag() { bDirty = false; }

		bool IsClamped() const { return bClamped; }

		/** 클램프 여부는 const 인 배치 단계에서 정해지므로 캐시만 해 두고, 다음 갱신이 위젯에 알린다. */
		void SetClamped(bool bInClamped) const
		{
			if (bClamped != bInClamped)
			{
				bClamped = bInClamped;
				bClampedChanged = true;
			}
		}

		/** 위젯에 알릴 만큼 값이 변했는지. 거리는 1m 단위로만 보아 매 프레임 BP 호출을 피한다. */
		bool NeedsWidgetRefresh(float DistanceMeters) const
		{
			return bClampedChanged || FMath::Abs(DistanceMeters - NotifiedDistanceMeters) >= 1.f;
		}

		void MarkWidgetRefreshed(float DistanceMeters)
		{
			NotifiedDistanceMeters = DistanceMeters;
			bClampedChanged = false;
		}

	private:
		/** 캔버스의 AddReferencedObjects 가 살려 둔다. */
		UWxIndicatorDescriptor* Indicator;

		/** 위젯 풀이 살려 둔다. */
		UWxIndicatorWidget* IndicatorWidget;

		FVector2D ScreenPosition;
		double DistanceToCamera;
		float NotifiedDistanceMeters;

		uint8 bIsIndicatorVisible : 1;
		uint8 bDirty : 1;

		mutable uint8 bClamped : 1;
		mutable uint8 bClampedChanged : 1;
	};

	SLATE_BEGIN_ARGS(SWxIndicatorCanvas)
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
	SLATE_END_ARGS()

	SWxIndicatorCanvas();

	void Construct(const FArguments& InArgs, const FLocalPlayerContext& InLocalPlayerContext);

	//~ Begin SWidget
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual FChildren* GetChildren() override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	//~ End SWidget

	//~ Begin FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	//~ End FGCObject

private:
	using FScopedWidgetSlotArguments = TPanelChildren<FSlot>::FScopedWidgetSlotArguments;

	/** 아직 매니저를 못 찾았으면 찾아 구독하고 이미 등록된 것들을 따라잡는다. */
	bool TryBindManager();

	void HandleIndicatorAdded(UWxIndicatorDescriptor* Indicator);
	void HandleIndicatorRemoved(UWxIndicatorDescriptor* Indicator);
	void HandleIndicatorWidgetClassLoaded(UWxIndicatorDescriptor* Indicator);
	EActiveTimerReturnType HandleActiveTimer(double InCurrentTime, float InDeltaTime);

	/** 위젯 클래스를 확보한다. 이미 로드돼 있으면 스트리밍을 건너뛴다. */
	void RequestIndicatorWidget(UWxIndicatorDescriptor* Indicator);

	/** 풀에서 위젯을 얻어 슬롯을 만든다. 위젯 클래스가 로드된 뒤에만 부른다. */
	void CreateIndicatorWidget(UWxIndicatorDescriptor* Indicator);

	void AddIndicatorSlot(UWxIndicatorDescriptor* Indicator, UWxIndicatorWidget* IndicatorWidget);
	void RemoveIndicatorSlot(UWxIndicatorDescriptor* Indicator);

	/** 뷰 투영을 못 얻는 동안(월드 전환 등) 전부 접는다. */
	void SetShowAnyIndicators(bool bInShowAnyIndicators);

	void UpdateActiveTimer();

	/**
	 * 대상의 월드 위치를 화면 좌표와 카메라 거리로 바꾼다. 대상이 무효면 false.
	 * 카메라 뒤 대상은 투영 좌표가 화면 안으로 접혀 들어오므로 화면 밖으로 밀어내, 클램프가 올바른 가장자리를 잡게 한다.
	 */
	bool ProjectIndicator(const UWxIndicatorDescriptor& Indicator, const FSceneViewProjectionData& ProjectionData, const FVector2f& ScreenSize, FVector2D& OutScreenPosition, double& OutDistanceToCamera) const;

	FLocalPlayerContext LocalPlayerContext;
	TWeakObjectPtr<UWxIndicatorManagerComponent> ManagerPtr;

	/** 등록된 모든 등록증. 위젯 로드를 기다리는 것도 포함해 GC 에서 보호한다. */
	TArray<TObjectPtr<UWxIndicatorDescriptor>> AllIndicators;

	/** 위젯 클래스 로드 대기 목록. 늦게 도착한 완료 통지를 버리는 근거이기도 하다. */
	TMap<UWxIndicatorDescriptor*, TSharedPtr<FStreamableHandle>> PendingWidgetClassLoads;

	TPanelChildren<FSlot> CanvasChildren;

	FUserWidgetPool IndicatorPool;

	bool bShowAnyIndicators = false;

	/** 페인트에서 캐시한 지오메트리. 한 번도 페인트되지 않았으면 화면 크기를 몰라 투영하지 않는다. */
	mutable TOptional<FGeometry> OptionalPaintGeometry;

	TSharedPtr<FActiveTimerHandle> TickHandle;
};
