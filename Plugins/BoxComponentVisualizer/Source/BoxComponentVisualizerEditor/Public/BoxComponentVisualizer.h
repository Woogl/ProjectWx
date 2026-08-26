// Copyright Woogle. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"

class UBoxComponent;

/** 클릭한 박스 모서리. 축마다 어느 끝인지(±1)를 셋으로 들어 여덟 모서리를 가른다. */
struct HBoxComponentVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HBoxComponentVisProxy(const UActorComponent* InComponent, const FVector& InCornerSign);

	FVector CornerSign;
};

/**
 * UBoxComponent 의 여덟 모서리에 핸들을 그리고, 그 핸들을 기즈모로 끌어 익스텐트를 편집한다.
 *
 * 잡은 모서리에 맞닿은 면만 따라오고 반대쪽은 고정된다 — 축마다 익스텐트와 위치를 절반씩 움직이면 그 동작이 나온다.
 * 디테일 패널의 익스텐트는 중심에서 각 면까지의 거리라 값 하나를 키우면 양쪽으로 벌어지고, 한쪽 벽에 맞추려면 익스텐트와 위치를 번갈아 손으로 계산해야 했다.
 *
 * 한 축 화살표만 끌면 그 면 하나, 평면 핸들이면 둘, 자유 이동이면 셋이 함께 움직인다.
 *
 * 편집 대상은 프로퍼티 경로로 들고 있다. BP 액터는 값이 바뀔 때마다 컴포넌트를 다시 만들 수 있어 포인터로는 드래그 도중에 끊긴다.
 */
class FBoxComponentVisualizer : public FComponentVisualizer
{
public:
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;

	virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;

	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;

	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;

	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;

	virtual void EndEditing() override;

	virtual UActorComponent* GetEditedComponent() const override;

	virtual bool IsVisualizingArchetype() const override;

private:
	FComponentPropertyPath BoxPropertyPath;

	FVector SelectedCornerSign = FVector::ZeroVector;

	float HandleSize = 10.0f;

	FLinearColor HandleColor = FLinearColor::White;

	float HudTextBottomMargin = 40.0f;
};
